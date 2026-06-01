#include "JsonHomeScreen.hpp"

#include "ActionTester.hpp"
#include "AddDevice.hpp"
#include "HaRuntime.hpp"
#include "HardwareFactory.hpp"
#include "JsonPage.hpp"
#include "RapidJsonUtilty.hpp"
#include "ScreenManager.hpp"
#include "SettingsPage.hpp"
#include "editor_sync_mode.hpp"
#include <filesystem>
#include <fstream>

using namespace UI::Screen;

#define RTC_SIG 0x128934ab56cd78ef
#define RTC_STR_SIZE 100

#ifdef OMOTE_SIM
static struct {
  uint64_t signature;
  char currentScene[RTC_STR_SIZE];
  uint16_t tabIdx;
} RtcLastState; // = {RTC_SIG, "Scenes/Scene_Audio.json", 1};
#else
RTC_DATA_ATTR static struct {
  uint64_t signature;
  char currentScene[RTC_STR_SIZE];
  uint16_t tabIdx;
} RtcLastState;
#endif

// NOTE : entry and exit sequence code is a little messy, this is because we have scenes
// that have them and scenes which don't (which therefore aren't really scenes).  Should really
// restructure to better handle this, and also give better status bar info, but this will do for now

JsonHomeScreen::JsonHomeScreen(DeviceFactory &aFactory)
    : Base(UI::ID::Screens::Home),
      mFactory(aFactory),
      mStatusBar(AddNewElement<Widget::StatusBar>(mFactory)),
      mList(AddNewElement<Widget::List>()),
      mTabView(AddNewElement<Page::TabView>(ID(ID::Pages::INVALID_PAGE_ID))) {

  UI::Screen::Manager::getInstance().setAllScreenProcessKeys(true);

  SetBgColor(UI::Color::BLACK);
  // Bring up immediately as main screen, backlight fade in provides equivalent to fade animation
  // otherwise combined fade in and backlight fade can cause odd effect at some brightensses
  SetPushAnimation(LV_SCR_LOAD_ANIM_NONE);
  // Init Factory to allow building of Json devices
  aFactory.InitJsonFactory();
  HaRuntime::init();

  mSceneChangeHandler.SetNotification(mStatusBar->GetSceneChangeNotification());
  mSceneChangeHandler = [this](std::string aNewScene) { GoToSceneSelection(aNewScene); };

  mStatusBar->SetTopButtonLabel("Select Scene");

  mStatusBar->AddExtraSettingItem({"Editor sync", LV_SYMBOL_REFRESH, [] {
    editor_sync_mode::enter();
    return UI::Page::Base::Ptr{};
  }});

  static constexpr auto ContentHeight =
      SCREEN_HEIGHT - Widget::StatusBar::Height;
  mList->SetHeight(ContentHeight);
  mList->AlignTo(mStatusBar, LV_ALIGN_OUT_BOTTOM_MID);
  mTabView->SetHeight(ContentHeight);
  mTabView->AlignTo(mStatusBar, LV_ALIGN_OUT_BOTTOM_MID);
  mTabView->SetVisiblity(false);

  rapidjson::Document d = OMOTE::JSON::GetDocument(std::filesystem::path(FS_PATH "Scenes.json"));
  if (d.HasMember("Scenes")) {
    for (rapidjson::SizeType i = 0; i < d["Scenes"].Size(); i++) {
      auto &scene = d["Scenes"][i];
      if (scene.HasMember("FileName") && scene["FileName"].IsString()) {
        std::string fileName = scene["FileName"].GetString();
        std::string sceneName;
        if (scene.HasMember("SceneName") && scene["SceneName"].IsString())
          sceneName = scene["SceneName"].GetString();
        else
          sceneName = fileName;

        auto symbol = checkSceneForEntryExit(fileName) ? LV_SYMBOL_WIFI : LV_SYMBOL_MINUS;
        mList->AddItem(sceneName, symbol, [this, fileName] { displayScenePage(fileName, false); });

        if (scene.HasMember("BindToKey") && scene["BindToKey"].IsString()) {
          if (scene.HasMember("PressType") && scene["PressType"].IsString()) {
            auto id = magic_enum::enum_cast<KeyIds>(scene["BindToKey"].GetString());
            auto type = magic_enum::enum_cast<KeyPressTypes>(scene["PressType"].GetString());
            if (id.has_value() && type.has_value())
              mSceneKeyHandlers.insert({id.value(), {type.value(), fileName}});
          }
        }
      }
    }
  }

  if (RtcLastState.signature == RTC_SIG) {
    // Serial.printf("Restoring scene from: %s\r\n", RtcLastState.currentScene);
    displayScenePage(RtcLastState.currentScene, true);
  } // else Serial.println("RTC sig invalid");

  mStatusBar->AddDebugSettingItem({"Test Actions", LV_SYMBOL_LIST, [this] {
                                     return std::make_unique<UI::Page::ActionTester>();
                                   }});

  mStatusBar->AddDebugSettingItem({"Add Json Device", LV_SYMBOL_EDIT, [this] {
                                     auto jsonDevices = mFactory.getJsonDevices();
                                     return std::make_unique<UI::Page::AddDevice>(mFactory.getActiveDevices(), jsonDevices);
                                   }});
}

bool JsonHomeScreen::checkSceneForEntryExit(const std::string &aFileName) {
  std::filesystem::path filePath(FS_PATH + aFileName);
  rapidjson::Document d = OMOTE::JSON::GetDocument(filePath);

  if (d.HasParseError() || d.IsNull())
    return false;

  return ((d.HasMember("StartCommandSequence") && d["StartCommandSequence"].IsArray()) ||
          (d.HasMember("ExitCommandSequence") && d["ExitCommandSequence"].IsArray()));
}

void JsonHomeScreen::clearScene() {
  RemoveElement(mTabView);
  mTabView = AddNewElement<Page::TabView>(ID(ID::Pages::INVALID_PAGE_ID));
  mTabView->SetHeight(SCREEN_HEIGHT - Widget::StatusBar::Height);
  mTabView->AlignTo(mStatusBar, LV_ALIGN_OUT_BOTTOM_MID);
  mTabView->SetVisiblity(false);
  mTabView->OnTabChangeEvent([this](uint16_t idx) { RtcLastState.tabIdx = mTabView->GetCurrentTabIdx(); });
  mOverrideKeyHandlers.clear();
}

void JsonHomeScreen::displayScenePage(const std::string &aFileName, bool restoreScene) {
  std::filesystem::path aFilePath(FS_PATH + aFileName);
  rapidjson::Document d = OMOTE::JSON::GetDocument(aFilePath);

  if (d.HasParseError() || d.IsNull())
    return; // file error, nothing to do

  if (mLastScene == aFileName) {
    mTabView->SetVisiblity(true);
    mList->SetVisiblity(false);
    lv_obj_fade_in(mTabView->LvglSelf(), 400, 0);
    return; // no scene change, just bring existing tabview back up
  }

  if (!mExitCommands.empty() && (mSavedExitSeq != aFileName) && checkSceneForEntryExit(aFileName)) {
    // if we have an exit sequence to use, which isn't for the new scene, and the new scene uses entry or exit sequences then send it
    sendExitSequence();
    // Serial.println("Sending Exit commands");
  }

  clearScene();

  if (d.HasMember("ScreenName") && d["ScreenName"].IsString()) {
    std::string string = d["ScreenName"].GetString();
    string.insert(0, "Scene:");
    mStatusBar->SetTopButtonLabel(string);
  } else
    mStatusBar->SetTopButtonLabel(aFileName);

  if (d.HasMember("Pages") && d["Pages"].IsArray()) {
    for (rapidjson::SizeType i = 0; i < d["Pages"].Size(); i++) {
      if (d["Pages"][i].HasMember("FileName") && d["Pages"][i]["FileName"].IsString()) {
        std::string fileName = d["Pages"][i]["FileName"].GetString();
        std::string pageName;
        std::string shortName;
        if (d["Pages"][i].HasMember("PageName") && d["Pages"][i]["PageName"].IsString())
          pageName = d["Pages"][i]["PageName"].GetString();
        else
          pageName = fileName;
        if (d["Pages"][i].HasMember("ShortName") && d["Pages"][i]["ShortName"].IsString())
          shortName = d["Pages"][i]["ShortName"].GetString();
        std::string commandPrefix;
        if (d["Pages"][i].HasMember("CommandPrefix") && d["Pages"][i]["CommandPrefix"].IsString())
          commandPrefix = d["Pages"][i]["CommandPrefix"].GetString();

        auto page = std::make_unique<Page::JsonPage>(fileName, pageName, commandPrefix);
        if (d["Pages"][i].HasMember("OverrideKeys") && d["Pages"][i]["OverrideKeys"].IsArray()) {
          auto array = d["Pages"][i]["OverrideKeys"].GetArray();
          std::multimap<Command::KeyIds, Command::KeyStruct> keyHandlers;
          page->getKeyOverrides(array, keyHandlers);
          mOverrideKeyHandlers.insert(keyHandlers.begin(), keyHandlers.end());
        }
        page->SetTitle(shortName);
        mTabView->AddTab(std::move(page));
      }
    }
  }

  if ((mLastStartSeq != aFileName) && !restoreScene && d.HasMember("StartCommandSequence") && d["StartCommandSequence"].IsArray()) {
    for (rapidjson::SizeType i = 0; i < d["StartCommandSequence"].Size(); i++) {
      if (d["StartCommandSequence"][i].HasMember("CommandFile") && d["StartCommandSequence"][i]["CommandFile"].IsString()) {
        if (d["StartCommandSequence"][i].HasMember("Command") && d["StartCommandSequence"][i]["Command"].IsString()) {
          Command::CommandStruct aCommand;
          if (Command::NONE != Command::Commands::getCommand(d["StartCommandSequence"][i]["CommandFile"].GetString(), "",
                                                             d["StartCommandSequence"][i]["Command"].GetString(), aCommand)) {
            Command::Commands::sendCommand(aCommand);
            mLastStartSeq = aFileName;
          }
        }
      }
    }
  }

  if (mExitCommands.empty() && d.HasMember("ExitCommandSequence") && d["ExitCommandSequence"].IsArray()) { // don't add if reloading
    for (rapidjson::SizeType i = 0; i < d["ExitCommandSequence"].Size(); i++) {
      if (d["ExitCommandSequence"][i].HasMember("CommandFile") && d["ExitCommandSequence"][i]["CommandFile"].IsString()) {
        if (d["ExitCommandSequence"][i].HasMember("Command") && d["ExitCommandSequence"][i]["Command"].IsString()) {
          Command::CommandStruct aCommand;
          if (Command::NONE != Command::Commands::getCommand(d["ExitCommandSequence"][i]["CommandFile"].GetString(), "",
                                                             d["ExitCommandSequence"][i]["Command"].GetString(), aCommand)) {
            mExitCommands.push_back(aCommand);
            mSavedExitSeq = aFileName;
          }
        }
      }
    }
  }

  mLastScene = aFileName;

  if (restoreScene) {
    // Serial.printf("Restoring tab: %d\r\n", RtcLastState.tabIdx);
    mTabView->SetCurrentTabIdx(RtcLastState.tabIdx, LV_ANIM_OFF);
    // needed when waking from deep sleep but then enabling light sleep
    HardwareFactory::getAbstract().setInScene(true);
  } else {
    RtcLastState.signature = RTC_SIG;
    strncpy(RtcLastState.currentScene, aFileName.c_str(), RTC_STR_SIZE);
    RtcLastState.tabIdx = 0;
    HardwareFactory::getAbstract().setInScene(true);
    //  Serial.printf("Restore scene set to: %s\r\n",RtcLastState.currentScene);
  }

  mTabView->SetVisiblity(true);
  mList->SetVisiblity(false);
  lv_obj_fade_in(mTabView->LvglSelf(), 400, 0);
  mTabView->OnShow();
}

void JsonHomeScreen::AddPage(Page::Base::Ptr aPage) {
  // mTabView->AddTab(std::move(aPage));
}

void JsonHomeScreen::SetBgColor(lv_color_t value, lv_part_t selector) {
  // mTabView->SetBgColor(value, selector);
  UI::UIElement::SetBgColor(value, selector);
}

bool JsonHomeScreen::OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) {
  // handle exit sequence first
  if ((aKeyEvent.mId == KeyPressAbstract::KeyId::Power)) {
    if (aKeyEvent.mType == Command::KeyPressTypes::Press) {
      return true; // prevent page from responding to press event
    } else if (aKeyEvent.mType == Command::KeyPressTypes::Long) {
      // long press - send exit sequence and return to home screen
      sendExitSequence();
      clearScene();
      mLastScene.clear();
      mLastStartSeq.clear();
      RtcLastState.signature = 0; // invalidate non volatile RAM sig to prevent restoring  closed scene on power up
      HardwareFactory::getAbstract().setInScene(false);
      mStatusBar->SetTopButtonLabel("Select Scene");
      GoToSceneSelection("");
      return true;
    } else if (aKeyEvent.mType == Command::KeyPressTypes::Short) {
      // return to home screen without sending sequence or cancelling last scene
      GoToSceneSelection("");
      return true;
    }
  }
  // scene slection keys first
  {
    auto range = mSceneKeyHandlers.equal_range(aKeyEvent.mId);
    if (range.first != mSceneKeyHandlers.end()) {
      for (auto i = range.first; i != range.second; ++i) {
        if (i->second.pressType == aKeyEvent.mType) {
          displayScenePage(i->second.ScreenFileName, false);
          return true;
        }
      }
    }
  }

  // then override key handlers
  {
    auto range = mOverrideKeyHandlers.equal_range(aKeyEvent.mId);
    if (range.first != mOverrideKeyHandlers.end()) {
      for (auto i = range.first; i != range.second; ++i) {
        if (i->second.pressType == aKeyEvent.mType) {
          Command::Commands::sendCommand(i->second.command);
          return true;
        }
      }
    }
  }

  return false;
};

void JsonHomeScreen::sendExitSequence() {
  for (auto &i : mExitCommands)
    Command::Commands::sendCommand(i);
  mExitCommands.clear();
  mSavedExitSeq.clear();
}

void JsonHomeScreen::reloadCurrentSceneFromDisk() {
  if (mLastScene.empty() || !mTabView->IsVisible())
    return;
  const std::string scene = mLastScene;
  const uint16_t tabIdx = mTabView->GetCurrentTabIdx();
  mLastScene.clear();
  displayScenePage(scene, false);
  mTabView->SetCurrentTabIdx(tabIdx, LV_ANIM_OFF);
  mTabView->OnShow();
}

void JsonHomeScreen::GoToSceneSelection(const std::string &aNewScene) {
  // Can't get clickable to work, assume it only sets parent not children
  // lv_obj_remove_flag(mTabView->LvglSelf(), LV_OBJ_FLAG_CLICKABLE);
  // lv_obj_fade_out(mTabView->LvglSelf(), 100, 0);
  mTabView->SetVisiblity(false);
  mList->SetVisiblity(true);
  lv_obj_fade_in(mList->LvglSelf(), 400, 0);
}

void JsonHomeScreen::OnLvglEvent(lv_event_t *aEvent) {
  if (lv_event_get_code(aEvent) == LV_EVENT_VALUE_CHANGED) {
    RtcLastState.tabIdx = mTabView->GetCurrentTabIdx();
    // Serial.printf("Restore tab Idx set to: %d\r\n",RtcLastState.tabIdx);
  }
}
