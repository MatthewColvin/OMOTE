#pragma once
#include "IAction.hpp"
#include "IRAction.hpp"
#include "RapidJsonUtilty.hpp"
#include <memory>
#include <string>
#include <vector>

class ActionFactory {
public:
  ActionFactory() = default;
  virtual ~ActionFactory() = default;

  /**
   * Given an Action Json Create the corresponding object
   */
  std::unique_ptr<IAction> createAction(const MemConciousValue &value);

  /**
   * Given a name of an action look through the Actions directory to
   * try and find and create it.
   */
  std::unique_ptr<IAction> createAction(const std::string &aActionName);

private:
  std::unique_ptr<IRAction> createIRAction(const std::string &aName, const MemConciousValue &aData);
};
