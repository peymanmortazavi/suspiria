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
    parser_settings_.on_header_field = &on_header_field;
    parser_settings_.on_header_value = &on_header_value;
    parser_settings_.on_headers_complete = &on_headers_complete;
    parser_settings_.on_body = &on_body;
  }
  http(const http& another) = delete;

  void data_received(const char* bytes, size_t length) override {
    http_parser_execute(&parser_, &parser_settings_, bytes, length);
  }

private:
  static int on_headers_complete(http_parser* parser) { return 0; }
  static int on_url(http_parser* parser, const char* at, size_t length) {
    auto self = reinterpret_cast<http*>(parser->data);
    self->request_.uri.append(at, length);
    return 0;
  }

  static int on_body(http_parser* parser, const char* at, size_t length) {
    return 0;
  }

  static int on_header_field(http_parser* parser, const char* at, size_t length) {
    auto self = reinterpret_cast<http*>(parser->data);
    if (self->should_clear_key_) {
      self->header_key_.clear();
      self->should_clear_key_ = false;
    }
    self->header_key_.append(at, length);
    return 0;
  }

  static int on_header_value(http_parser* parser, const char* at, size_t length) {
    auto self = reinterpret_cast<http*>(parser->data);
    self->request_.headers[self->header_key_].append(at, length);
    self->should_clear_key_ = true;
    return 0;
  }

  static int on_msg_begin(http_parser* parser) {
    auto self = reinterpret_cast<http*>(parser->data);
    self->request_.reset();
    cout << "HTTP New Message" << endl;
    return 0;
  }

  static int on_msg_complete(http_parser* parser) {
    cout << "MSG complete" << endl;
    auto self = reinterpret_cast<http*>(parser->data);
    cout << self->request_.uri << endl;
    for (auto& item : self->request_.headers) {
      cout << item.first << " : " << item.second << endl;
    }
    if (!self->request_.keep_alive)
      self->connection_.close();
    return 0;
  }

  bool should_clear_key_ = false;
  string header_key_;
  HttpRequest request_;
  http_parser parser_;
  http_parser_settings parser_settings_;
};


unique_ptr<protocol> http_protocol_factory::create_protocol(tcp_connection& connection) {
  return make_unique<http>(connection);
}