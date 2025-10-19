#include "ActionFactory.hpp"
#include "HardwareFactory.hpp"
#include <filesystem>
#include <fstream>

std::unique_ptr<IAction> ActionFactory::createAction(const MemConciousValue &value) {
  if (!value.HasMember("type") || !value.HasMember("name") || !value.HasMember("data"))
    return nullptr;

  const std::string type = value["type"].GetString();
  const std::string name = value["name"].GetString();
  const auto &data = value["data"];

  if (type == "IRAction") {
    return createIRAction(name, data);
  }

  return nullptr;
}

std::unique_ptr<IRAction> ActionFactory::createIRAction(const std::string &aName,
                                                        const MemConciousValue &aData) {
  if (!aData.HasMember("protocol") || !aData.HasMember("data")) {
    return nullptr;
  }

  const std::string protocol = aData["protocol"].GetString();
  const std::string hexData = aData["data"].GetString();

  return std::make_unique<IRAction>(aName, protocol, hexData);
}

std::unique_ptr<IAction> ActionFactory::createAction(const std::string &aActionName) {
  for (auto const &dir_entry : std::filesystem::directory_iterator{ActionsDirectory}) {
    if (dir_entry.is_regular_file()) {
      std::string fullPath(FS_PATH);
      fullPath += dir_entry.path().string();
      std::ifstream file(fullPath, std::ios::in);
      if (file) {
        MemConsciousDocument actionDoc;
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        std::string actionJsonStr(buffer.str());
        actionDoc.Parse(actionJsonStr.c_str());
        if (actionDoc.HasMember("name") && actionDoc.IsString()) {
          return createAction(actionDoc);
        }
      }
    }
  }
  return nullptr;
}

std::vector<std::unique_ptr<IAction>> ActionFactory::getAllActions() {
  std::vector<std::unique_ptr<IAction>> actions;

  for (auto const &dir_entry : std::filesystem::directory_iterator{ActionsDirectory}) {
    if (dir_entry.is_regular_file()) {
      std::string fullPath(FS_PATH);
      fullPath += dir_entry.path().string();
      std::ifstream file(fullPath, std::ios::in);
      if (file) {
        MemConsciousDocument actionDoc;
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        std::string actionJsonStr(buffer.str());
        actionDoc.Parse(actionJsonStr.c_str());
        if (actionDoc.HasMember("name") && actionDoc["name"].IsString()) {
          auto action = createAction(actionDoc);
          if (action) {
            actions.push_back(std::move(action));
          }
        }
      }
    }
  }
  return actions;
}
