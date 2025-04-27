#include "ColorButtons.hpp"

#include "Colors.hpp"

namespace UI::Widget {

ColorButtons::ColorButtons(const std::vector<Command::CommandStruct> &aCommands)
    : Base(ID::Widgets::ColorButtons) {

  if (aCommands.size() != 4)
    return;

  SetHeight(Height);

  mRedButton = AddNewElement<Widget::Button>([this, aCommands] { Command::Commands::sendCommand(aCommands[0]); });
  mGreenButton = AddNewElement<Widget::Button>([this, aCommands] { Command::Commands::sendCommand(aCommands[1]); });
  mYellowButton = AddNewElement<Widget::Button>([this, aCommands] { Command::Commands::sendCommand(aCommands[2]); });
  mBlueButton = AddNewElement<Widget::Button>([this, aCommands] { Command::Commands::sendCommand(aCommands[3]); });

  mRedButton->SetSize(Width, Height);
  mRedButton->SetBgColor(Color::RED);
  mGreenButton->SetSize(Width, Height);
  mGreenButton->SetBgColor(Color::GREEN);
  mYellowButton->SetSize(Width, Height);
  mYellowButton->SetBgColor(Color::YELLOW);
  mBlueButton->SetSize(Width, Height);
  mBlueButton->SetBgColor(Color::BLUE);

  mRedButton->AlignTo(this, LV_ALIGN_TOP_LEFT, 10, 0);
  mGreenButton->AlignTo(mRedButton, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
  mYellowButton->AlignTo(mGreenButton, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
  mBlueButton->AlignTo(mYellowButton, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
}
} // namespace UI::Widget
