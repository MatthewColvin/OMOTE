#include "Esp32Logger.hpp"
#include "Arduino.h"

void ESP32Logger::log(LogLevel aLevel, LogModule aModule, std::string_view aMessage) {
  const char *tag = getModuleTag(aModule);

  // Use ESP_LOG_LEVEL_LOCAL to bypass ESP32's log level filtering
  // since we already handled filtering in LoggingInterface::isPrintWanted
  if (aLevel != LogLevel::None) {
    Serial.print(tag);
    Serial.print(":");
  }

  switch (aLevel) {
  case LogLevel::Debug:
    Serial.print("DEBUG:");
    break;
  case LogLevel::Info:
    Serial.print("INFO:");
    break;
  case LogLevel::Warning:
    Serial.print("WARNING:");
    break;
  case LogLevel::Error:
    Serial.print("ERROR:");
    break;
  case LogLevel::Critical:
    Serial.print("CRITICAL:");
    break;
  case LogLevel::None:
    return;
  }

  Serial.println(aMessage.data());
}

const char *ESP32Logger::getModuleTag(LogModule aModule) {
  switch (aModule) {
  case LogModule::WebSocket:
    return "WebSocket";
  case LogModule::Display:
    return "Display";
  case LogModule::Battery:
    return "Battery";
  case LogModule::Keys:
    return "Keys";
  case LogModule::LittleFs:
    return "LittleFs";
  case LogModule::Mqtt:
    return "Mqtt";
  default:
    return "Unknown";
  }
}
