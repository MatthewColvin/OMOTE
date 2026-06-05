#include "captive_portal_sim.hpp"

#include "Hardware/captive_portal_html.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_len_t = int;
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_len_t = socklen_t;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
using SOCKET = int;
#define closesocket close
#endif

namespace {

bool gWinsockReady = false;
bool portalActive = false;
SOCKET listenFd = INVALID_SOCKET;
uint16_t listenPort = 8080;
captive_portal_sim::SaveCallback gSaveCallback;
char statusBuf[96] = "WiFi setup: http://127.0.0.1:8080";

bool ensureSockets() {
#ifdef _WIN32
  if (gWinsockReady)
    return true;
  WSADATA wsa{};
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    return false;
  gWinsockReady = true;
#endif
  return true;
}

void setNonBlocking(SOCKET fd) {
#ifdef _WIN32
  u_long mode = 1;
  ioctlsocket(fd, FIONBIO, &mode);
#else
  const int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
}

std::string urlDecode(const std::string &in) {
  std::string out;
  out.reserve(in.size());
  for (size_t i = 0; i < in.size(); ++i) {
    if (in[i] == '+') {
      out.push_back(' ');
    } else if (in[i] == '%' && i + 2 < in.size()) {
      const auto hex = in.substr(i + 1, 2);
      char *end = nullptr;
      const long v = strtol(hex.c_str(), &end, 16);
      if (end && *end == '\0') {
        out.push_back(static_cast<char>(v));
        i += 2;
      } else {
        out.push_back(in[i]);
      }
    } else {
      out.push_back(in[i]);
    }
  }
  return out;
}

bool parseFormBody(const std::string &body, std::string &ssid, std::string &password) {
  ssid.clear();
  password.clear();
  size_t pos = 0;
  while (pos < body.size()) {
    const size_t amp = body.find('&', pos);
    const std::string pair = body.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
    const size_t eq = pair.find('=');
    if (eq != std::string::npos) {
      const std::string key = urlDecode(pair.substr(0, eq));
      const std::string val = urlDecode(pair.substr(eq + 1));
      if (key == "ssid")
        ssid = val;
      else if (key == "password")
        password = val;
    }
    if (amp == std::string::npos)
      break;
    pos = amp + 1;
  }
  return !ssid.empty();
}

void sendResponse(SOCKET client, int code, const char *status, const char *contentType, const std::string &body) {
  char header[256];
  snprintf(header, sizeof(header),
           "HTTP/1.0 %d %s\r\nContent-Type: %s\r\nConnection: close\r\nContent-Length: %zu\r\n\r\n", code, status,
           contentType, body.size());
  send(client, header, static_cast<int>(strlen(header)), 0);
  if (!body.empty())
    send(client, body.c_str(), static_cast<int>(body.size()), 0);
}

void handleClient(SOCKET client) {
  std::string request;
  char buf[1024];
  for (;;) {
    const int n = recv(client, buf, sizeof(buf) - 1, 0);
    if (n <= 0)
      break;
    buf[n] = '\0';
    request.append(buf, buf + n);
    if (request.find("\r\n\r\n") != std::string::npos)
      break;
    if (request.size() > 8192)
      break;
  }

  const size_t lineEnd = request.find("\r\n");
  if (lineEnd == std::string::npos) {
    closesocket(client);
    return;
  }
  const std::string line = request.substr(0, lineEnd);
  const bool isPost = line.rfind("POST ", 0) == 0;
  const bool isGet = line.rfind("GET ", 0) == 0;
  std::string path = "/";
  if (isPost || isGet) {
    const size_t sp1 = line.find(' ');
    const size_t sp2 = line.find(' ', sp1 + 1);
    if (sp2 != std::string::npos)
      path = line.substr(sp1 + 1, sp2 - sp1 - 1);
  }

  if (isGet && (path == "/" || path.empty())) {
    sendResponse(client, 200, "OK", "text/html", captive_portal_html::page(true));
    closesocket(client);
    return;
  }

  if (isPost && path == "/save") {
    const size_t hdrEnd = request.find("\r\n\r\n");
    std::string body = hdrEnd == std::string::npos ? "" : request.substr(hdrEnd + 4);
    std::string ssid, password;
    if (parseFormBody(body, ssid, password)) {
      if (gSaveCallback)
        gSaveCallback(ssid, password);
      sendResponse(client, 200, "OK", "text/html", captive_portal_html::savedPage(true));
      closesocket(client);
      ::captive_portal_sim::stop();
      return;
    }
    sendResponse(client, 400, "Bad Request", "text/plain", "Missing ssid");
    closesocket(client);
    return;
  }

  sendResponse(client, 302, "Found", "text/plain", "");
  closesocket(client);
}

} // namespace

namespace captive_portal_sim {

void start(uint16_t port, SaveCallback onSave) {
  stop();
  if (!ensureSockets())
    return;

  listenPort = port;
  gSaveCallback = std::move(onSave);
  snprintf(statusBuf, sizeof(statusBuf), "WiFi setup: http://127.0.0.1:%u", static_cast<unsigned>(port));
  printf("[captive_portal_sim] Open %s in a browser\n", statusBuf);

  listenFd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenFd == INVALID_SOCKET)
    return;

  int yes = 1;
  setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&yes), sizeof(yes));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(port);
  if (bind(listenFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR) {
    closesocket(listenFd);
    listenFd = INVALID_SOCKET;
    return;
  }
  if (listen(listenFd, 4) == SOCKET_ERROR) {
    closesocket(listenFd);
    listenFd = INVALID_SOCKET;
    return;
  }
  setNonBlocking(listenFd);
  portalActive = true;
}

void loop() {
  if (!portalActive || listenFd == INVALID_SOCKET)
    return;

  sockaddr_in clientAddr{};
  socket_len_t len = sizeof(clientAddr);
  const SOCKET client = accept(listenFd, reinterpret_cast<sockaddr *>(&clientAddr), &len);
  if (client == INVALID_SOCKET)
    return;

  setNonBlocking(client);
  handleClient(client);
}

void stop() {
  if (listenFd != INVALID_SOCKET) {
    closesocket(listenFd);
    listenFd = INVALID_SOCKET;
  }
  portalActive = false;
}

bool isActive() { return portalActive; }

const char *statusText() { return statusBuf; }

} // namespace captive_portal_sim
