#pragma once
#include "Button.hpp"
#include "Hardware/wifiHandlerInterface.h"
#include "PageBase.hpp"

namespace UI::Widget {
class List;
class Label;
class Switch;
class Keyboard;
} // namespace UI::Widget

namespace UI::Page {
enum field { broker,
             port,
             user,
             password,
             clientid };

class MqttSettings : public Base {
  using WifiInfo = wifiHandlerInterface::WifiInfo;

public:
  MqttSettings(std::shared_ptr<wifiHandlerInterface> aWifi);

  std::string GetTitle() override { return "MQTT Settings"; };

  void SetHeight(lv_coord_t aHeight) override;

protected:
  void OpenPasswordKeyboard(field aField, std::string aText);
  void Reconnect();

private:
  std::shared_ptr<wifiHandlerInterface> mWifi;

  UI::Widget::List *mList;
  UI::Widget::Keyboard *mPasswordGetter;
  UI::Widget::Button *mButton;
  UI::Widget::Label *mEnLabel;
  UI::Widget::Switch *mEnSwitch;
};

} // namespace UI::Page