#if defined(IS_SIMULATOR)

#include "HaWebSocket.hpp"

namespace HaWebSocket {

void start() {}
void tick() {}
void setSettings(const std::string &, const std::string &) {}
void subscribeEntities(const std::vector<std::string> &) {}
bool isConnected() { return false; }
bool callService(const std::string &, const std::string &, const std::string &) { return false; }
bool callServiceRest(const std::string &, const std::string &, const std::string &) { return false; }
bool callServiceRestWithData(const std::string &, const std::string &, const std::string &, const std::string &) {
  return false;
}
bool fetchEntityStateRest(const std::string &, std::string &, std::string &) { return false; }
void setStateCallback(StateCallback) {}

} // namespace HaWebSocket

#endif
