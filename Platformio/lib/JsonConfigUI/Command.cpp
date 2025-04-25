#include "Command.hpp"
#include "HardwareFactory.hpp"

using namespace Command;

CommandMode Commands::getCommand(const std::string &aCommandFIle, const std::string &aCommandPrefix, const std::string &aCommand, CommandStruct &aCommandStruct) {

  File fp = HardwareFactory::getAbstract().littleFs()->open(aCommandFIle, LFS_O_RDONLY);
  if (!fp)
    return NONE;
  std::string content = fp.read(10000);

  MemConsciousDocument d;
  if (d.Parse(content.c_str()).HasParseError())
    return NONE;

  std::string fullCommand(aCommand);
  if (!aCommandPrefix.empty())
    fullCommand.insert(0, aCommandPrefix);

  std::string protocol;
  if (d.HasMember("Commands") && d["Commands"].IsArray()) {
    for (rapidjson::SizeType i = 0; i < d["Commands"].Size(); i++) {
      if (d["Commands"][i].HasMember("Command") && d["Commands"][i]["Command"].IsString()) {
        std::string command = d["Commands"][i]["Command"].GetString();
        if (command == fullCommand) {
          if (d["Commands"][i].HasMember("Mode") && d["Commands"][i]["Mode"].IsString()) {
            aCommandStruct.mode = magic_enum::enum_cast<CommandMode>(d["Commands"][i]["Mode"].GetString()).value_or(CommandMode::NONE);
            if (aCommandStruct.mode != CommandMode::NONE) {
              if (d["Commands"][i].HasMember("Protocol") && d["Commands"][i]["Protocol"].IsString()) {
                aCommandStruct.protocol = d["Commands"][i]["Protocol"].GetString();
                if (d["Commands"][i].HasMember("Data") && d["Commands"][i]["Data"].IsArray()) {
                  for (rapidjson::SizeType j = 0; j < d["Commands"][i]["Data"].Size(); j++) {
                    if (d["Commands"][i]["Data"][j].IsString())
                      aCommandStruct.data.push_back(d["Commands"][i]["Data"][j].GetString());
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return aCommandStruct.mode;
}

void Commands::sendCommand(const CommandStruct &aCommandStruct) {
  if ((aCommandStruct.mode == MQTT) && (aCommandStruct.protocol == "PUB")) {
    HardwareFactory::getAbstract().wifi()->mqttSend(aCommandStruct.data[0].c_str(), aCommandStruct.data[1].c_str());
  } else if ((aCommandStruct.mode == IR) && (aCommandStruct.data.size() > 0)) {
    HardwareFactory::getAbstract().ir()->sendBackground(aCommandStruct.protocol, aCommandStruct.data);
  }
}