#include "JsonDevices/KeyAction.hpp"
#include "ActionFactory.hpp"
#include "magic_enum.hpp"

KeyAction::KeyAction(const rapidjson::Value &aKeyPressBehaviorJson) {
  ActionFactory factory;
  for (auto KeyIdEnum : magic_enum::enum_values<KeyPressAbstract::KeyEvent::Type>()) {
    auto KeyIdStr = magic_enum::enum_name(KeyIdEnum);
    if (aKeyPressBehaviorJson.HasMember(KeyIdStr.data())) {
      const auto &actionJson = aKeyPressBehaviorJson[KeyIdStr.data()];
      if (!actionJson.ObjectEmpty()) {
        auto action = factory.createAction(actionJson);
        if (action) {
          mActions[KeyIdEnum] = std::move(action);
        }
      }
    }
  }
}

bool KeyAction::isValid() const {
  return !mActions.empty();
}

void KeyAction::ExecuteAction(KeyPressAbstract::KeyEvent::Type aKeyEventType) {
  auto it = mActions.find(aKeyEventType);
  if (it != mActions.end()) {
    it->second->execute();
  }
}
