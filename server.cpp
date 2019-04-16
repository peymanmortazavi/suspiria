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
#include "src/networking/http_parser.h"

using namespace asio;
using namespace asio::ip;


template <typename ConnectionType>
class pool {
public:
  explicit pool() = default;
  pool(const pool&) = delete;
  ~pool() {
    this->close_all();
  }

  void add_connection(shared_ptr<ConnectionType> connection) {
    connection->async_activate();
    connections_.emplace(move(connection));
  }

  void close_connection(shared_ptr<ConnectionType> connection) {
    auto it = connections_.find(connection);
    if (it != end(connections_)) {
      connections_.erase(it);
    }
  }

  void close_all() {
    connections_.clear();
  }

private:
  unordered_set<shared_ptr<ConnectionType>> connections_;
};


class protocol {
public:
  virtual void data_received(const char* bytes, size_t length) = 0;
  virtual ~protocol() {};
};


class tcp_connection : public std::enable_shared_from_this<tcp_connection> {
public:
  tcp_connection(const tcp_connection&) = delete;
  explicit tcp_connection(tcp::socket&& socket, pool<tcp_connection>& pool) : socket_(move(socket)), pool_(pool) {
    receive_buffer_ = new char[4];
  }
  ~tcp_connection() {
    delete[] this->receive_buffer_;
  }

  tcp::socket& get_socket() { return socket_; }

  void set_protocol(unique_ptr<protocol>&& protocol) {
    protocol_ = move(protocol);
  }

  void async_activate() {
    this->run_receive_loop();
  }

  void close() {
    cerr << "trying to close" << endl;
    asio::error_code ec;
    socket_.shutdown(socket_.shutdown_both, ec);
    pool_.close_connection(shared_from_this());
  }

private:
  void run_receive_loop() {
    auto self(shared_from_this());
    socket_.async_receive(asio::buffer(receive_buffer_, 4), [this, self](const auto& error_code, size_t length) {
      if (error_code) {
        cout << "receive code: " << error_code << " = " << error_code.message() << endl;
        if (error_code != asio::error::operation_aborted) this->close();
        return;
      }
      cout << "Just received " << length << endl;
      protocol_->data_received(receive_buffer_, length);
      if (!socket_.is_open()) this->close(); else this->run_receive_loop();
    });
  }

  unique_ptr<protocol> protocol_;
  char* receive_buffer_;
  asio::streambuf send_buffer_;
  tcp::socket socket_;
  pool<tcp_connection>& pool_;
};


template<typename Protocol>
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
        auto connection = make_shared<tcp_connection>(move(socket), pool_);
        connection->set_protocol(make_unique<Protocol>(*connection));
        pool_.add_connection(move(connection));
      }
      run_accept_loop();
    });
  }

  unsigned short port_;
  string host_;
  tcp::acceptor acceptor_;
  io_context& io_;

  pool<tcp_connection> pool_;
};

class http : public protocol {
public:
  http(tcp_connection& connection) : connection_(connection) {
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
  tcp_connection& connection_;
};

typedef tcp_server<http> http_server;


int main() {
  io_context io;
  http_server server{io, "localhost", 1600};
  server.start();
  io.run();
  return 0;
}