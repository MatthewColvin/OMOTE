#include "Label.hpp"

#include "BackgroundScreen.hpp"
#include "Colors.hpp"
#include "LvglResourceManager.hpp"
#include "observerHandles.hpp"

using namespace UI::Widget;

Label::Label(std::string aText)
    : Base(lv_label_create(UI::Screen::BackgroundScreen::getLvInstance()),
           ID::Widgets::Label) {
  SetText(aText);
}

void Label::SetText(std::string aText) {
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  lv_label_set_text(LvglSelf(), aText.c_str());
}

void Label::SetLongMode(lv_label_long_mode_t aLongMode) {
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  lv_label_set_long_mode(LvglSelf(), aLongMode);
}

void Label::BindTextEvent(uint32_t key, const char *fmt) {
  // lv_subject_t *obsHandle;
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  UI::observerHandles::bindLabelHandle(LvglSelf(), key, fmt);
}

uint32_t Label::RegisterBindTextEvent(uint16_t bufSize, const char *fmt, const char *initVal) {
  // lv_subject_t *obsHandle;
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  uint32_t id = UI::observerHandles::registerNextID(bufSize, initVal);
  UI::observerHandles::bindLabelHandle(LvglSelf(), id, fmt);
  return id;
}
