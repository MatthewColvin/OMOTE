#if !defined(IS_SIMULATOR)
#include "keys.hpp"

#include <Arduino.h>
#include <magic_enum.hpp>

Keys::Keys() {}

void Keys::HandleKeyPresses(const KeyEvent &aJustOccuredKeyEvent) {
#ifdef OMOTE_DEBUG_KEYS
  if (aJustOccuredKeyEvent.mType != KeyEvent::Type::INVALID &&
      aJustOccuredKeyEvent.mId != KeyId::INVALID) {
    const auto keyName = magic_enum::enum_name(aJustOccuredKeyEvent.mId);
    const auto typeName = magic_enum::enum_name(aJustOccuredKeyEvent.mType);
    Serial.printf("keys: %.*s %.*s\n", static_cast<int>(keyName.size()), keyName.data(),
                  static_cast<int>(typeName.size()), typeName.data());
  }
#endif
  if (mKeyEventHandler)
    mKeyEventHandler(aJustOccuredKeyEvent);
}
#endif // !IS_SIMULATOR

