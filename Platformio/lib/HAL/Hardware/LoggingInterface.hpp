#pragma once

#include <array>
#include <string>
#include <string_view>

enum class LogLevel {
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  CRITICAL,
  NONE
};

enum class LogModule : uint8_t {
  WebSocket,
  Display,
  Battery,
  Keys,
  LittleFs,
  Mqtt,
  // Add more Log Modules here
  Count
};

static constexpr auto NumLogModules = static_cast<uint8_t>(LogModule::Count);

class LoggingInterface {
public:
  virtual ~LoggingInterface() = default;
  LoggingInterface();

  void setLogLevel(LogModule aModule, LogLevel aLevel);
  LogLevel getLogLevel(LogModule aModule) const;

  virtual void debug(LogModule aModule, std::string_view aMessage);
  virtual void info(LogModule aModule, std::string_view aMessage);
  virtual void warning(LogModule aModule, std::string_view aMessage);
  virtual void error(LogModule aModule, std::string_view aMessage);
  virtual void critical(LogModule aModule, std::string_view aMessage);

private:
  bool isPrintWanted(LogModule aModule, LogLevel aLevelToCheck);

  virtual void log(LogLevel aLevel, LogModule aModule, std::string_view aMessage) = 0;

private:
  std::array<LogLevel, NumLogModules> mCurrentLogLevels;
};

inline LoggingInterface::LoggingInterface() {
  // Default all logs to critical to start
  std::fill(mCurrentLogLevels.begin(), mCurrentLogLevels.end(), LogLevel::CRITICAL);
}

inline void LoggingInterface::setLogLevel(LogModule aModule, LogLevel aLevel) {
  auto moduleIndex = static_cast<uint8_t>(aModule);
  mCurrentLogLevels[moduleIndex] = aLevel;
}

inline LogLevel LoggingInterface::getLogLevel(LogModule aModule) const {
  auto moduleIndex = static_cast<uint8_t>(aModule);
  return mCurrentLogLevels[moduleIndex];
}

inline bool LoggingInterface::isPrintWanted(LogModule aModule, LogLevel aLevelToCheck) {
  auto moduleIndex = static_cast<uint8_t>(aModule);
  auto currentLevel = mCurrentLogLevels[moduleIndex];
  return static_cast<int>(aLevelToCheck) >= static_cast<int>(currentLevel);
}

inline void LoggingInterface::debug(LogModule aModule, std::string_view aMessage) {
  if (isPrintWanted(aModule, LogLevel::DEBUG)) {
    log(LogLevel::DEBUG, aModule, aMessage);
  }
}

inline void LoggingInterface::info(LogModule aModule, std::string_view aMessage) {
  if (isPrintWanted(aModule, LogLevel::INFO)) {
    log(LogLevel::INFO, aModule, aMessage);
  }
}

inline void LoggingInterface::warning(LogModule aModule, std::string_view aMessage) {
  if (isPrintWanted(aModule, LogLevel::WARNING)) {
    log(LogLevel::WARNING, aModule, aMessage);
  }
}

inline void LoggingInterface::error(LogModule aModule, std::string_view aMessage) {
  if (isPrintWanted(aModule, LogLevel::ERROR)) {
    log(LogLevel::ERROR, aModule, aMessage);
  }
}

inline void LoggingInterface::critical(LogModule aModule, std::string_view aMessage) {
  if (isPrintWanted(aModule, LogLevel::CRITICAL)) {
    log(LogLevel::CRITICAL, aModule, aMessage);
  }
}
