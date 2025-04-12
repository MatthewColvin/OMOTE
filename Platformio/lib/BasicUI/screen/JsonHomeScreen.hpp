#pragma once
#include <string>

#include "DeviceFactory.hpp"
#include "HardwareAbstract.hpp"
#include "List.hpp"
#include "MainTopBar.hpp"
#include "PageBase.hpp"
#include "ScreenBase.hpp"
#include "StatusBar.hpp"

namespace UI::Screen {

#define TOP_BAR_HEIGHT 20

class JsonHomeScreen : public Base {
public:
  JsonHomeScreen(DeviceFactory &factory);

  void SetBgColor(lv_color_t value,
                  lv_style_selector_t selector = LV_PART_MAIN) override;

  void AddPage(Page::Base::Ptr aPage);

  bool GoToPage(ID anId) { return false; }; // return mTabView->GoToTab(anId); }

  void displayScenePage(std::string aFileName);

protected:
  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override;

private:
  void ActiveListPress();
  void SettingsPress();

  DeviceFactory &mFactory;

  // Page::TabView *mTabView;
  Widget::StatusBar *mStatusBar;
  Widget::List *mList;
};

} // namespace UI::Screen