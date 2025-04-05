#pragma once

#include <array>
#include <experimental/array>
#include <sstream>
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

/**
 * This class is meant to be used as a way to log.
 *
 * The log level is set statically and there for is a global config
 * for the whole program on a per module basis.
 *
 * The Interface provdes a set of functions for logging string and
 * stringstream.
 *
 * The string logging will return a bool to allow for checking if
 * logging was done so the extra string stream processing can be
 * skipped. The stringstream is cleared after the logging for easy reuse.
 *
 * Example:
 * std::unique_ptr<LoggingInterface> aLogger;
 * if(aLogger && aLogger->debug("Debug message")) {
 *   stringstream ss;
 *   int i = 42;
 *   ss << "Debug message with extra info" << i << std::endl;
 *   aLogger->debug(ss);
 * }
 *
 */
class LoggingInterface {
public:
  // Global setters to change log level per module
  static void setLogLevel(LogModule aModule, LogLevel aLevel);
  static LogLevel getLogLevel(LogModule aModule);

  virtual ~LoggingInterface() = default;
  LoggingInterface() = default;

  // Return True if the message was logged
  bool debug(std::string_view aMessage);
  bool info(std::string_view aMessage);
  bool warning(std::string_view aMessage);
  bool error(std::string_view aMessage);
  bool critical(std::string_view aMessage);

  // Take Stream and print it and then clear stream for next print
  void debug(std::stringstream &aMessageStream);
  void info(std::stringstream &aMessageStream);
  void warning(std::stringstream &aMessageStream);
  void error(std::stringstream &aMessageStream);
  void critical(std::stringstream &aMessageStream);

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

inline void LoggingInterface::setLogLevel(LogModule aModule, LogLevel aLevel) {
  auto moduleIndex = static_cast<uint8_t>(aModule);
  mCurrentLogLevels[moduleIndex] = aLevel;
}

inline LogLevel LoggingInterface::getLogLevel(LogModule aModule) {
  auto moduleIndex = static_cast<uint8_t>(aModule);
  return mCurrentLogLevels[moduleIndex];
}

inline void LoggingInterface::setLogLevel(LogLevel aLevel) {
  setLogLevel(mModule, aLevel);
}

inline LogLevel LoggingInterface::getLogLevel() const {
  return getLogLevel(mModule);
}

inline bool LoggingInterface::isPrintWanted(LogModule aModule, LogLevel aLevelToCheck) {
  auto moduleIndex = static_cast<uint8_t>(aModule);
  auto currentLevel = mCurrentLogLevels[moduleIndex];
  if (currentLevel == LogLevel::NONE) {
    return false;
  }
  return static_cast<int>(aLevelToCheck) >= static_cast<int>(currentLevel);
}

inline bool LoggingInterface::debug(std::string_view aMessage) {
  if (isPrintWanted(mModule, LogLevel::DEBUG)) {
    log(LogLevel::DEBUG, mModule, aMessage);
    return true;
  }
  return false;
}

inline bool LoggingInterface::info(std::string_view aMessage) {
  if (isPrintWanted(mModule, LogLevel::INFO)) {
    log(LogLevel::INFO, mModule, aMessage);
    return true;
  }
  return false;
}

inline bool LoggingInterface::warning(std::string_view aMessage) {
  if (isPrintWanted(mModule, LogLevel::WARNING)) {
    log(LogLevel::WARNING, mModule, aMessage);
    return true;
  }
  return false;
}

inline bool LoggingInterface::error(std::string_view aMessage) {
  if (isPrintWanted(mModule, LogLevel::ERROR)) {
    log(LogLevel::ERROR, mModule, aMessage);
    return true;
  }
  return false;
}

inline bool LoggingInterface::critical(std::string_view aMessage) {
  if (isPrintWanted(mModule, LogLevel::CRITICAL)) {
    log(LogLevel::CRITICAL, mModule, aMessage);
    return true;
  }
  return false;
}

inline void LoggingInterface::debug(std::stringstream &aMessageStream) {
  if (isPrintWanted(mModule, LogLevel::DEBUG)) {
    log(LogLevel::DEBUG, mModule, aMessageStream.str().c_str());
  }
  aMessageStream.clear();
}
inline void LoggingInterface::info(std::stringstream &aMessageStream) {
  if (isPrintWanted(mModule, LogLevel::INFO)) {
    log(LogLevel::INFO, mModule, aMessageStream.str().c_str());
  }
  aMessageStream.clear();
}

inline void LoggingInterface::warning(std::stringstream &aMessageStream) {
  if (isPrintWanted(mModule, LogLevel::WARNING)) {
    log(LogLevel::WARNING, mModule, aMessageStream.str().c_str());
  }
  aMessageStream.clear();
}
inline void LoggingInterface::error(std::stringstream &aMessageStream) {
  if (isPrintWanted(mModule, LogLevel::ERROR)) {
    log(LogLevel::ERROR, mModule, aMessageStream.str().c_str());
  }
  aMessageStream.clear();
}
inline void LoggingInterface::critical(std::stringstream &aMessageStream) {
  if (isPrintWanted(mModule, LogLevel::CRITICAL)) {
    log(LogLevel::CRITICAL, mModule, aMessageStream.str().c_str());
  }
  aMessageStream.clear();
}
