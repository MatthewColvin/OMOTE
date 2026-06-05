#pragma once

#include <functional>
#include <string>
#include <vector>

namespace HaWebSocket {

using StateCallback =
    std::function<void(const std::string &entityId, const std::string &state, const std::string &attributesJson)>;

void start();
void tick();
void setSettings(const std::string &url, const std::string &token);
void subscribeEntities(const std::vector<std::string> &entityIds);

bool isConnected();
/** Queue call_service on WebSocket (no-op if queue full). */
bool callService(const std::string &domain, const std::string &service, const std::string &entityId);
/** Synchronous REST call_service — used for touchscreen taps. */
bool callServiceRest(const std::string &domain, const std::string &service, const std::string &entityId);
bool callServiceRestWithData(const std::string &domain, const std::string &service, const std::string &entityId,
                             const std::string &serviceDataJson);
bool fetchEntityStateRest(const std::string &entityId, std::string &stateOut, std::string &attributesJsonOut);

void setStateCallback(StateCallback cb);

} // namespace HaWebSocket
