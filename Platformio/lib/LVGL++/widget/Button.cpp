#include "Button.hpp"
#include "BackgroundScreen.hpp"
#include "Colors.hpp"
#include "LvglResourceManager.hpp"

using namespace UI::Widget;

Button::Button() : Base(lv_btn_create(UI::Screen::BackgroundScreen::getLvInstance()),
                        ID::Widgets::Button) {
  SetAllPadding(2);
  SetBgOpacity(LV_OPA_COVER);
}

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
  } else if ((eventCode == LV_EVENT_CLICKED || eventCode == LV_EVENT_SHORT_CLICKED) &&
             mOnShortClick) {
    mOnShortClick();
  } else if (eventCode == LV_EVENT_LONG_PRESSED && mOnLongHold) {
    mOnLongHold();
  }
};

void Button::SetTextStyle(TextStyle aNewStyle, lv_part_t aStyle) {
  if (mText) {
    mText->SetTextStyle(aNewStyle, aStyle);
  }
  UIElement::SetTextStyle(aNewStyle, aStyle);
};

void Button::applyLabelReadabilityStyle(lv_obj_t *label) {
  // Montserrat 16 + light stroke reads bolder on the low-res panel than 12px alone.
  lv_obj_set_style_text_outline_stroke_width(label, 1, LV_PART_MAIN);
  lv_obj_set_style_text_outline_stroke_color(label, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_text_outline_stroke_opa(label, LV_OPA_70, LV_PART_MAIN);
}

void Button::layoutLabelText() {
  if (!mText)
    return;
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  lv_obj_t *label = mText->LvglSelf();
  lv_obj_set_width(label, LV_PCT(100));
  lv_obj_set_height(label, LV_SIZE_CONTENT);
  lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_DOTS);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

void Button::SetHeight(lv_coord_t aHeight) {
  UIElement::SetHeight(aHeight);
  layoutLabelText();
}

void Button::SetSize(lv_coord_t aWidth, lv_coord_t aHeight) {
  UIElement::SetSize(aWidth, aHeight);
  layoutLabelText();
}

void Button::SetText(std::string aText) {
  if (!mText) {
    mText = AddNewElement<Label>(aText);
    mText->SetTextStyle(UI::TextStyle()
                            .Color(Color::WHITE)
                            .Align(LV_TEXT_ALIGN_CENTER)
                            .Font(Color::buttonFont()));
    auto lock = LvglResourceManager::GetInstance().scopeLock();
    lv_obj_t *label = mText->LvglSelf();
    lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(label, LV_OBJ_FLAG_EVENT_BUBBLE);
    applyLabelReadabilityStyle(label);
  }
  mText->SetText(aText);
  layoutLabelText();
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