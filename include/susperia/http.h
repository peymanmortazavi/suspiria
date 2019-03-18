//
// Created by Peyman Mortazavi on 2019-02-09.
//

#ifndef SUSPIRIA_HTTP_H
#define SUSPIRIA_HTTP_H

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <functional>

#include <mongoose/mongoose.h>

#include <susperia/router.h>
#include "router.h"


namespace suspiria {

  namespace networking {

    enum HttpStatus {
      OK = 200,
      NotFound = 404,
      BadRequest = 400,
    };

    class HttpRequest {
    public:
      explicit HttpRequest(RouterParams& params, mg_connection& connection, std::ostream& response_stream, std::istream& input_stream);
      RouterParams& url_params;
      std::istream& body;

    private:
      std::ostream& _response_stream;
      mg_connection& _connection;

      friend class StreamingResponse;
    };

    class HttpResponse {
    public:
      explicit HttpResponse(HttpStatus status = HttpStatus::OK) : status(status) {}
      std::unordered_map<std::string, std::string> headers;
      HttpStatus status;

      virtual void write(std::ostream& output);

    protected:
      void write_header(std::ostream& output);
    };

    class TextResponse : public HttpResponse {
    public:
      explicit TextResponse(std::string&& content, HttpStatus status = HttpStatus::OK);
      void write(std::ostream& output) override;
    private:
      std::string _content;
    };

    class StreamingResponse : public HttpResponse {
    public:
      void prepare(HttpRequest& request, const std::function<void(std::ostream&)>& write_func);
      void write(std::ostream& output) override { }
    };

    class HttpRequestHandler {
    public:
      virtual std::unique_ptr<HttpResponse> handle(HttpRequest& request) = 0;
    };

    class WebSocketServer {
    public:

      enum Status {
        Listening,  // When actively listening.
        Stopping,  // When the server is stopping all its operations and I/O loops. This is unlikely.
        Idle,  // When the server is not doing anything. No I/O loop here.
      };

      explicit WebSocketServer(std::string address, std::shared_ptr<Router<HttpRequestHandler>> router);
      ~WebSocketServer();
      void start();
      void stop();

      const Status& get_status() const { return _status; }
      int polling_timeout = 1000;  // timeout for every poll (in milliseconds)
      inline const Router<HttpRequestHandler>& get_router() const { return *_router; }

    private:
      mg_mgr _manager;
      std::string _address;
      Status _status = Status::Idle;
      std::shared_ptr<Router<HttpRequestHandler>> _router;
    };

  }

}

#endif //SUSPIRIA_HTTP_H
