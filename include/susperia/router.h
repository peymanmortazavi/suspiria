//
// Created by Peyman Mortazavi on 2019-02-09.
//

#ifndef SUSPIRIA_ROUTER_H
#define SUSPIRIA_ROUTER_H

#include <algorithm>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "utility.h"


namespace suspiria {

  namespace networking {

    typedef std::map<std::string, std::string> RouterParams;

    template<class T>
    struct ResolveResult {
      bool matched = false;
      std::shared_ptr<T> handler = nullptr;
      RouterParams params;
    };

    template<class T>
    struct Router {
      virtual ResolveResult<T> resolve(const std::string& path) const = 0;
    };

    /* RouteMatcher matches routes and extends the router params upon a successful match. */

    struct RouteMatcher {
      explicit RouteMatcher(const std::vector<std::string>& args) {
        auto hash_func = std::hash<std::string>();
        std::for_each(begin(args), end(args), [&](auto& arg) { this->_hash += hash_func(arg); });
      };
      virtual bool match(const std::string& route, RouterParams& params) const = 0;
      size_t hash_code() const { return this->_hash; }
      bool operator==(const RouteMatcher& other_matcher) const {
        return ((typeid(*this) == typeid(other_matcher)) && (other_matcher.hash_code() == this->hash_code()));
      }
      bool operator!=(const RouteMatcher& other_matcher) const {
        return !(*this == other_matcher);
      }
    private:
      size_t _hash = 0;
    };

    static utility::factory<RouteMatcher, const std::vector<std::string>&> route_matcher_factory;

    struct RegexRouteMatcher : public RouteMatcher {
      explicit RegexRouteMatcher(const std::vector<std::string>& args);
      bool match(const std::string &route, RouterParams &params) const override;
    private:
      std::regex _rule;
      std::vector<std::string> _names;
    };

    struct VariableRouteMatcher : public RouteMatcher {
      explicit VariableRouteMatcher(const std::vector<std::string>& args) : RouteMatcher(args), _name(args[0]) {}
      bool match(const std::string &route, RouterParams &params) const override {
        params[_name] = route;
        return true;
      }
    private:
      std::string _name;
    };

    /** GraphRouter comes with a fleet of specialty classes that accommodate graph routing, a fast generic routing tool.
     */
    template<class T>
    class RouterNode {
    public:
      struct route_matcher_pair {
        std::unique_ptr<RouteMatcher> matcher;
        std::shared_ptr<RouterNode> node;
      };

      std::string name;
      std::map<std::string, std::shared_ptr<RouterNode>> static_nodes;
      std::vector<route_matcher_pair> dynamic_nodes;
      std::shared_ptr<T> handler = nullptr;

      void add_node(std::string name, std::shared_ptr<RouterNode> node) {
          this->static_nodes[std::move(name)] = std::move(node);
      }

      void add_node(std::unique_ptr<RouteMatcher>&& resolver, std::shared_ptr<RouterNode> node) {
          this->dynamic_nodes.emplace_back(route_matcher_pair{std::move(resolver), std::move(node)});
      }
    };

    template<class T>
    class GraphRouter : public Router<T> {
    public:
      RouterNode<T> root;

      /**
       * Adds a new route to the graph router. It will automatically create all router nodes necessary to get to the
       * final path. For example /api/v3/auth will assure node api, v3 and auth get created if they don't already exist.
       * @param path The path to the handler. accepts format /static/name/and/<matcher_name:param1:param2:...>/other.
       * @param handler The actual handler that will be resolved once the provided path gets matched.
       * @param name A friendly name for this router node, you can easily get this node back by using this name.
       * @return The newly created or updated router_node<T>
       */
      RouterNode<T>& add_route(const std::string& path, std::shared_ptr<T> handler, const std::string& name="") {
        auto cursor = &this->root;
        utility::string_partitioner::for_each(path, [&](auto& route) {
          // If route is a static string.
          if (_is_static(route)) {
            auto map_it = cursor->static_nodes.find(route);
            if (map_it == end(cursor->static_nodes)) {  // create a node if one doesn't exist.
              cursor->add_node(route, std::make_shared<RouterNode<T>>());
            }
            cursor = cursor->static_nodes[route].get();
            return;
          }

          // If route is dynamic and needs to create a matcher.
          std::string matcher_name;
          std::vector<std::string> args;
          if (_parse_dynamic_route(route, matcher_name, args)) {  // try to parse the route as a dynamic route format.
            auto matcher = route_matcher_factory.get_factory(matcher_name).make_unique(args);
            auto matcher_it = std::find_if(begin(cursor->dynamic_nodes), end(cursor->dynamic_nodes), [&](auto& item) {
              return *item.matcher == *matcher;
            });
            if (matcher_it != end(cursor->dynamic_nodes)) {
              cursor = matcher_it->node.get();
            } else {
              auto new_node = new RouterNode<T>();
              cursor->add_node(std::move(matcher), std::shared_ptr<RouterNode<T>>(new_node));
              cursor = new_node;
            }
          } else {
            // route is invalid format. raise exception
            throw 25;
          }
        });
        cursor->handler = move(handler);
        cursor->name = name;
        return *cursor;
      }

      ResolveResult<T> resolve(const std::string &path) const override {
        ResolveResult<T> result{};
        const RouterNode<T>* head = &this->root;
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

    private:
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
