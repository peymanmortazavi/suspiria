//
// Created by Peyman Mortazavi on 2019-02-07.
//

#include <iostream>

#include "http_parser.h"

#include <susperia/internal/networking/http.h>

using namespace std;
using namespace suspiria::networking;


/*
 * The HTTP implementation.
**/
class http : public protocol {
public:
  http(tcp_connection& connection) : protocol(connection) {
    http_parser_init(&parser_, HTTP_REQUEST);
    parser_.data = this;
    parser_settings_.on_message_begin = &on_msg_begin;
    parser_settings_.on_url = &on_url;
    parser_settings_.on_message_complete = &on_msg_complete;
  }
  http(const http& another) = delete;

  void data_received(const char* bytes, size_t length) override {
    http_parser_execute(&parser_, &parser_settings_, bytes, length);
  }

private:
  static int on_url(http_parser* parser, const char* at, size_t length) {
    auto self = reinterpret_cast<http*>(parser->data);
    cout << "URL ";
    cout.write(at, length);
    cout.flush();
    return 0;
  }

  static int on_msg_begin(http_parser* parser) {
    cout << "MSG begin" << endl;
    return 0;
  }

  static int on_msg_complete(http_parser* parser) {
    cout << "MSG complete" << endl;
    auto self = reinterpret_cast<http*>(parser->data);
    self->connection_.close();
    return 0;
  }

  http_parser parser_;
  http_parser_settings parser_settings_;
};


unique_ptr<protocol> http_protocol_factory::create_protocol(tcp_connection& connection) {
  return make_unique<http>(connection);
}