#include "KeyPressSim.hpp"

#include <memory>

KeyPressSim::KeyPressSim() : mSDLEventNotification(std::make_shared<Notification<SDL_Event *>>()) {
  SDL_AddEventWatch(KeyPressSim::GrabKeyImpl, this);
}

int KeyPressSim::GrabKeyImpl(void *aSelf, SDL_Event *aEvent) {
  reinterpret_cast<KeyPressSim *>(aSelf)->GrabKeys(aEvent);
  return 0;
}

void KeyPressSim::GrabKeys(SDL_Event *aEvent) {
  // Not running in LVGL thread so not safe to issue events from here
  // instead add to queue to transfer to LVGL thread
  mSDLEventNotification->notify(aEvent);
  if (aEvent->type == SDL_KEYDOWN || aEvent->type == SDL_KEYUP) {
    auto keyEventType = aEvent->type == SDL_KEYDOWN ? KeyEvent::Type::Press
                                                    : KeyEvent::Type::Release;
    const auto SDLK_key = aEvent->key.keysym.sym;
    if (KeyMap.count(SDLK_key) > 0) {
      std::lock_guard lock(mQueueGaurd);
      mKeyEventQueue.push(KeyEvent(KeyMap.at(SDLK_key), keyEventType));
      mProcessKeyQueueCondition.notify_one();
    }
  }
}

// this is repeatedly called from main LVGL thread so safe to send events from here
void KeyPressSim::KeyboardScan() {
  std::unique_lock lock(mQueueGaurd);
  if (!mKeyEventQueue.empty()) {
    HandleKeyPresses(mKeyEventQueue.front());
    mKeyEventQueue.pop();
  }
}

void KeyPressSim::HandleKeyPresses(const KeyEvent &aJustOccuredKeyEvent) {
  if (mKeyEventHandler) {
    mKeyEventHandler(aJustOccuredKeyEvent);
  }
}
