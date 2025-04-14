#include "Bar.hpp"
#include "BackgroundScreen.hpp"
#include "LvglResourceManager.hpp"

using namespace UI::Widget;

Bar::Bar(int32_t aMinVal, int32_t aMaxVal)
    : Base(lv_bar_create(UI::Screen::BackgroundScreen::getLvInstance()),
           ID::Widgets::Bar) {
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  lv_bar_set_range(LvglSelf(), aMinVal, aMaxVal);
}

int32_t Bar::GetValue() {
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  return lv_bar_get_value(LvglSelf());
}

void Bar::SetValue(int32_t aValue, lv_anim_enable_t aIsAnimate) {
  LvglResourceManager::GetInstance().AttemptNow([this, aValue, aIsAnimate] {
    lv_bar_set_value(LvglSelf(), aValue, aIsAnimate);
  });
}