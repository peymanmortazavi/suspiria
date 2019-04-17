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

    enum HttpStatus {
      OK = 200,
      NotFound = 404,
      BadRequest = 400,
    };


    class NewHttpRequest {
    public:
      std::string uri;
      std::string method;
      bool keep_alive = false;
      std::unordered_map<std::string, std::string> headers;
    };


    class http_protocol_factory : public protocol_factory {
    public:
      std::unique_ptr<protocol> create_protocol(tcp_connection& connection) override;
    };


    class http_server : public tcp_server {
    public:
      http_server(
        asio::io_context& io, std::string host, unsigned short port
      ) : tcp_server(io, std::move(host), port, std::make_shared<http_protocol_factory>()) {}
    };

  }

}

#endif //SUSPIRIA_HTTP_H
