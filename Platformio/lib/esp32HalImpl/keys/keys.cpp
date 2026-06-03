#if !defined(IS_SIMULATOR)
#include "keys.hpp"

Keys::Keys() {}

void Keys::HandleKeyPresses(const KeyEvent &aJustOccuredKeyEvent) {
  if (mKeyEventHandler) {
      mKeyEventHandler(aJustOccuredKeyEvent);
    }
}
#endif // !IS_SIMULATOR

