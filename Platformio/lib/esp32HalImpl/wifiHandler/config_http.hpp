#pragma once

namespace config_http {
void begin(const char *mdnsName);
void sync();
void stop();
bool isRunning();
/** True for a short window after any config HTTP API request (editor connect/load). */
bool isRemoteSessionActive();
} // namespace config_http
