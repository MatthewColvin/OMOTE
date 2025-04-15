#include "Esp32Logger.hpp"
#include "Arduino.h"
#include "magic_enum.hpp"

void ESP32Logger::log(LogLevel aLevel, LogModule aModule, std::string_view aMessage) {
  auto module = magic_enum::enum_name(aModule);
  auto level = magic_enum::enum_name(aLevel);

  // Use ESP_LOG_LEVEL_LOCAL to bypass ESP32's log level filtering
  // since we already handled filtering in LoggingInterface::isPrintWanted
  if (aLevel != LogLevel::None) {
    Serial.print(module.data());
    Serial.print(":");
  }
  Serial.print(level.data());

  Serial.println(aMessage.data());
}
