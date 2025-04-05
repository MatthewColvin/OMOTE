#pragma once

#include <Hardware/LoggingInterface.hpp>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

class SimLogger : public LoggingInterface {
public:
  SimLogger() = default;
  ~SimLogger() override = default;

private:
  void log(LogLevel aLevel, LogModule aModule, std::string_view aMessage) override {
    std::stringstream ss;
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);

    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " [";

    switch (aLevel) {
    case LogLevel::DEBUG:
      ss << "DEBUG";
      break;
    case LogLevel::INFO:
      ss << "INFO";
      break;
    case LogLevel::WARNING:
      ss << "WARN";
      break;
    case LogLevel::ERROR:
      ss << "ERROR";
      break;
    case LogLevel::CRITICAL:
      ss << "CRIT";
      break;
    default:
      ss << "NONE";
      break;
    }

    ss << "][" << static_cast<int>(aModule) << "] " << aMessage << std::endl;
    std::cout << ss.str();
  }
};
