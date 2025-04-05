#pragma once
#include "Hardware/LoggingInterface.hpp"
#include <esp_log.h>

class ESP32Logger : public LoggingInterface {
public:
  ESP32Logger() = default;
  ~ESP32Logger() override = default;

private:
  void log(LogLevel aLevel, LogModule aModule, std::string_view aMessage) override;
  static const char *getModuleTag(LogModule aModule);
};
