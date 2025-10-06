#pragma once
#include <iostream>
#include <thread>
#if defined(ASIO_STANDALONE) && ASIO_STANDALONE
// Some standalone Asio versions use io_context instead of io_service.
// Websocketpp expects io_service and io_service::strand names. Provide
// compatibility aliases so websocketpp compiles against modern standalone Asio
// on Windows (mingw/msys2) where io_service may be missing.
namespace asio {
// forward declare io_context if not yet visible
class io_context;
using io_service = io_context;
// io_context::strand exists in modern Asio; alias name used by websocketpp
class io_service::strand;
} // namespace asio
#endif
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include "Hardware/websockets/webSocketInterface.hpp"

class webSocketSimulator : public webSocketInterface {
public:
  webSocketSimulator();
  ~webSocketSimulator() override;

  void connect(const std::string &url) override;
  void disconnect() override;
  void sendMessage(const std::string &message) override;
  void setMessageCallback(MessageCallback callback) override;

private:
  using client = websocketpp::client<websocketpp::config::asio_client>;
  client wsClient;
  websocketpp::connection_hdl connectionHandle;
  std::thread wsThread;
  bool connected = false;
  MessageCallback messageCallback;
  int messageNum = 0;

  void onMessage(websocketpp::connection_hdl hdl, client::message_ptr msg);
};
