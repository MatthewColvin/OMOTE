#include "Esp32Logger.hpp"

void ESP32Logger::log(LogLevel aLevel, LogModule aModule, std::string_view aMessage) {
  const char *tag = getModuleTag(aModule);

  // Use ESP_LOG_LEVEL_LOCAL to bypass ESP32's log level filtering
  // since we already handled filtering in LoggingInterface::isPrintWanted
  switch (aLevel) {
  case LogLevel::DEBUG:
    ESP_LOG_LEVEL_LOCAL(ESP_LOG_DEBUG, tag, "%.*s", static_cast<int>(aMessage.length()), aMessage.data());
    break;
  case LogLevel::INFO:
    ESP_LOG_LEVEL_LOCAL(ESP_LOG_INFO, tag, "%.*s", static_cast<int>(aMessage.length()), aMessage.data());
    break;
  case LogLevel::WARNING:
    ESP_LOG_LEVEL_LOCAL(ESP_LOG_WARN, tag, "%.*s", static_cast<int>(aMessage.length()), aMessage.data());
    break;
  case LogLevel::ERROR:
    ESP_LOG_LEVEL_LOCAL(ESP_LOG_ERROR, tag, "%.*s", static_cast<int>(aMessage.length()), aMessage.data());
    break;
  case LogLevel::CRITICAL:
    ESP_LOG_LEVEL_LOCAL(ESP_LOG_ERROR, tag, "CRITICAL: %.*s", static_cast<int>(aMessage.length()), aMessage.data());
    break;
  case LogLevel::NONE:
    break;
  }
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
