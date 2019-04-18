//
// Created by Peyman Mortazavi on 2019-02-09.
//

#ifndef SUSPIRIA_HTTP_H
#define SUSPIRIA_HTTP_H

#include <memory>
#include <unordered_map>
#include <string>

#include "routing/routing.h"
#include "ip.h"
#include "protocol.h"


namespace suspiria {

  namespace networking {

    enum HttpMethod : unsigned int {
      DELETE = 0,
      GET = 1,
      HEAD = 2,
      POST = 3,
      PUT = 4,
    };

    enum HttpStatus {
      OK = 200,
      NotFound = 404,
      BadRequest = 400,
    };


    class HttpRequest {
    public:
      std::string uri;
      HttpMethod method;
      bool keep_alive = false;
      std::unordered_map<std::string, std::string> headers;

      void reset() {
        uri.clear();
        keep_alive = false;
        headers.clear();
      }
    };


    class http_delegate {
    public:
      virtual void handle(HttpRequest& request) {}
    };


    template<typename Handler>
    class func_delegate : public http_delegate {
    public:
      explicit func_delegate(Handler&& handler) : handler_(std::move(handler)) {}
      void handle(HttpRequest& request) override { handler_(request); }

    private:
      Handler handler_;
    };


    class http_protocol_factory : public protocol_factory {
    public:
      http_protocol_factory(std::shared_ptr<http_delegate> delegate) : delegate_(std::move(delegate)) {}

      template<typename Handler>
      http_protocol_factory(Handler handler) : delegate_(std::make_shared<func_delegate<Handler>>(std::move(handler))) {}

      std::unique_ptr<protocol> create_protocol(tcp_connection& connection) override;

    protected:
      std::shared_ptr<http_delegate> delegate_;
    };


    class http_server : public tcp_server {
    public:
      http_server(const http_server& another) = delete;
      explicit http_server(
        asio::io_context& io, std::string host, unsigned short port, std::shared_ptr<http_delegate> delegate
      ) : tcp_server(io, std::move(host), port, std::make_shared<http_protocol_factory>(std::move(delegate))) {}

      template<typename Handler>
      explicit http_server(
        asio::io_context& io, std::string host, unsigned short port, Handler handler
      ) : tcp_server(io, std::move(host), port, std::make_shared<http_protocol_factory>(handler)) {}
    };

  }

}

#endif //SUSPIRIA_HTTP_H
