#include "JsonHomeScreen.hpp"

#include "ActionTester.hpp"
#include "AddDevice.hpp"
#include "HardwareFactory.hpp"
#include "JsonTabView.hpp"
#include "ScreenManager.hpp"
#include "SettingsPage.hpp"

using namespace UI::Screen;

JsonHomeScreen::JsonHomeScreen(DeviceFactory &aFactory)
    : Base(UI::ID::Screens::Home),
      mFactory(aFactory),
      mStatusBar(AddNewElement<Widget::StatusBar>(mFactory)),
      mList(AddNewElement<Widget::List>()) {
  SetBgColor(UI::Color::BLACK);
  SetPushAnimation(LV_SCR_LOAD_ANIM_FADE_IN);

  static constexpr auto ContentHeight =
      SCREEN_HEIGHT - Widget::StatusBar::Height;
  mList->SetHeight(ContentHeight);
  mList->AlignTo(mStatusBar, LV_ALIGN_OUT_BOTTOM_MID);
  File fp = HardwareFactory::getAbstract().littleFs()->open("Scenes.json", LFS_O_RDONLY);
  if (!fp)
    return;
  std::string content = fp.read(1000);

  MemConsciousDocument d;
  d.Parse(content.c_str());

  if (d.HasMember("Scenes")) {
    for (rapidjson::SizeType i = 0; i < d["Scenes"].Size(); i++) {
      if (d["Scenes"][i].HasMember("FileName")) {
        std::string fileName = d["Scenes"][i]["FileName"].GetString();
        std::string sceneName;
        if (d["Scenes"][i].HasMember("SceneName"))
          sceneName = d["Scenes"][i]["SceneName"].GetString();
        else
          sceneName = fileName;

        mList->AddItem(sceneName, NULL, [this, fileName] { displayScenePage(fileName); });
      }
    }
  }

  mStatusBar->AddExtraSettingItem({"Test Actions", LV_SYMBOL_LIST, [this] {
                                     return std::make_unique<UI::Page::ActionTester>();
                                   }});
  mStatusBar->AddExtraSettingItem({"Add Json Device", LV_SYMBOL_EDIT, [this] {
                                     auto jsonDevices = mFactory.getJsonDevices();
                                     // return std::make_unique<UI::Page::AddDevice>(ActiveDevices, jsonDevices);
                                     return nullptr;
                                   }});
}

void JsonHomeScreen::displayScenePage(std::string aFileName) {
  UI::Screen::Manager::getInstance().pushScreen(
      std::make_unique<JsonTabView>(mFactory, aFileName));
}

void JsonHomeScreen::AddPage(Page::Base::Ptr aPage) {
  // mTabView->AddTab(std::move(aPage));
}

void JsonHomeScreen::SetBgColor(lv_color_t value, lv_style_selector_t selector) {
  // mTabView->SetBgColor(value, selector);
  UI::UIElement::SetBgColor(value, selector);
}

bool JsonHomeScreen::OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) {
  return false;
};
