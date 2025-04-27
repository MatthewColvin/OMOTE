#pragma once

#include "Button.hpp"
#include "Command.hpp"
#include "DeviceFactory.hpp"
#include "WidgetBase.hpp"

namespace UI::Widget {

class NumberPad : public Base {
public:
  static constexpr auto Height = 180;
  static constexpr auto ButtonHeight = 30;
  static constexpr auto ButtonWidth = 60;
  static constexpr auto ButtonSpacingX = 20;
  static constexpr auto ButtonSpacingY = 15;
  NumberPad(const std::vector<Command::CommandStruct> &aCommands);

private:
  std::vector<Widget::Button *> mButtons;
};

} // namespace UI::Widget
