//
// Created by Peyman Mortazavi on 2019-04-16.
//

#ifndef SUSPIRIA_PROTOCOL_H
#define SUSPIRIA_PROTOCOL_H

#include <memory>


namespace suspiria::networking {

  class tcp_connection;

  class protocol {
  public:
    protocol(tcp_connection& connection) : connection_(connection) {}
    virtual void data_received(const char* bytes, size_t length) = 0;
    virtual ~protocol() {};

  protected:
    tcp_connection& connection_;
  };


  class protocol_factory {
  public:
    virtual std::unique_ptr<protocol> create_protocol(tcp_connection& connection) = 0;
    virtual ~protocol_factory() {};
  };

}

#endif //SUSPIRIA_PROTOCOL_H