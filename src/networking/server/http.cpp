//
// Created by Peyman Mortazavi on 2019-02-07.
//

#include <unordered_map>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include <susperia/internal/networking/http.h>

using namespace std;
using namespace suspiria::networking;


unordered_map<int, string> msgs = {
  {MG_EV_WEBSOCKET_FRAME, "WebSocket Frame"},
  {MG_EV_WEBSOCKET_CONTROL_FRAME, "WebSocket Control Frame"},
  {MG_EV_WEBSOCKET_HANDSHAKE_DONE, "WebSocket Handshake Done"},
  {MG_EV_WEBSOCKET_HANDSHAKE_REQUEST, "WebSocket Handshake Request"},
  {MG_EV_HTTP_REQUEST, "HTTP Request"},
  {MG_EV_CLOSE, "Connection Closed"},
  {MG_EV_CONNECT, "Connection Received"},
  {MG_EV_RECV, "Connection Received Socket Data"},
  {MG_EV_SEND, "Connection Sent Socket Data"},
};

std::unordered_map<HttpStatus, std::string> status_text_map = {
  {HttpStatus::OK, "OK"},
  {HttpStatus::BadRequest, "Bad Request"},
  {HttpStatus::NotFound, "Not Found"},
};

string endh = "\r\n";

class mg_response_streambuf : public streambuf {
public:
  explicit mg_response_streambuf(mg_str& buffer) : begin_(buffer.p), end_(buffer.p + buffer.len), current_(buffer.p) {}

protected:
  int uflow() override {
    if (current_ == end_)
      return traits_type::eof();
    return traits_type::to_int_type(*current_++);
  }

  streamsize showmanyc() override {
    return end_ - current_;
  }

  int underflow() override {
    if (current_ == end_)
      return traits_type::eof();
    return traits_type::to_int_type(*current_);
  }

private:
  const char* begin_;
  const char* end_;
  const char* current_;
};

class mg_streambuf : public streambuf {
public:
  mg_connection* connection;
  char* buffer = nullptr;

  explicit mg_streambuf(mg_connection* connection, size_t buffer_size) : connection(connection) {
    buffer = new char[buffer_size + 1];
    setp(buffer, buffer + buffer_size - 1);
  }

protected:
  int_type overflow(int_type __c) override {
    if (__c != traits_type::eof()) {
      *pptr() = traits_type::to_char_type(__c);
      pbump(1);
      sync();
      return __c;
    }
    return traits_type::eof();
  }

  int_type sync() override {
    auto length = distance(pbase(), pptr());
    pbump(-length);
    connection->iface->vtable->tcp_send(connection, pbase(), length);
    return 0;
  }
};

class mg_chunked_streambuf : public streambuf {
public:
  mg_connection* connection;
  char* buffer = nullptr;
  char hex_holder[5];
  int max_length;

  explicit mg_chunked_streambuf(mg_connection* connection, size_t buffer_size) : connection(connection) {
    max_length = sprintf(hex_holder, "%lx", buffer_size);
    buffer = new char[buffer_size + max_length + 5];
    buffer[max_length] = '\r';
    buffer[max_length + 1] = '\n';
    setp(buffer + max_length + 2, buffer + buffer_size + max_length + 5);
  }

  /**
   * Returns true if there is some data available in the buffer that needs to be flushed.
   */
  bool needs_flush() const {
    return pptr() != pbase();
  }

protected:
  traits_type::int_type overflow(traits_type::int_type __c) override {
    if (__c != traits_type::eof()) {
      *pptr() = traits_type::to_char_type(__c);
      pbump(1);
      sync();
      return __c;
    }
    return traits_type::eof();
  }

  traits_type::int_type sync() override {
    auto length = distance(pbase(), pptr());
    pbump(-length);
    pbase()[length] = '\r';
    pbase()[length + 1] = '\n';
    auto count = sprintf(hex_holder, "%lx", length);
    auto offset = max_length - count;
    memcpy(buffer + offset, hex_holder, count);
    connection->iface->vtable->tcp_send(connection, buffer + offset, count + length + 4);
    return 0;
  }
};


void StreamingResponse::prepare(HttpRequest &request, const function<void(std::ostream&)>& write_func) {
  headers.emplace("Transfer-Encoding", "chunked");
  this->write_header(request._response_stream);
  request._response_stream.flush();
  auto* original_buffer = request._response_stream.rdbuf();
  auto* temp_buffer = new mg_chunked_streambuf{&request._connection, 8192};
  try {
    request._response_stream.rdbuf(temp_buffer);
    write_func(request._response_stream);
    if (temp_buffer->needs_flush()) {
      request._response_stream.flush();  // flush to actually empty the buffer.
    }
    request._response_stream.flush();  // flush to send 0 byte message indicating end of the stream.
  } catch(...) {
    request._response_stream.rdbuf(original_buffer);
    delete temp_buffer;
    throw;
  };
  request._response_stream.rdbuf(original_buffer);
  delete temp_buffer;
}

mg_streambuf * output_streambuf = new mg_streambuf(nullptr, 8182);
ostream suspiria_output_stream{output_streambuf};


static void handle_mg_event(struct mg_connection* connection, int event, void* data) {
  if (event == MG_EV_POLL) return;
  // cout << "Event: " << msgs[event] << endl;
  auto server = reinterpret_cast<WebSocketServer*>(connection->mgr->user_data);
  switch (event) {
    case MG_EV_HTTP_REQUEST:
      auto request = (http_message*) data;
      auto uri = string(request->uri.p, request->uri.len);
      auto result = server->get_router().resolve(uri);
      output_streambuf->connection = connection;
      auto receive_buffer = new mg_response_streambuf(request->body);
      auto input_stream = istream(receive_buffer);
      std::unique_ptr<HttpResponse> response;
      if (result.matched) {
        auto http_request = HttpRequest{result.params, *connection, suspiria_output_stream, input_stream};
        response = result.handler->handle(http_request);
      } else {
        response = move(make_unique<TextResponse>("No Handler for " + uri, HttpStatus::NotFound));
      }
      response->write(suspiria_output_stream);
      suspiria_output_stream.flush();
      delete receive_buffer;
      mbuf_remove(&connection->recv_mbuf, connection->recv_mbuf.len);
      connection->flags |= MG_F_SEND_AND_CLOSE;
      break;
  }
}


WebSocketServer::WebSocketServer(string address, shared_ptr<Router<HttpRequestHandler>> router) {
  this->_address = move(address);
  this->_router = move(router);
  mg_mgr_init(&this->_manager, this);
}

void WebSocketServer::start() {
  auto connection = mg_bind(&this->_manager, this->_address.c_str(), handle_mg_event);
  mg_set_protocol_http_websocket(connection);

  // If binding worked, set the server to listening and start the I/O loop.
  this->_status = Status::Listening;
  while (this->_status == Status::Listening) {  // So long as status is set to listening, run the I/O loop.
    mg_mgr_poll(&this->_manager, this->polling_timeout);
  }

  this->_status = Status::Idle;  // Since server is idle, stop the event loop.
}

void WebSocketServer::stop() {
  if (this->_status == Status::Listening) {
    this->_status = Status::Stopping;  // Passively update the flag to stopping so infinite loop stops.
  }
}

WebSocketServer::~WebSocketServer() {
  mg_mgr_free(&this->_manager);
}


HttpRequest::HttpRequest(RouteParams &params, mg_connection& connection, ostream &response_stream, istream &input_stream)
  : url_params(params), _response_stream(response_stream), _connection(connection), body(input_stream) {}


void HttpResponse::write_header(ostream& output) {
  output << "HTTP/1.1 " << status << " " << status_text_map[status] << endh;
  if (headers.find("Server") == end(headers)) { headers.emplace("Server", "Suspiria"); }
  for (const auto& item : headers) {
    output << item.first << ": " << item.second << endh;
  }
  output << endh;
}

void HttpResponse::write(std::ostream &output) {
  headers.emplace("Content-Length", "0");
  this->write_header(output);
}


TextResponse::TextResponse(string &&content, HttpStatus status) : _content(move(content)), HttpResponse(status) {}

void TextResponse::write(std::ostream &output) {
  headers["Content-Length"] = to_string(_content.size());
  if (headers.find("Content-Type") == end(headers)) { headers.emplace("Content-Type", "text/plain"); }
  this->write_header(output);
  output << _content;
}
