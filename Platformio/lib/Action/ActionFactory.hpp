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

  std::unique_ptr<IAction> createAction(const MemConciousValue &value);

private:
  std::unique_ptr<IRAction> createIRAction(const std::string &aName, const MemConciousValue &aData);
};
