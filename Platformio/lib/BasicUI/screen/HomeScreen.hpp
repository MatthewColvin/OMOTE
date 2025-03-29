#pragma once
#include <string>

#include "Button.hpp"
#include "HardwareAbstract.hpp"
#include "Label.hpp"
#include "PageBase.hpp"
#include "ScreenBase.hpp"
#include "StatusBar.hpp"
#include "TabView.hpp"

namespace UI::Screen {

#define TOP_BAR_HEIGHT 20

class HomeScreen : public Base {
 public:
  HomeScreen();

  void SetBgColor(lv_color_t value, lv_style_selector_t selector = LV_PART_MAIN) override;

  void AddPage(Page::Base::Ptr aPage);

  bool GoToPage(ID anId) { return mTabView->GoToTab(anId); }

 protected:
  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override;

 private:
  void SettingsPress();
  void ActiveListPress();

  Widget::StatusBar* mStatusBar;
  Page::TabView* mTabView;
};

}  // namespace UI::Screen