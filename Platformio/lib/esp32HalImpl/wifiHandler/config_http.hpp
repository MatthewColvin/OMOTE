#pragma once

namespace config_http {
void begin(const char *mdnsName);
void sync();
void stop();
bool isRunning();
} // namespace config_http
