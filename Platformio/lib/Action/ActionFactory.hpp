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

  ActionFactory() = default;
  virtual ~ActionFactory() = default;

  /**
   * Given an Action Json Create the corresponding object
   */
  std::unique_ptr<IAction> createAction(const rapidjson::Value &value);

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
  /**
   *  Preconditions: aValidatedData has been validated for the given ActionType schema
   */
  std::unique_ptr<IAction> createAction(ActionTypes aActionType, const std::string &aActionName, const rapidjson::Value &aValidatedData);

  Validator mJsonValidator;
};
