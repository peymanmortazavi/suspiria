//
// Created by Peyman Mortazavi on 2019-02-07.
//

#include <map>
#include <string>
#include <sstream>
#include <iostream>

#include <susperia/http.h>

using namespace std;
using namespace suspiria::networking;


map<int, string> msgs = {
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

std::map<HttpStatus, std::string> status_text_map = {
  {HttpStatus::OK, "OK"},
  {HttpStatus::BadRequest, "Bad Request"},
  {HttpStatus::NotFound, "Not Found"},
};

const string endh = "\r\n";


string HttpResponse::flush() {
  stringstream content;
  content << "HTTP/1.1 " << this->status << " " << status_text_map[this->status] << endh;
  this->headers["Server"] = "IpecacServer/1.0";
  for (const auto& item : headers) {
    content << item.first << ": " << item.second << endh;
  }
  auto data = writer.str();
  content << "Content-Length: " << data.size() << endh;
  content << endh << data << endh;
  return content.str();
}


static void handle_mg_event(struct mg_connection* connection, int event, void* data) {
  if (event == MG_EV_POLL) return;
  // cout << "Event: " << msgs[event] << endl;
  auto server = reinterpret_cast<WebSocketServer*>(connection->mgr->user_data);
  switch (event) {
    case MG_EV_HTTP_REQUEST:
      auto request = (http_message*) data;
      auto uri = string(request->uri.p, request->uri.len);
      if (auto handler = server->router.resolve(uri)) {
        auto http_request = HttpRequest();
        auto response = handler->handle(http_request);
        mg_printf(connection, "%s", response.flush().c_str());
      } else {
        auto response = HttpResponse();
        response.status = HttpStatus::NotFound;
        response.writer << "No Handler For " << uri;
        mg_printf(connection, "%s", response.flush().c_str());
      }
      connection->flags |= MG_F_SEND_AND_CLOSE;
      break;
  }
}


WebSocketServer::WebSocketServer(string address) {
  this->_address = move(address);
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