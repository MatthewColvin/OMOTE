#include "HardwareAbstract.hpp"
#include "PageBase.hpp"

namespace UI::Widget {
class Button;
class List;
} // namespace UI::Widget
namespace UI::Page {
class SettingsPage : public Base {
public:
  SettingsPage();

  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override {
    return false;
  };

  std::string GetTitle() override { return "Settings"; };

  void PushDisplaySettings();
  void PushSystemSettings();
  void PushWifiSettings();
  void PushMqttSettings();
  void PushIrReader();
  void PushLoggingSettings();

protected:
  void OnShow() override {};
  void OnHide() override {};

  Widget::Button *mButton;
  Widget::List *mSettingsList;
};
} // namespace UI::Page
