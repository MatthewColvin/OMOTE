#if !defined(IS_SIMULATOR)
#include "Esp32Logger.hpp"
#include "Arduino.h"
#include "magic_enum.hpp"

void ESP32Logger::log(LogLevel aLevel, LogModule aModule, std::string_view aMessage) {
  auto module = magic_enum::enum_name(aModule);
  auto level = magic_enum::enum_name(aLevel);

  Serial.print(module.data());
  Serial.print(":");
  Serial.print(level.data());
  Serial.print(" ");
  Serial.println(aMessage.data());
}
#endif // !IS_SIMULATOR

