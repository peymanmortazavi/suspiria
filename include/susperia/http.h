//
// Created by Peyman Mortazavi on 2019-02-09.
//

#ifndef SUSPIRIA_HTTP_H
#define SUSPIRIA_HTTP_H

#include <sstream>

#include <mongoose/mongoose.h>

#include <susperia/router.h>


namespace suspiria {

  namespace networking {

    enum HttpStatus {
      OK = 200,
      NotFound = 404,
      BadRequest = 400,
    };

    class HttpResponse {
    public:
      std::map<std::string, std::string> headers;
      HttpStatus status = HttpStatus::OK;
      std::stringstream writer;

      std::string flush();
    };


    struct HttpRequest {
      RouterParams& url_params;
    };


    class HttpRequestHandler {
    public:
      virtual HttpResponse handle(HttpRequest& request) = 0;
    };

    class WebSocketServer {
    public:

      enum Status {
        Listening,  // When actively listening.
        Stopping,  // When the server is stopping all its operations and I/O loops. This is unlikely.
        Idle,  // When the server is not doing anything. No I/O loop here.
      };

      explicit WebSocketServer(std::string address);
      ~WebSocketServer();
      void start();
      void stop();

      const Status& get_status() const { return _status; }
      int polling_timeout = 1000;  // timeout for every poll (in milliseconds)
      GraphRouter<HttpRequestHandler> router;

    private:
      mg_mgr _manager;
      std::string _address;
      Status _status = Status::Idle;
    };

  }

}

#endif //SUSPIRIA_HTTP_H
