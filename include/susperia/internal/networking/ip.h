//
// Created by Peyman Mortazavi on 2019-04-16.
//

#ifndef SUSPIRIA_IP_H
#define SUSPIRIA_IP_H

#include <memory>
#include <string>
#include <unordered_set>

#include <asio.hpp>

#include "protocol.h"


namespace suspiria::networking {

  template <typename ConnectionType>
  class pool {
  public:
    explicit pool() = default;
    pool(const pool&) = delete;
    ~pool() {
      this->close_all();
    }

    void add_connection(std::shared_ptr<ConnectionType> connection) {
      connections_.emplace(std::move(connection));
    }

    void close_connection(std::shared_ptr<ConnectionType> connection) {
      auto it = connections_.find(connection);
      if (it != end(connections_)) {
        connections_.erase(it);
      }
    }

    void close_all() {
      connections_.clear();
    }

  private:
    std::unordered_set<std::shared_ptr<ConnectionType>> connections_;
  };


  class tcp_connection : public std::enable_shared_from_this<tcp_connection> {
  public:
    tcp_connection(const tcp_connection&) = delete;
    explicit tcp_connection(asio::ip::tcp::socket&& socket, pool<tcp_connection>& pool) : socket_(std::move(socket)), pool_(pool) {
      receive_buffer_ = new char[4];
    }
    ~tcp_connection() {
      delete[] this->receive_buffer_;
    }

    asio::ip::tcp::socket& get_socket() { return socket_; }

    void set_protocol(std::unique_ptr<protocol>&& protocol) {
      protocol_ = std::move(protocol);
      protocol_->connection_made();
      this->resume_reading();
    }

    void pause_reading() { reading_paused_ = true; }
    void resume_reading() {
      reading_paused_ = false;
      if (!is_reading_) run_receive_loop();
    }

    void close() {
      asio::error_code ec;
      socket_.shutdown(socket_.shutdown_both, ec);
      protocol_->connection_lost();
      pool_.close_connection(shared_from_this());
    }

  private:
    void run_receive_loop() {
      this->is_reading_ = true;
      auto self(shared_from_this());
      socket_.async_receive(asio::buffer(receive_buffer_, 4), [this, self](const auto& error_code, size_t length) {
        if (error_code) {
          if (error_code != asio::error::operation_aborted) this->close();
          return;
        }
        protocol_->data_received(receive_buffer_, length);
        if (!socket_.is_open()) {
          this->close();
        } else if (!this->reading_paused_) {
          this->run_receive_loop();
        } else {
          this->is_reading_ = false;
        }
      });
    }

    bool is_reading_ = false;
    bool reading_paused_ = false;
    std::unique_ptr<protocol> protocol_;
    char* receive_buffer_;
    asio::streambuf send_buffer_;
    asio::ip::tcp::socket socket_;
    pool<tcp_connection>& pool_;
  };


  class tcp_server {
  public:
    tcp_server(tcp_server& server) = delete;
    explicit tcp_server(
      asio::io_context& io,
      std::string host,
      unsigned short port,
      std::shared_ptr<protocol_factory> factory
    ) : io_(io), host_(std::move(host)), port_(port), acceptor_(io), protocol_factory_(std::move(factory)) {}

    void start() {
      asio::ip::tcp::resolver resolver{io_};
      asio::ip::tcp::endpoint endpoint = *resolver.resolve(host_, std::to_string(port_)).begin();
      this->acceptor_.open(endpoint.protocol());
      this->acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
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
        } else {  // create a connection and add it to the connection pool.
          auto connection = std::make_shared<tcp_connection>(std::move(socket), pool_);
          connection->set_protocol(protocol_factory_->create_protocol(*connection));
          pool_.add_connection(move(connection));
        }
        run_accept_loop();
      });
    }

    std::shared_ptr<protocol_factory> protocol_factory_;
    unsigned short port_;
    std::string host_;
    asio::ip::tcp::acceptor acceptor_;
    asio::io_context& io_;
    pool<tcp_connection> pool_;
  };

}

#endif //SUSPIRIA_IP_H