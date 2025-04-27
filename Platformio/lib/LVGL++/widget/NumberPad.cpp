#include "NumberPad.hpp"

#include "Colors.hpp"

namespace UI::Widget {

NumberPad::NumberPad(const std::vector<Command::CommandStruct> &aCommands)
    : Base(ID::Widgets::ColorButtons) {

  if (aCommands.size() != 10)
    return;

  SetHeight(Height);

  auto button = AddNewElement<Widget::Button>([this, aCommands] { Command::Commands::sendCommand(aCommands[0]); });
  button->SetSize(ButtonWidth, ButtonHeight);
  button->SetX(((10 % 3) * (ButtonWidth + ButtonSpacingX)) + (ButtonSpacingX / 2));
  button->SetY(((10 / 3) * (ButtonHeight + ButtonSpacingY)) + (ButtonSpacingY / 2));
  button->SetBgColor(Color::GREY);
  button->SetText("0");
  mButtons.push_back(button);

  for (uint16_t i = 1; i < 10; i++) {
    auto button = AddNewElement<Widget::Button>([this, aCommands, i] { Command::Commands::sendCommand(aCommands[i]); });
    button->SetSize(ButtonWidth, ButtonHeight);
    button->SetX((((i - 1) % 3) * (ButtonWidth + ButtonSpacingX)) + (ButtonSpacingX / 2));
    button->SetY((((i - 1) / 3) * (ButtonHeight + ButtonSpacingY)) + (ButtonSpacingY / 2));
    button->SetBgColor(Color::GREY);
    button->SetText(std::to_string(i));
    mButtons.push_back(button);
  }
}
} // namespace UI::Widget
