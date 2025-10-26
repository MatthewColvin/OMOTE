#include "ActionFactory.hpp"
#include "ActionTypes.hpp"
#include "HardwareFactory.hpp"
#include "magic_enum.hpp"
#include <filesystem>
#include <fstream>

// TODO: Add Error tracking via stateful enum since we do not have std::expected in C++20
std::unique_ptr<IAction> ActionFactory::createAction(const rapidjson::Value &aProbableActionJson) {
  if (!mJsonValidator.IsAction(aProbableActionJson)) {
    mLastError = CreationError::InvalidActionJson;
    return nullptr;
  }
  auto &actionJson = aProbableActionJson;
  const std::string type = actionJson["type"].GetString();
  const std::string name = actionJson["name"].GetString();
  const auto &data = actionJson["data"];

  auto actionType = magic_enum::enum_cast<ActionTypes>(type).value_or(ActionTypes::COUNT);
  if (actionType == ActionTypes::COUNT) {
    mLastError = CreationError::UnknownActionType;
    return nullptr;
  }
  if (!mJsonValidator.IsDataValid(actionType, data)) {
    mLastError = CreationError::InvalidActionData;
    return nullptr;
  }
  auto &actionCreator = mActionCreators[static_cast<uint16_t>(actionType)];
  if (!actionCreator) {
    mLastError = CreationError::NoCreatorRegistered;
    return nullptr;
  }
  auto action = actionCreator(name, data);
  if (!action) {
    mLastError = CreationError::ActionCreationFailed;
    return nullptr;
  }

  return action;
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
