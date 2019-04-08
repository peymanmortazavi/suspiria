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


// TCP Connection
#include <asio.hpp>
#include "src/networking/http_parser.h"

using namespace asio;
using namespace asio::ip;


template <typename T>
class pool {
public:
  explicit pool() = default;
  pool(const pool&) = delete;

  void add_connection(shared_ptr<T> connection) {
    connection->async_activate();
    connections_.emplace(move(connection));
  }

  void close_connection(const shared_ptr<T>& connection) {
    auto it = connections_.find(connection);
    if (it != end(connections_))
      connections_.erase(it);
  }

  void close_all() {
    connections_.clear();
  }

private:
  unordered_set<shared_ptr<T>> connections_;
};


class http_connection : public std::enable_shared_from_this<http_connection> {
public:
  http_connection(const http_connection&) = delete;
  explicit http_connection(tcp::socket&& socket, pool<http_connection>& pool) : socket_(move(socket)), pool_(pool) {
    http_parser_init(&parser_, HTTP_REQUEST);
    parser_.data = this;
    parser_settings_.on_body = &on_data_event;
    parser_settings_.on_headers_complete = &on_event;
    parser_settings_.on_message_begin = &on_event;
    parser_settings_.on_url = &on_url_capture;
    parser_settings_.on_message_complete = &on_request_ready;
    rec_buff_ = new char[4];
  }
  ~http_connection() {
    delete[] this->rec_buff_;
  }

  tcp::socket& get_socket() { return socket_; }

  void async_activate() {
    this->run_receive_loop();
  }

  void close() {
//    socket_.shutdown(socket_.shutdown_both);
    pool_.close_connection(shared_from_this());
  }

  void handle(NewHttpRequest& request, ostream& response) {
    response << "@url: " << request.uri << endl;
  }

private:
  static int on_request_ready(http_parser* parser) {
    auto self = reinterpret_cast<http_connection*>(parser->data);
    ostream os(&self->send_buffer_);
    self->handle(self->request_, os);
    auto n = self->socket_.send(self->send_buffer_.data());
    self->send_buffer_.consume(n);
//    self->close();
    self->should_close_ = true;
    // self->socket_.async_send(self->send_buffer_.data(), [&](const auto& error_code, size_t len) {
    //     if (error_code) {
    //       cout << "send code: " << error_code << endl;
    //       self->close();
    //       return;
    //     }
    //     self->send_buffer_.consume(len);
    //     self->close();
    //   });
    return 10;
  }

  static int on_data_event(http_parser* parser, const char* at, unsigned long length) { return 0; }
  static int on_event(http_parser* parser) { return 0; }
  
  static int on_url_capture(http_parser* parser, const char* at, unsigned long length) {
    auto self = reinterpret_cast<http_connection*>(parser->data);
    self->request_.uri += string{at, length};
    return 0;
  }

  void run_receive_loop() {
    socket_.async_receive(asio::buffer(rec_buff_, 4), [this](const auto& error_code, size_t length) {
      if (error_code) {
        cout << "receive code: " << error_code << endl;
        this->close();
        return;
      }
      cout << "Just received " << length << endl;
      auto consumed = http_parser_execute(&parser_, &parser_settings_, rec_buff_, length);
      if (consumed != length) {
        socket_.send(asio::buffer("BAD! I'm going away!\r\n"));
        this->close();
        return;
      }
      // istream is(&recieve_buffer_);
      // ostream os(&send_buffer_);
      // this->handle(is, os);
      // socket_.async_send(send_buffer_.data(), [this](const auto& error_code, size_t len) {
      //   if (error_code) {
      //     cout << "send code: " << error_code << endl;
      //     return;
      //   }
      //   cout << "Just sent " << len << endl;
      //   send_buffer_.consume(len);
      // });
      try { if (should_close_ || !socket_.is_open()) this->close(); else this->run_receive_loop(); }
      catch (exception& error) { cerr << error.what() << endl; }
    });
  }

  bool should_close_ = false;
  char* rec_buff_;
  
  NewHttpRequest request_;
  http_parser_settings parser_settings_;
  http_parser parser_;

  asio::streambuf recieve_buffer_;
  asio::streambuf send_buffer_;
  tcp::socket socket_;
  pool<http_connection>& pool_;
};


template<typename T>
class tcp_server {
public:
  tcp_server(tcp_server& server) = delete;
  explicit tcp_server(io_context& io, string host, unsigned short port) : io_(io), host_(move(host)), port_(port), acceptor_(io) {}

  void start() {
    tcp::resolver resolver{io_};
    tcp::endpoint endpoint = *resolver.resolve(host_, to_string(port_)).begin();
    this->acceptor_.open(endpoint.protocol());
    this->acceptor_.set_option(tcp::acceptor::reuse_address(true));
    this->acceptor_.bind(endpoint);
    this->acceptor_.listen();
    this->run_accept_loop();
  }

  void stop() {
    this->acceptor_.close();
    this->pool_.close_all();
  }

private:
  void run_accept_loop() {
    this->acceptor_.async_accept([this](const auto& error_code, auto socket) {
      if (!socket.is_open()) return;  // If the socket isn't open for any reason, do not proceed.
      if (error_code) {  // if there is any error, print it out for now and move on.
        cerr << "error: " << error_code << endl;
      } else {  // create a connection and add it to the connection pool.
        pool_.add_connection(make_shared<T>(move(socket), pool_));
      }
      run_accept_loop();
    });
  }

  unsigned short port_;
  string host_;
  tcp::acceptor acceptor_;
  io_context& io_;

  pool<T> pool_;
};

typedef tcp_server<http_connection> http_server;


int main() {
  io_context io;
  http_server server{io, "localhost", 1600};
  server.start();
  io.run();
  return 0;
}