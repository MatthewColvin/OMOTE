#pragma once

#include "Button.hpp"
#include "Command.hpp"
#include "DeviceFactory.hpp"
#include "WidgetBase.hpp"

namespace UI::Widget {

class ColorButtons : public Base {
public:
  static constexpr auto Height = 25;
  static constexpr auto Width = 40;
  ColorButtons(const std::vector<Command::CommandStruct> &aCommands);

private:
  Widget::Button *mRedButton;
  Widget::Button *mGreenButton;
  Widget::Button *mYellowButton;
  Widget::Button *mBlueButton;
};

} // namespace UI::Widget
