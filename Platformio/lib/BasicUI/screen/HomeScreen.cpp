#include "HomeScreen.hpp"

#include "Colors.hpp"
#include "Demo.hpp"
#include "IrLearner.hpp"
#include "MainTopBar.hpp"
#include "SettingsPage.hpp"

using namespace UI::Screen;

HomeScreen::HomeScreen(ActiveDevices& aActiveDevices)
    : Base(UI::ID::Screens::Home),
      mActiveDevices(aActiveDevices),
      mTopBar(AddNewElement<Widget::MainTopBar>(mActiveDevices)),
      mTabView(AddNewElement<Page::TabView>(ID(ID::Pages::INVALID_PAGE_ID))) {
  SetBgColor(UI::Color::BLACK);
  SetPushAnimation(LV_SCR_LOAD_ANIM_FADE_IN);

  static constexpr auto ContentHeight =
      SCREEN_HEIGHT - Widget::MainTopBar::Height;

  mTabView->SetHeight(ContentHeight);

  mTabView->AlignTo(mTopBar, LV_ALIGN_OUT_BOTTOM_MID);

  mTabView->AddTab(std::make_unique<Page::SettingsPage>());
}

void HomeScreen::AddPage(Page::Base::Ptr aPage) {
  mTabView->AddTab(std::move(aPage));
}

void HomeScreen::SetBgColor(lv_color_t value, lv_style_selector_t selector) {
  mTabView->SetBgColor(value, selector);
  UI::UIElement::SetBgColor(value, selector);
}

bool HomeScreen::OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) {
  return false;
};
