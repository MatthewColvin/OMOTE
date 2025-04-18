#pragma once
#include <string>

#include "DeviceFactory.hpp"
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
  HomeScreen(DeviceFactory &factory);

  void SetBgColor(lv_color_t value,
                  lv_style_selector_t selector = LV_PART_MAIN) override;

  void AddPage(Page::Base::Ptr aPage);

  bool GoToPage(ID anId) { return mTabView->GoToTab(anId); }

protected:
  // Height of the tabview under the status bar on the homescreen
  static constexpr auto ContentHeight = SCREEN_HEIGHT - Widget::StatusBar::Height;

  /**
   * Put aNewTabView on the HomeScreen and return the old one.
   */
  std::unique_ptr<Page::TabView> SwapTabView(std::unique_ptr<Page::TabView> aNewTabView);

  Widget::StatusBar *GetStatusBar() const { return mStatusBar; }
  DeviceFactory &GetDeviceFactory() const { return mFactory; }

private:
  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override;

  DeviceFactory &mFactory;

  Page::TabView *mTabView;
  Widget::StatusBar *mStatusBar;
};

} // namespace UI::Screen