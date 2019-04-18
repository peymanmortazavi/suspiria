#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <map>
#include <streambuf>
#include <sstream>
#include <unordered_set>

#include <susperia/suspiria.h>

using namespace std;
using namespace suspiria::networking;


#ifdef READY
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
#endif

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


// TCP Connection
/*
 * From the looks of it, we want to have a connection that is just there to be a connection and we can close it when we
 * desire. Then we want to have different protocols, like HTTP that can use the connection to tackle the request.
 * Now for the http handler, we'll create a request instance and put the connection on the http request.
 * That way if a response is just a straight forward response, we just return it. Otherwise, we'd have to call
 * prepare on it when passing the http request class which would then allow the response to access the connection
 * directly.
 * As far as the protocol upgrades go, we can either update the handler on the connection. but in the case of WebSockets
 * we might not need that. All we'd need is to stop calling the receive loop with the same handler and switch to one
 * where we just send data and receive data and pass it to the handler returned by the user.
**/
#include <asio.hpp>
using namespace asio;

void handle(HttpRequest& request) {
  cout << "URL >> " << request.uri << "\n";
  for (auto& item : request.headers) {
    cout << "@ " << item.first << " = " << item.second << "\n";
  }
  cout << "# Keep Alive ? " << (request.keep_alive ? "Yes" : "No") << "\n";
  cout << "# Method " << request.method << endl;
  cout << "eof --" << endl;
}

int main() {
  io_context io;
  http_server server{io, "localhost", 1600, handle};
  server.start();
  io.run();
  return 0;
}
