#include <iostream>
#include <susperia/suspiria.h>

using namespace std;
using namespace suspiria::networking;


class IndexHandler : public HttpRequestHandler {
public:
  HttpResponse handle(HttpRequest &request) override {
    auto response = HttpResponse();
    response.headers["X-APP-NAME"] = "MessageCrypt";
    response.headers["Connection"] = "close";
    for (auto i = 0; i < 100000; i++) {
      response.writer << 1;
    }
    return response;
  }
};



int main() {
  cout << "Hello from C++ world inside a docker image." << endl;
  WebSocketServer server("0.0.0.0:5001");
  server.router.add_handler("/index", make_shared<IndexHandler>());
  server.polling_timeout = 200;
  server.start();

  return 0;
}