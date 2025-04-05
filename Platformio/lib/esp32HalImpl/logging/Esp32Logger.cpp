#include "Esp32Logger.hpp"
#include "Arduino.h"

void ESP32Logger::log(LogLevel aLevel, LogModule aModule, std::string_view aMessage) {
  const char *tag = getModuleTag(aModule);

  // Use ESP_LOG_LEVEL_LOCAL to bypass ESP32's log level filtering
  // since we already handled filtering in LoggingInterface::isPrintWanted
  if (aLevel != LogLevel::NONE) {
    Serial.print(tag);
    Serial.print(":");
  }

  switch (aLevel) {
  case LogLevel::DEBUG:
    Serial.print("DEBUG:");
    break;
  case LogLevel::INFO:
    Serial.print("INFO:");
    break;
  case LogLevel::WARNING:
    Serial.print("WARNING:");
    break;
  case LogLevel::ERROR:
    Serial.print("ERROR:");
    break;
  case LogLevel::CRITICAL:
    Serial.print("CRITICAL:");
    break;
  case LogLevel::NONE:
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
