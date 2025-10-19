#pragma once
#include <string>

#include "Command.hpp"
#include "DeviceFactory.hpp"
#include "HardwareAbstract.hpp"
// #include "JsonTabView.hpp"
#include "List.hpp"
#include "MainTopBar.hpp"
#include "PageBase.hpp"
#include "ScreenBase.hpp"
#include "StatusBar.hpp"
#include "TabView.hpp"

namespace UI::Screen {

#define TOP_BAR_HEIGHT 20

using KeyIds = KeyPressAbstract::KeyId;
using KeyPressTypes = KeyPressAbstract::KeyEvent::Type;

struct ScreensStruct {
  KeyPressTypes pressType = KeyPressTypes::INVALID;
  std::string ScreenFileName;
};

class JsonHomeScreen : public Base {
public:
  JsonHomeScreen(DeviceFactory &factory);

  void SetBgColor(lv_color_t value,
                  lv_part_t selector = LV_PART_MAIN) override;

  void AddPage(Page::Base::Ptr aPage);

  bool GoToPage(ID anId) { return false; }; // return mTabView->GoToTab(anId); }

  void displayScenePage(const std::string &aFileName, bool restoreScene);

  void OnLvglEvent(lv_event_t *aEvent);

protected:
  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override;

  void GoToSceneSelection(const std::string &aNewScene);

  bool checkSceneForEntryExit(const std::string &aFileName);

  void clearScene();

  void sendExitSequence();

private:
  DeviceFactory &mFactory;

  Handler<std::string> mSceneChangeHandler;

  // Page::TabView *mTabView;
  Widget::StatusBar *mStatusBar;
  Widget::List *mList;
  Page::TabView *mTabView;
  // Screen::JsonTabView *mSceneTabViewScreen = nullptr;
  std::vector<Command::CommandStruct> mExitCommands;
  std::string mLastScene;
  std::string mLastStartSeq;
  std::string mSavedExitSeq;
  std::multimap<KeyIds, ScreensStruct> mSceneKeyHandlers;
  std::multimap<Command::KeyIds, Command::KeyStruct> mOverrideKeyHandlers;
  std::string mScreenToLoad;
};

} // namespace UI::Screen