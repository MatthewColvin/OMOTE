#pragma once

#include "Hardware/KeyPressAbstract.hpp"
#include "IAction.hpp"
#include "RapidJsonUtilty.hpp"
#include <map>
#include <memory>

// Responsible for mapping a Key Press EVENT to an action for a single key
class KeyAction {
public:
  KeyAction() = default;
  KeyAction(const rapidjson::Value &aKeyPressBehaviorJson);
  virtual ~KeyAction() = default;

  void ExecuteAction(KeyPressAbstract::KeyEvent::Type aKeyEventType);

  bool isValid() const;

private:
  // Map the KeyEvent to an Action
  std::map<KeyPressAbstract::KeyEvent::Type, std::unique_ptr<IAction>> mActions;
};
