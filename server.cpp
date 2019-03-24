#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <map>
#include <streambuf>
#include <sstream>

#include <susperia/suspiria.h>

using namespace std;
using namespace suspiria::networking;


// Server Stuff
class IndexHandler : public HttpRequestHandler {
  stringstream s;
public:
  IndexHandler() {
    for (unsigned long i = 0; i < 100; ++i) {
      s << "LOOP " << i + 1 << endl;
    }
  }

  unique_ptr<HttpResponse> handle(HttpRequest &request) override {
    return make_unique<TextResponse>(s.str());
  }
};


class EchoHandler : public HttpRequestHandler {
public:
  unique_ptr<HttpResponse> handle(HttpRequest &request) override {
    string body{istreambuf_iterator<char>(request.body.rdbuf()), {}};
    return make_unique<TextResponse>(move(body));
  }
};


struct StreamingHandler : public HttpRequestHandler {
  unique_ptr<HttpResponse> handle(HttpRequest &request) override {
    auto response = make_unique<StreamingResponse>();
    response->headers["X-Name"] = "Streaming Handler";
    response->headers["Content-Type"] = "text/html";
    response->headers["Connection"] = "close";
    response->prepare(request, [](auto& writer) {
      for (unsigned long i = 0; i < 100; ++i) {
        writer << "LOOP " << i + 1 << "\r\n";
      }
    });
    return response;
  }
};


void start_server() {
  cout << "Hello from C++ world inside a docker image." << endl;

  auto router = make_shared<GraphRouter<HttpRequestHandler>>();
  router->add_route("/", make_shared<IndexHandler>());
  router->add_route("/echo", make_shared<EchoHandler>());
  router->add_route("/loop", make_shared<StreamingHandler>());

  WebSocketServer server{"0.0.0.0:1500", router};
  server.polling_timeout = 200;
  server.start();
}


// Testing stuff.
#include <chrono>
#include <regex>


template<typename PerformMethod>
double timeit(PerformMethod&& perform) {
  auto start = std::chrono::high_resolution_clock::now();
  perform();
  auto finish = std::chrono::high_resolution_clock::now();
  return static_cast<chrono::duration<double>>(finish - start).count();
}

int main() {
  start_server();
  return 0;
}