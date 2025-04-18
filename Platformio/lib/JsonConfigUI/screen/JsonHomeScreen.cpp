#include "JsonHomeScreen.hpp"

#include "ActionTester.hpp"
#include "HardwareFactory.hpp"
#include "JsonTabView.hpp"
#include "SettingsPage.hpp"

using namespace UI::Screen;

JsonHomeScreen::JsonHomeScreen(DeviceFactory &aFactory)
    : HomeScreen(aFactory),
      mList(AddNewElement<Widget::List>()) {
  SetBgColor(UI::Color::BLACK);
  SetPushAnimation(LV_SCR_LOAD_ANIM_FADE_IN);

  static constexpr auto ContentHeight =
      SCREEN_HEIGHT - Widget::StatusBar::Height;
  mList->SetHeight(ContentHeight);
  mList->AlignTo(GetStatusBar(), LV_ALIGN_OUT_BOTTOM_MID);
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

  GetStatusBar()->AddExtraSettingItem({"Test Actions", LV_SYMBOL_LIST, [this] {
                                         return std::make_unique<UI::Page::ActionTester>();
                                       }});
}

void JsonHomeScreen::displayScenePage(std::string aFileName) {
  // Use Base class HomeScreen replacing its tabview to represent the new scene.
  auto oldSceneTabView = SwapTabView(
      std::make_unique<UI::Page::JsonTabView>(aFileName));
  // TODO: Save the non scene tabview and then restore it when scene is complete?
}
