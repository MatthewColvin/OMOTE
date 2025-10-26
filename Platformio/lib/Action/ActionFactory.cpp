#include "ActionFactory.hpp"
#include "ActionTypes.hpp"
#include "HardwareFactory.hpp"
#include "magic_enum.hpp"
#include <filesystem>
#include <fstream>

std::unique_ptr<IAction> ActionFactory::createAction(const rapidjson::Value &value) {
  if (!mJsonValidator.IsAction(value)) {
    return nullptr;
  }

  const std::string type = value["type"].GetString();
  const std::string name = value["name"].GetString();
  const auto &data = value["data"];

  auto actionType = magic_enum::enum_cast<ActionTypes>(type).value_or(ActionTypes::COUNT);
  if (actionType == ActionTypes::COUNT || !mJsonValidator.IsDataValid(actionType, data)) {
    return nullptr;
  }

  return createAction(actionType, name, data);
}

std::unique_ptr<IAction> ActionFactory::createAction(ActionTypes aActionType, const std::string &aActionName, const rapidjson::Value &aValidatedData) {
  switch (aActionType) {
  case ActionTypes::IRAction:
    std::make_unique<IRAction>(aActionName, aValidatedData["protocol"].GetString(), aValidatedData["data"].GetString());
  default:
    return nullptr;
  }
}

std::unique_ptr<IAction> ActionFactory::createAction(const std::string &aActionName) {
  for (auto const &dir_entry : std::filesystem::directory_iterator{ActionsDirectory}) {
    if (dir_entry.is_regular_file()) {
      std::string fullPath(FS_PATH);
      fullPath += dir_entry.path().string();
      std::ifstream file(fullPath, std::ios::in);
      if (file) {
        rapidjson::Document actionDoc;
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
    if (!dir_entry.is_regular_file()) {
      continue;
    }
    auto actionDoc = GetDocument(dir_entry.path());
    auto action = createAction(actionDoc);
    if (action) {
      actions.push_back(std::move(action));
    }
  }
  return actions;
}
