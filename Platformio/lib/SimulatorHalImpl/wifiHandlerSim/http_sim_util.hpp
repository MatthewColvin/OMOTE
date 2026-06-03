#pragma once

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>

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

namespace http_sim {

inline bool ensureSockets() {
#ifdef _WIN32
  static bool ready = false;
  if (ready)
    return true;
  WSADATA wsa{};
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    return false;
  ready = true;
#endif
  return true;
}

inline void setNonBlocking(SOCKET fd) {
#ifdef _WIN32
  u_long mode = 1;
  ioctlsocket(fd, FIONBIO, &mode);
#else
  const int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
}

inline std::string urlDecode(const std::string &in) {
  std::string out;
  out.reserve(in.size());
  for (size_t i = 0; i < in.size(); ++i) {
    if (in[i] == '+')
      out.push_back(' ');
    else if (in[i] == '%' && i + 2 < in.size()) {
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

inline std::map<std::string, std::string> parseQuery(const std::string &query) {
  std::map<std::string, std::string> out;
  size_t pos = 0;
  while (pos < query.size()) {
    const size_t amp = query.find('&', pos);
    const std::string pair = query.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
    const size_t eq = pair.find('=');
    if (eq != std::string::npos)
      out[urlDecode(pair.substr(0, eq))] = urlDecode(pair.substr(eq + 1));
    if (amp == std::string::npos)
      break;
    pos = amp + 1;
  }
  return out;
}

inline void sendResponse(SOCKET client, int code, const char *status, const char *contentType, const std::string &body) {
  char header[320];
  snprintf(header, sizeof(header),
           "HTTP/1.0 %d %s\r\nContent-Type: %s\r\nAccess-Control-Allow-Origin: *\r\n"
           "Access-Control-Allow-Methods: GET, POST, PUT, OPTIONS\r\n"
           "Access-Control-Allow-Headers: Content-Type\r\nConnection: close\r\nContent-Length: %zu\r\n\r\n",
           code, status, contentType, body.size());
  send(client, header, static_cast<int>(strlen(header)), 0);
  if (!body.empty())
    send(client, body.c_str(), static_cast<int>(body.size()), 0);
}

inline size_t headerContentLength(const std::string &headers) {
  const std::string key = "content-length:";
  size_t pos = 0;
  while (pos < headers.size()) {
    const size_t lineEnd = headers.find("\r\n", pos);
    const std::string line = headers.substr(pos, lineEnd == std::string::npos ? std::string::npos : lineEnd - pos);
    if (line.size() >= key.size()) {
      std::string lower = line;
      for (char &c : lower)
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
      if (lower.rfind(key, 0) == 0) {
        const char *digits = line.c_str() + key.size();
        while (*digits == ' ' || *digits == '\t')
          ++digits;
        return static_cast<size_t>(strtoul(digits, nullptr, 10));
      }
    }
    if (lineEnd == std::string::npos)
      break;
    pos = lineEnd + 2;
  }
  return 0;
}

inline bool recvRequest(SOCKET client, std::string &method, std::string &path, std::map<std::string, std::string> &query,
                        std::string &body) {
  method.clear();
  path.clear();
  query.clear();
  body.clear();
  std::string request;
  char buf[8192];
  const size_t kMaxRequest = 8 * 1024 * 1024;
  while (request.size() < kMaxRequest) {
    const int n = recv(client, buf, sizeof(buf), 0);
    if (n <= 0)
      break;
    request.append(buf, buf + n);
    const size_t hdrEnd = request.find("\r\n\r\n");
    if (hdrEnd == std::string::npos)
      continue;
    const size_t contentLen = headerContentLength(request.substr(0, hdrEnd));
    const size_t haveBody = request.size() - (hdrEnd + 4);
    if (contentLen == 0 || haveBody >= contentLen)
      break;
  }
  const size_t lineEnd = request.find("\r\n");
  if (lineEnd == std::string::npos)
    return false;
  const std::string line = request.substr(0, lineEnd);
  const size_t sp1 = line.find(' ');
  const size_t sp2 = line.find(' ', sp1 + 1);
  if (sp1 == std::string::npos || sp2 == std::string::npos)
    return false;
  method = line.substr(0, sp1);
  std::string target = line.substr(sp1 + 1, sp2 - sp1 - 1);
  const size_t qm = target.find('?');
  if (qm != std::string::npos) {
    path = target.substr(0, qm);
    query = parseQuery(target.substr(qm + 1));
  } else {
    path = target;
  }
  const size_t hdrEnd = request.find("\r\n\r\n");
  if (hdrEnd != std::string::npos)
    body = request.substr(hdrEnd + 4);
  return true;
}

} // namespace http_sim
