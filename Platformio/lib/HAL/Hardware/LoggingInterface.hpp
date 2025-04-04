#pragma once

#include <array>
#include <experimental/array>
#include <string>
#include <string_view>

enum class LogLevel {
  NONE,
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  CRITICAL
};

enum class LogModule : uint8_t {
  General,
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
  LoggingInterface() = default;

  void debug(std::string_view aMessage);
  void info(std::string_view aMessage);
  void warning(std::string_view aMessage);
  void error(std::string_view aMessage);
  void critical(std::string_view aMessage);

  void setLogLevel(LogLevel aLevel);
  LogLevel getLogLevel() const;

  void setLogModule(LogModule aModule);
  LogModule getLogModule() const;

protected:
  virtual void log(LogLevel aLevel, LogModule aModule, std::string_view aMessage) = 0;

private:
  LogModule mModule = LogModule::General;

  // Check level and module to see if we want to print the log
  static bool isPrintWanted(LogModule aModule, LogLevel aLevelToCheck);
  // Default to first value in LogLevel enum
  static inline std::array<LogLevel, NumLogModules> mCurrentLogLevels{};
};

inline void LoggingInterface::setLogModule(LogModule aModule) {
  mModule = aModule;
}

inline LogModule LoggingInterface::getLogModule() const {
  return mModule;
}

inline void LoggingInterface::setLogLevel(LogLevel aLevel) {
  auto moduleIndex = static_cast<uint8_t>(mModule);
  mCurrentLogLevels[moduleIndex] = aLevel;
}

inline LogLevel LoggingInterface::getLogLevel() const {
  auto moduleIndex = static_cast<uint8_t>(mModule);
  return mCurrentLogLevels[moduleIndex];
}

inline bool LoggingInterface::isPrintWanted(LogModule aModule, LogLevel aLevelToCheck) {
  auto moduleIndex = static_cast<uint8_t>(aModule);
  auto currentLevel = mCurrentLogLevels[moduleIndex];
  return static_cast<int>(aLevelToCheck) >= static_cast<int>(currentLevel);
}

inline void LoggingInterface::debug(std::string_view aMessage) {
  if (isPrintWanted(mModule, LogLevel::DEBUG)) {
    log(LogLevel::DEBUG, mModule, aMessage);
  }
}

inline void LoggingInterface::info(std::string_view aMessage) {
  if (isPrintWanted(mModule, LogLevel::INFO)) {
    log(LogLevel::INFO, mModule, aMessage);
  }
}

inline void LoggingInterface::warning(std::string_view aMessage) {
  if (isPrintWanted(mModule, LogLevel::WARNING)) {
    log(LogLevel::WARNING, mModule, aMessage);
  }
}

inline void LoggingInterface::error(std::string_view aMessage) {
  if (isPrintWanted(mModule, LogLevel::ERROR)) {
    log(LogLevel::ERROR, mModule, aMessage);
  }
}

inline void LoggingInterface::critical(std::string_view aMessage) {
  if (isPrintWanted(mModule, LogLevel::CRITICAL)) {
    log(LogLevel::CRITICAL, mModule, aMessage);
  }
}
