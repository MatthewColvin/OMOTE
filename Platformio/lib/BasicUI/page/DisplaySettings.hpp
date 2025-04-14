#pragma once
#include "DropDown.hpp"
#include "Hardware/DisplayAbstract.h"
#include "PageBase.hpp"

namespace UI::Widget {
class Slider;
class Label;
} // namespace UI::Widget

namespace UI::Page {
class DisplaySettings : public Base {
public:
  DisplaySettings(std::shared_ptr<DisplayAbstract> aDisplay);
  ~DisplaySettings();

  std::string GetTitle() override { return "Display Settings"; };

private:
  std::shared_ptr<DisplayAbstract> mDisplay;
  Widget::Label  *mLcdDayLabel;
  Widget::Slider *mLcdDaySlider;
  Widget::Label  *mLcdNightLabel;
  Widget::Slider *mLcdNightSlider;
  Widget::Label  *mKbdDayLabel;
  Widget::Slider *mKbdDaySlider;
  Widget::Label  *mKbdNightLabel;
  Widget::Slider *mKbdNightSlider;
};
} // namespace UI::Page
