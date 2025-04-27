#include "keys.hpp"

Keys::Keys() {}

void Keys::HandleKeyPresses(const KeyEvent &aJustOccuredKeyEvent) {
  if (mKeyEventHandler) {
      mKeyEventHandler(aJustOccuredKeyEvent);
    }
}