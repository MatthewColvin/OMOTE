#pragma once
#include <string>

#include "WidgetBase.hpp"

namespace UI::Widget {
class Label : public Base {
public:
  Label(std::string aText);

  void SetText(std::string aText);

  void SetLongMode(lv_label_long_mode_t aLongMode);

  // bind to ID
  void BindTextEvent(uint32_t key, const char *fmt);

  // bind and return ID
  uint32_t RegisterBindTextEvent(uint16_t bufSize, const char *fmt, const char *initVal);
};

} // namespace UI::Widget
