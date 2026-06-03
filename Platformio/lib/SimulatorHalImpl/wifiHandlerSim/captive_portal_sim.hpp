#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace captive_portal_sim {

using SaveCallback = std::function<void(const std::string &ssid, const std::string &password)>;

void start(uint16_t port = 8080, SaveCallback onSave = {});
void loop();
void stop();
bool isActive();
const char *statusText();

} // namespace captive_portal_sim
