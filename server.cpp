#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <map>

#include <susperia/suspiria.h>

using namespace std;
using namespace suspiria::networking;


// Server Stuff
class IndexHandler : public HttpRequestHandler {
public:
  HttpResponse handle(HttpRequest &request) override {
    auto response = HttpResponse();
    response.headers["X-APP-NAME"] = "MessageCrypt";
    response.headers["Connection"] = "close";
    ifstream my_file;
    my_file.open("/home/tools/a.txt", ios::in);
    response.writer << my_file.rdbuf();
    return response;
  }
};


void start_server() {
  cout << "Hello from C++ world inside a docker image." << endl;
  WebSocketServer server("0.0.0.0:5000");
//  server.router.add_handler("/index", make_shared<IndexHandler>());
  server.polling_timeout = 200;
  server.start();
}


// Testing stuff.
#include <chrono>
#include <regex>

struct regex_experiment {
  void prepare() {
    res = {
            regex{R"(^/api/v3/auth/basic/?)", regex_constants::ECMAScript | regex_constants::icase},
            regex{R"(^/api/v3/users/(\w*)/jobs/(\d*)/details/?)", regex_constants::ECMAScript | regex_constants::icase},
            regex{R"(^/(\w*)/?)", regex_constants::ECMAScript | regex_constants::icase},
    };
    urls = {
            "/api/v3/auth/basic/",
            "/api/v3/users/peymanmo/jobs/23/details/",
            "/LucasConnors/",
    };
  }
  void perform() {
    for (auto i = 0; i < 1e6; i++) {
      auto url = urls[i%3];
      smatch match_result;
      for (auto& rule : res) {
        if (regex_match(url, match_result, rule)) {
          break;
        }
      }
    }
  }
  vector<regex> res;
  vector<string> urls;
};


struct node_experiment {

  struct handler {
    explicit handler(string&& _name) : name(move(_name)) {}
    void handle(const RouterParams& params) {
//      cout << name << endl;
//      for_each(begin(params), end(params), [](auto& element) {
//        cout << "   - " << element.first << " : " << element.second << endl;
//      });
    }
    string name;
  };

  struct url_resolver {
    virtual bool match(const string& dir, RouterParams& params) = 0;
  };

  struct regex_resolver : public url_resolver {
    explicit regex_resolver(const string& pattern, string name) {
      this->_rule = regex{pattern, regex_constants::ECMAScript | regex_constants::icase};
      this->_name = move(name);
    }

    bool match(const string &dir, RouterParams &params) override {
      smatch result;
      if (regex_match(dir, result, this->_rule)) {
        params[this->_name] = result.str(1);
        return true;
      }
      return false;
    }

  private:
    regex _rule;
    string _name;
  };

  struct node {
    string name;
    map<string, shared_ptr<node>> network;
    vector<tuple<unique_ptr<url_resolver>, shared_ptr<node>>> resolvers;
    shared_ptr<handler> node_handler;

    void add_node(string name, shared_ptr<node> node) {
      this->network[move(name)] = move(node);
    }
    void add_node(unique_ptr<url_resolver>&& resolver, const shared_ptr<node>& node_ptr) {
      this->resolvers.emplace_back(tuple<unique_ptr<url_resolver>, shared_ptr<node>>(move(resolver), node_ptr));
    }
  };

  void prepare() {
    auto basic_node = make_shared<node>();
    basic_node->node_handler = make_shared<handler>("basic");

    auto auth_node = make_shared<node>();
    auth_node->add_node("/basic", basic_node);

    auto details_node = make_shared<node>();
    details_node->node_handler = make_shared<handler>("details");

    auto job_node = make_shared<node>();
    job_node->add_node("/details", details_node);

    auto jobs_node = make_shared<node>();
    jobs_node->add_node(make_unique<regex_resolver>(R"(/(\d*))", "job_id"), job_node);

    auto user_node = make_shared<node>();
    user_node->add_node("/jobs", jobs_node);

    auto users_node = make_shared<node>();
    users_node->add_node(make_unique<regex_resolver>(R"(/(\w*))", "name"), user_node);

    auto v3_node = make_shared<node>();
    v3_node->add_node("/auth", auth_node);
    v3_node->add_node("/users", users_node);

    auto api_node = make_shared<node>();
    api_node->add_node("/v3", v3_node);

    auto facebook_node = make_shared<node>();
    facebook_node->node_handler = make_shared<handler>("facebook");

    root.add_node("/api", api_node);
    root.add_node(make_unique<regex_resolver>(R"(/(\w*))", "username"), facebook_node);

    urls = {
            "/api/v3/auth/basic/",
            "/api/v3/users/peymanmo/jobs/23/details/",
            "/LucasConnors/",
    };
  }
  void perform() {
    for (auto i = 0; i < 1e6; i++) {
      auto url = urls[i%3];
      resolve(url);
    }
  }

  void resolve(const string& path) {
    unsigned long start = 0;
    unsigned long count = 0;

    node* cursor = &this->root;
    RouterParams params;
    for(unsigned long index = 0; index < path.size(); index++) {
      if (path[index] == '/') {
        if (count) {
          auto word = path.substr(start, count);
          // first try the map
          const auto& map_it = cursor->network.find(word);
          if (map_it != end(cursor->network)) {
            cursor = map_it->second.get();
          } else {  // then loop through the resolvers.
            const auto& it = find_if(begin(cursor->resolvers), end(cursor->resolvers), [&word, &params](auto& element) {
              return get<0>(element)->match(word, params);
            });
            if (it != end(cursor->resolvers)) {
              cursor = get<1>(*it).get();
            } else {
              cout << "NO URL MATCHED" << endl;
              return;
            }
          }
        }
        start = index;
        count = 0;
      }
      count++;
    }
    if (const auto& node_handler = cursor->node_handler) {
      node_handler->handle(params);
    }
  }

  node root;
  vector<string> urls;
};


int main() {
  node_experiment e;
  e.prepare();
  auto start = std::chrono::high_resolution_clock::now();
  e.perform();
  auto finish = std::chrono::high_resolution_clock::now();
  chrono::duration<double> elapsed = finish - start;
  cout << elapsed.count() << endl;

  return 0;
}