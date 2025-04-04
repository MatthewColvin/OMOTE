#include "Switch.hpp"
#include "BackgroundScreen.hpp"
#include "LvglResourceManager.hpp"

using namespace UI::Widget;

Switch::Switch(std::function<void(bool)> aOnSwitchValueChange, bool aState)
    : Base(lv_switch_create(UI::Screen::BackgroundScreen::getLvInstance()),
           ID::Widgets::Switch),
      mOnSwitchChange(std::move(aOnSwitchValueChange)) {
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  if (aState)
    lv_obj_add_state(LvglSelf(), LV_STATE_CHECKED);
}

bool Switch::GetValue() {
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  return ((lv_obj_get_state(LvglSelf()) & LV_STATE_CHECKED) == LV_STATE_CHECKED);
}

void Switch::SetValue(bool aState) {
  LvglResourceManager::GetInstance().AttemptNow([this, aState] {
    if (aState)
      lv_obj_add_state(LvglSelf(), LV_STATE_CHECKED);
    else
      lv_obj_clear_state(LvglSelf(), LV_STATE_CHECKED);
  });
}

void Switch::OnLvglEvent(lv_event_t *anEvent) {
  switch (lv_event_get_code(anEvent)) {
  case LV_EVENT_VALUE_CHANGED:
    mOnSwitchChange(GetValue());
    break;
  default:
    break;
  }
}