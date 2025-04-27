#pragma once
#include "HardwareAbstract.hpp"
#include "PageBase.hpp"

namespace UI::Widget {
class Button;
class List;
} // namespace UI::Widget
namespace UI::Page {
class SettingsPage : public Base {
public:
  using InjectedItem = std::tuple<std::string, const char *,
                                  std::function<Base::Ptr()>>;

  SettingsPage();

  /**
   * Add item to settings aPageGetter should return a page
   * to launch when pressed in settings
   */
  void AddSettingItem(std::string aTitle, const char *aSymbol,
                      std::function<Base::Ptr()> aPageGetter);

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
  const lv_coord_t mHeight = 45;
};
} // namespace UI::Page
