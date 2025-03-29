#pragma once
#include "PageBase.hpp"
#include "Hardware/wifiHandlerInterface.h"

namespace UI::Widget {
class List;
class Label;
class Keyboard;
} // namespace UI::Widget

namespace UI::Page {
enum field {broker, port, user, password};

class MqttSettings : public Base {
using WifiInfo = wifiHandlerInterface::WifiInfo;
public:
  MqttSettings(std::shared_ptr<wifiHandlerInterface> aWifi);

  std::string GetTitle() override { return "MQTT Settings"; };

  void SetHeight(lv_coord_t aHeight) override;

protected:
  
  void OpenPasswordKeyboard(field aField, std::string aText);

private:

  std::shared_ptr<wifiHandlerInterface> mWifi;
  
  UI::Widget::List *mList;
  UI::Widget::Keyboard *mPasswordGetter;
};

} // namespace UI::Page