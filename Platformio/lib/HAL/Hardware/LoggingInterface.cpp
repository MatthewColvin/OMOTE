#define RAPIDJSON_HAS_STDSTRING 1
#include "LoggingInterface.hpp"
#include "HardwareFactory.hpp"
#include <fstream>
#include <rapidjson/document.h>

#define LOG_SETTINGS_FILE "logSettings.json"

void LoggingInterface::restoreSettings() {
  // Default to warning for all modules before loading from file
  // so new modules default to warning
  std::fill(mCurrentLogLevels.begin(), mCurrentLogLevels.end(), LogLevel::Warning);

  std::ifstream file(FS_PATH LOG_SETTINGS_FILE, std::ios::in);
  if (!file)
    return;

  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string content(buffer.str());

  rapidjson::Document d;
  if (!d.Parse(content.c_str()).HasParseError()) {
    if (d.IsObject()) {
      for (auto &item : d.GetObject()) {
        if (item.name.IsString() && item.value.IsString()) {
          std::string moduleName = item.name.GetString();
          std::string moduleValue = item.value.GetString();
          auto module = magic_enum::enum_cast<LogModule>(moduleName);
          auto level = magic_enum::enum_cast<LogLevel>(moduleValue);
          if (module.has_value() and level.has_value()) {
            setLogLevel(module.value(), level.value());
          }
        }
      }
    }
  }
}

void LoggingInterface::saveSettings() {

  rapidjson::Document d;
  d.SetObject();

  // Add data to the JSON document
  for (auto module : magic_enum::enum_values<LogModule>()) {
    rapidjson::Value key(std::string(magic_enum::enum_name(module)), d.GetAllocator());
    rapidjson::Value val(std::string(magic_enum::enum_name(getLogLevel(module))), d.GetAllocator());
    d.AddMember(key, val, d.GetAllocator());
  }

  std::ofstream file(FS_PATH LOG_SETTINGS_FILE, std::ios::out | std::ios::trunc);
  if (!file)
    return;

  std::string jsonStr = OMOTE::JSON::ToString(d);
  file << jsonStr;
  file.close();
}

#ifndef IS_SIMULATOR
#include "Arduino.h"
void LoggingInterface::log(LogLevel aLevel, LogModule aModule, std::string_view aMessage) {
  auto module = magic_enum::enum_name(aModule);
  auto level = magic_enum::enum_name(aLevel);

  // Use ESP_LOG_LEVEL_LOCAL to bypass ESP32's log level filtering
  // since we already handled filtering in LoggingInterface::isPrintWanted
  if (aLevel != LogLevel::None) {
    Serial.print(module.data());
    Serial.print(":");
  }
  Serial.print(level.data());
  Serial.print(" ");
  Serial.println(aMessage.data());
}
#else
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
void LoggingInterface::log(LogLevel aLevel, LogModule aModule, std::string_view aMessage) {
  std::stringstream ss;
  auto now = std::time(nullptr);
  auto tm = *std::localtime(&now);

  ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " [";

  switch (aLevel) {
  case LogLevel::Debug:
    ss << "DEBUG";
    break;
  case LogLevel::Info:
    ss << "INFO";
    break;
  case LogLevel::Warning:
    ss << "WARN";
    break;
  case LogLevel::Error:
    ss << "ERROR";
    break;
  case LogLevel::Critical:
    ss << "CRIT";
    break;
  default:
    ss << "NONE";
    break;
  }

  auto moduleName = magic_enum::enum_name(aModule);
  ss << "][" << moduleName << "] " << aMessage << std::endl;
  std::cout << ss.str();
}
#endif