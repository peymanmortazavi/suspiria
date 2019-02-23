#include <utility>

//
// Created by Peyman Mortazavi on 2019-02-09.
//

#ifndef SUSPIRIA_ROUTER_H
#define SUSPIRIA_ROUTER_H

#include <algorithm>
#include <unordered_map>
#include <memory>
#include <regex>
#include <string>
#include <vector>
#include <functional>

#include "utility.h"


namespace suspiria {

  namespace networking {

    typedef std::unordered_map<std::string, std::string> RouterParams;
    typedef std::vector<std::string> RouteMatcherParams;

    template<class T>
    struct ResolveResult {
      bool matched = false;
      std::shared_ptr<T> handler = nullptr;
      RouterParams params;
    };

    template<class T>
    class Router {
    public:
      virtual ResolveResult<T> resolve(const std::string& path) const = 0;
    };

    /* RouteMatcher matches routes and extends the router params upon a successful match. */

    class RouteMatcher {
    public:
      virtual ~RouteMatcher() = default;
      virtual bool match(const std::string& route, RouterParams& params) const noexcept = 0;
    };

    class RegexRouteMatcher : public RouteMatcher {
    public:
      explicit RegexRouteMatcher(const std::string& pattern, const std::vector<std::string>& capture_names);
      explicit RegexRouteMatcher(const std::string& pattern, std::vector<std::string>&& capture_names);
      bool match(const std::string &route, RouterParams &params) const noexcept override;
      static std::shared_ptr<RegexRouteMatcher> create_from_args(RouteMatcherParams&& route_args) {
        auto pattern = route_args[0];
        route_args.erase(route_args.begin());
        return std::make_shared<RegexRouteMatcher>(pattern, std::move(route_args));
      }
    private:
      std::regex _rule;
      std::vector<std::string> _names;
    };

    class VariableRouteMatcher : public RouteMatcher {
    public:
      explicit VariableRouteMatcher(std::string name) : _name(std::move(name)) {}
      bool match(const std::string &route, RouterParams &params) const noexcept override {
        params[_name] = route;
        return true;
      }
      static std::shared_ptr<VariableRouteMatcher> create_from_args(RouteMatcherParams&& route_args) {
        return std::make_shared<VariableRouteMatcher>(route_args[0]);
      }
    private:
      std::string _name;
    };

    /** GraphRouter comes with a fleet of specialty classes that accommodate graph routing, a fast generic routing tool.
     */
    template<class T>
    class RouterNode {
    public:
      struct dynamic_node {
        size_t hash = 0;
        std::shared_ptr<RouteMatcher> matcher;
        std::shared_ptr<RouterNode> node;
      };

      std::string name;
      std::unordered_map<std::string, std::shared_ptr<RouterNode>> static_nodes;
      std::vector<dynamic_node> dynamic_nodes;
      std::shared_ptr<T> handler = nullptr;

      void add_node(std::string name, std::shared_ptr<RouterNode> node) {
          this->static_nodes[std::move(name)] = std::move(node);
      }
      std::shared_ptr<RouterNode> get_static_node(const std::string& name) const noexcept {
        auto it = static_nodes.find(name);
        if (it == end(static_nodes)) return nullptr;
        return it->second;
      }

      void add_node(size_t hash, std::shared_ptr<RouteMatcher> matcher, std::shared_ptr<RouterNode> node) {
          this->dynamic_nodes.emplace_back(dynamic_node{hash, std::move(matcher), std::move(node)});
      }
      std::shared_ptr<RouterNode> get_dynamic_node(const size_t& hash) const noexcept {
        auto find_it = std::find_if(begin(dynamic_nodes), end(dynamic_nodes), [&hash](auto& item) {
          return item.hash == hash;
        });
        if (find_it == end(dynamic_nodes)) return nullptr;
        return find_it->node;
      }
    };

    template<class T>
    class GraphRouter : public Router<T> {
    public:
      typedef std::function<std::shared_ptr<RouteMatcher>(RouteMatcherParams&&)> RouteMatcherBuilder;

      GraphRouter() {
        // Register the basic route matchers.
        this->add_route_matcher_alias("var", &VariableRouteMatcher::create_from_args);
        this->add_route_matcher_alias("regex", &RegexRouteMatcher::create_from_args);
      };

      RouterNode<T>& add_route(const std::string& path, const RouterNode<T>& node, const std::string& name="") {
        auto new_root = this->_mk_route(path);
        new_root->handler = node.handler;
        new_root->static_nodes.insert(begin(node.static_nodes), end(node.static_nodes));
        std::copy(begin(node.dynamic_nodes), end(node.dynamic_nodes), std::back_inserter(new_root->dynamic_nodes));
        return *new_root;
      }

      /**
       * Adds a new route to the graph router. It will automatically create all router nodes necessary to get to the
       * final path. For example /api/v3/auth will assure node api, v3 and auth get created if they don't already exist.
       * @param path The path to the handler. accepts format /static/name/and/<matcher_name:param1:param2:...>/other.
       * @param handler The actual handler that will be resolved once the provided path gets matched.
       * @param name A friendly name for this router node, you can easily get this node back by using this name.
       * @return The newly created or updated router_node<T>
       */
      RouterNode<T>& add_route(const std::string& path, std::shared_ptr<T> handler, const std::string& name="") {
        auto node = this->_mk_route(path);
        node->handler = move(handler);
        node->name = name;
        return *node;
      }

      /**
       * Returns a ResolveResult containing the matched handler with its matched keyed parameter list if there is
       * actually a match.
       * @param path The resource path.
       * @return
       */
      ResolveResult<T> resolve(const std::string &path) const override {
        ResolveResult<T> result{};
        const RouterNode<T>* head = &this->_root;
        utility::string_partitioner it{path};
        std::string route;
        while (it.next(route)) {
          if(auto node = _resolve(head, route, result.params)) {
            head = node;
          } else {
            return result;
          }
        }
        if (head->handler) {
          result.matched = true;
          result.handler = head->handler;
        }
        return result;
      }

      /**
       * Adds an alias for a route matcher builder. The way it works is that if an alias X exists for route matcher
       * Y. Then calls to add_router of type /some/path/<X:arg0:arg1:arg2:...>/continue/path would call the provided
       * route matcher builder with the populated argument set.
       *
       * @param alias the name of the alias to be referenced later on with format <alias_name:arg0:arg1:...>
       * @param builder the builder. This could be a lambda or a function reference.
       */
      void add_route_matcher_alias(std::string alias, RouteMatcherBuilder builder) {
        this->_route_matcher_builder.emplace(std::move(alias), std::move(builder));
      }

      RouterNode<T>& get_root() noexcept { return _root; }

    private:
      std::unordered_map<std::string, RouteMatcherBuilder> _route_matcher_builder;
      RouterNode<T> _root;

      bool _is_static(const std::string& route) {
        std::regex static_node_matcher{R"(^[\w\d]*$)", std::regex_constants::ECMAScript | std::regex_constants::icase};
        return std::regex_match(route, static_node_matcher);
      }

      bool _parse_dynamic_route(const std::string& route, std::string& matcher_name, std::vector<std::string>& args) {
        std::regex dynamic_node_matcher{R"(^<([\w\d]*)((?::[\d\w!@#$%^&*()~\\,-]*)*)>$)", std::regex_constants::ECMAScript};
        std::smatch match_result;
        if (std::regex_match(route, match_result, dynamic_node_matcher)) {
          matcher_name = match_result.str(1);
          utility::string_partitioner::for_each(match_result.str(2), [&args](auto arg) {
            args.push_back(std::move(arg));
          }, ':');
          return true;
        }
        return false;
      }

      RouterNode<T>* _mk_route(const std::string& path) {
        auto cursor = &this->_root;
        utility::string_partitioner::for_each(path, [&](auto& route) {
          // If route is a static string.
          if (_is_static(route)) {
            if (auto static_node = cursor->get_static_node(route))
              cursor = static_node.get();
            else {
              auto new_node = new RouterNode<T>();
              cursor->add_node(route, std::shared_ptr<RouterNode<T>>(new_node));
              cursor = new_node;
            }
            return;
          }

          // If route is dynamic and needs to create a matcher.
          std::string matcher_name;
          RouteMatcherParams args;
          if (_parse_dynamic_route(route, matcher_name, args)) {  // try to parse the route as a dynamic route format.
            auto hash = std::hash<std::string>()(route);  // make a hash of the route.
            if (auto dynamic_node = cursor->get_dynamic_node(hash))
              cursor = dynamic_node.get();
            else {
              auto matcher = _route_matcher_builder[matcher_name](std::move(args));
              auto new_node = new RouterNode<T>();
              cursor->add_node(hash, std::move(matcher), std::shared_ptr<RouterNode<T>>(new_node));
              cursor = new_node;
            }
            return;
          } else {
            // route is invalid format. raise exception
            throw std::invalid_argument{"The provided path is not valid: " + route};
          }
        });
        return cursor;
      }

      const RouterNode<T> * _resolve(const RouterNode<T>* head, const std::string& route, RouterParams& params) const {
          // First try the fast hash map approach for static static_nodes.
          const auto& map_it = head->static_nodes.find(route);
          if (map_it != end(head->static_nodes)) {
              return map_it->second.get();
          }

          // Now go through all matchers and try to match.
          const auto& matcher_it = std::find_if(begin(head->dynamic_nodes), end(head->dynamic_nodes), [&route, &params](auto& pair) {
            return pair.matcher->match(route, params);
          });
          if (matcher_it != end(head->dynamic_nodes)) {
              return matcher_it->node.get();
          }

          // Otherwise, just return nothing.
          return nullptr;
      }
    };

  }

}

#endif //SUSPIRIA_ROUTER_H
