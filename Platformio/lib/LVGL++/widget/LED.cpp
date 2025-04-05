#include "LED.hpp"
#include "BackgroundScreen.hpp"
#include "LvglResourceManager.hpp"

using namespace UI::Widget;

LED::LED()
    : Base(lv_led_create(UI::Screen::BackgroundScreen::getLvInstance()),
           ID::Widgets::LED) {
}

void LED::SetBrightness(uint8_t aBrightness) {
  LvglResourceManager::GetInstance().AttemptNow([this, aBrightness] {
    lv_led_set_brightness(LvglSelf(), aBrightness);
  });
}

void LED::SetColor(lv_color_t aColor) {
  LvglResourceManager::GetInstance().AttemptNow([this, aColor] {
    lv_led_set_color(LvglSelf(), aColor);
  });
}

void LED::Toggle() {
  LvglResourceManager::GetInstance().AttemptNow([this] {
    if (mIsOn) {
      lv_led_off(LvglSelf());
    } else {
      lv_led_on(LvglSelf());
    }
    mIsOn = !mIsOn;
  });
}

void LED::On() {
  LvglResourceManager::GetInstance().AttemptNow([this] {
    lv_led_on(LvglSelf());
    mIsOn = true;
  });
}

void LED::Off() {
  LvglResourceManager::GetInstance().AttemptNow([this] {
    lv_led_off(LvglSelf());
    mIsOn = false;
  });
}

bool LED::IsOn() const {
  return mIsOn;
}
