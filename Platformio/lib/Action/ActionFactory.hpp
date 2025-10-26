#pragma once
#include "ActionTypes.hpp"
#include "IAction.hpp"
#include "IRAction.hpp"
#include "RapidJsonUtilty.hpp"
#include "Validator.hpp"
#include <memory>
#include <string>
#include <vector>

class ActionFactory {
public:
  static constexpr auto ActionsDirectory = FS_PATH "Actions";
  static constexpr auto MaxActionFileLength = 500;
  using ActionCreator = std::function<std::unique_ptr<IAction>(const std::string &aName, const rapidjson::Value &aDataJson)>;

  /**
   * Register an Action Type with its schema and creator function
   * aSchemaJson - JSON Schema for validating action data portion of action json
   * aCreator - Function that creates the action given name and data json
   *            ** should assume the data has been validated via the aSchemaJson **
   */
  static bool Register(ActionTypes aActionType, std::string_view aSchemaJson, ActionCreator aCreator);

  ActionFactory() = default;
  virtual ~ActionFactory() = default;

  /**
   * Given an Action Json Create the corresponding object
   */
  std::unique_ptr<IAction> createAction(const rapidjson::Value &aProbableActionJson);

  /**
   * Given a name of an action look through the Actions directory to
   * try and find and create it.
   */
  std::unique_ptr<IAction> createAction(const std::string &aActionName);

  /**
   * Create a vector of actions defined in Actions directory
   */
  std::vector<std::unique_ptr<IAction>> getAllActions();

private:
  Validator mJsonValidator;

  static inline std::array<ActionCreator, static_cast<uint16_t>(ActionTypes::COUNT)> mActionCreators;
};

inline bool ActionFactory::Register(ActionTypes aActionType, std::string_view aSchemaJson, ActionCreator aCreator) {
  mActionCreators[static_cast<uint16_t>(aActionType)] = aCreator;
  return Validator::Register(aActionType, aSchemaJson);
}
