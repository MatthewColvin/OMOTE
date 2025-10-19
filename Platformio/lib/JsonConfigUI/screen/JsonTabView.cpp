#include "JsonTabView.hpp"

#include "Colors.hpp"
#include "HardwareFactory.hpp"
#include "HeatingPage.hpp"
#include "IrLearner.hpp"
#include "JsonPage.hpp"
#include "PopUpScreen.hpp"
#include "ScreenManager.hpp"
#include "SettingsPage.hpp"
#include "observerHandles.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace UI::Screen;

JsonTabView::JsonTabView(DeviceFactory &aFactory, std::string aFileName)
    : Base(UI::ID::Screens::Home),
      mFactory(aFactory),
      mStatusBar(AddNewElement<Widget::StatusBar>(mFactory)),
      mTabView(AddNewElement<Page::TabView>(ID(ID::Pages::INVALID_PAGE_ID))) {
  SetBgColor(UI::Color::BLACK);
  SetPushAnimation(LV_SCR_LOAD_ANIM_FADE_IN);

  static constexpr auto ContentHeight =
      SCREEN_HEIGHT - Widget::StatusBar::Height;
  mTabView->SetHeight(ContentHeight);
  mTabView->AlignTo(mStatusBar, LV_ALIGN_OUT_BOTTOM_MID);

  std::ofstream fp(FS_PATH + std::string(aFileName), std::ios::in);
  std::stringstream contentSs;

  MemConsciousDocument d;
  d.Parse(contentSs.str().c_str());

  if (d.HasMember("Pages")) {
    for (rapidjson::SizeType i = 0; i < d["Pages"].Size(); i++) {
      if (d["Pages"][i].HasMember("FileName")) {
        std::string fileName = d["Pages"][i]["FileName"].GetString();
        std::string pageName;
        if (d["Pages"][i].HasMember("PageName"))
          pageName = d["Pages"][i]["PageName"].GetString();
        else
          pageName = fileName;
        std::string commandPrefix;
        if (d["Pages"][i].HasMember("CommandPrefix"))
          commandPrefix = d["Pages"][i]["CommandPrefix"].GetString();

        mTabView->AddTab(std::make_unique<Page::JsonPage>(fileName, pageName, commandPrefix));
      }
    }
  }
}

void JsonTabView::AddPage(Page::Base::Ptr aPage) {
  mTabView->AddTab(std::move(aPage));
}

void JsonTabView::SetBgColor(lv_color_t value, lv_part_t selector) {
  mTabView->SetBgColor(value, selector);
  UI::UIElement::SetBgColor(value, selector);
}

bool JsonTabView::OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) {
  return false;
};

void JsonTabView::SettingsPress() {
  UI::Screen::Manager::getInstance().pushPopUp(
      std::make_unique<Page::SettingsPage>(), LV_SCR_LOAD_ANIM_OVER_BOTTOM);
}

void JsonTabView::ActiveListPress() {}
