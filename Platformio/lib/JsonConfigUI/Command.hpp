#pragma once
#include "Hardware/IRInterface.h"
#include "Hardware/KeyPressAbstract.hpp"
#include "Hardware/wifi/wifiHandlerInterface.h"
#include "RapidJsonUtilty.hpp"

namespace Command {

typedef enum {
  NONE = 0,
  MQTT,
  IR
} CommandMode;

using KeyIds = KeyPressAbstract::KeyId;
using KeyPressTypes = KeyPressAbstract::KeyEvent::Type;

struct CommandStruct {
  CommandMode mode = NONE;
  std::string protocol;
  std::vector<std::string> data;
};

struct KeyStruct {
  KeyPressTypes pressType = KeyPressTypes::INVALID;
  CommandStruct command;
};

class Commands {

public:
  Commands() {};
  ~Commands() {};
  static void sendCommand(const CommandStruct &aCommandStruct);
  static CommandMode getCommand(const std::string &aCommandFIle, const std::string &aCommandPrefix, const std::string &aCommand, CommandStruct &aCommandStrings);
};
} // namespace Command