#pragma once
#include "Hardware/wifi/wifiHandlerInterface.h"
#include "PageBase.hpp"
namespace UI::Widget {
class Label;
class Button;
} // namespace UI::Widget

namespace UI::Page {

class Heating : public Base {
  using WifiInfo = wifiHandlerInterface::WifiInfo;

public:
  Heating(std::shared_ptr<wifiHandlerInterface> aWifi);
  virtual ~Heating();

  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override;

private:
  std::shared_ptr<wifiHandlerInterface> mWifi;

  void Boost();
  void Advance();
  void Up();
  void Down();

  Widget::Label *mCurrentTemperature;
  Widget::Label *mSetpoint;
  static constexpr auto distBetweenRxButtons = 5;
  Widget::Button *mAdvance;
  Widget::Button *mBoost;
  Widget::Button *mUp;
  Widget::Button *mDown;

  uint32_t mTemperatureId = 0;
  uint32_t mSetpointId = 0;
};

} // namespace UI::Page
