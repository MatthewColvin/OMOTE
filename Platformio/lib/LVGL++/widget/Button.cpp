#include "Button.hpp"
#include "BackgroundScreen.hpp"

using namespace UI::Widget;

Button::Button() : Base(lv_btn_create(UI::Screen::BackgroundScreen::getLvInstance()),
                        ID::Widgets::Button) {}

Button::Button(std::function<void()> aOnPressHandler,
               std::function<void()> aOnReleaseHandler)
    : Button() {
  mOnPress = aOnPressHandler;
  mOnRelease = aOnReleaseHandler;
}

void Button::OnLvglEvent(lv_event_t *anEvent) {
  auto eventCode = lv_event_get_code(anEvent);
  if (eventCode == LV_EVENT_PRESSED && mOnPress) {
    mOnPress();
  } else if (eventCode == LV_EVENT_RELEASED && mOnRelease) {
    mOnRelease();
  } else if (eventCode == LV_EVENT_SHORT_CLICKED && mOnShortClick) {
    mOnShortClick();
  } else if (eventCode == LV_EVENT_LONG_PRESSED && mOnLongHold) {
    mOnLongHold();
  }
};

void Button::SetTextStyle(TextStyle aNewStyle, lv_style_selector_t aStyle) {
  if (mText) {
    mText->SetTextStyle(aNewStyle, aStyle);
  }
  UIElement::SetTextStyle(aNewStyle, aStyle);
};

void Button::SetText(std::string aText) {
  if (!mText) {
    mText = AddNewElement<Label>(aText);
    mText->SetTextStyle(UI::TextStyle().Align(LV_TEXT_ALIGN_CENTER));
  }
  mText->SetText(aText);
}

Button &Button::OnPress(std::function<void()> aOnPressHandler) {
  mOnPress = aOnPressHandler;
  return *this;
}

Button &Button::OnRelease(std::function<void()> aOnReleaseHandler) {
  mOnRelease = aOnReleaseHandler;
  return *this;
}

Button &Button::OnShortClick(std::function<void()> aOnShortClickHandler) {
  mOnShortClick = aOnShortClickHandler;
  return *this;
}

Button &Button::OnLongHold(std::function<void()> aOnLongClickHandler) {
  mOnLongHold = aOnLongClickHandler;
  return *this;
}