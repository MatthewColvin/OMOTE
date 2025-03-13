#pragma once
#include "ActiveDevices.hpp"
#include "LED.hpp"
#include "Label.hpp"
#include "WidgetBase.hpp"

namespace UI::Widget {

class MainTopBar : public Base {
 public:
  static constexpr auto Height = 0.15 * SCREEN_HEIGHT;

  MainTopBar(ActiveDevices& aActiveDevices);

 private:
  void DisplayDeviceInfo(std::shared_ptr<IDevice> aDeviceToDisplay);

  ActiveDevices& mActiveDevices;
  Widget::Label* mDeviceLabel;
  Widget::LED* mLed;
};

}  // namespace UI::Widget
