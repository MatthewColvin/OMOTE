#include "NtpSettings.hpp"
#include "HardwareFactory.hpp"
#include "Keyboard.hpp"
#include "Label.hpp"
#include "List.hpp"
#include "LvglResourceManager.hpp"
#include "Switch.hpp"

using namespace UI;
using namespace UI::Page;

NtpSettings::NtpSettings(std::shared_ptr<wifiHandlerInterface> aWifi)
    : Base(ID::Pages::NtpSettings), mWifi(aWifi),
      mEnLabel(AddNewElement<Widget::Label>("Enable")),
      mEnSwitch(AddNewElement<Widget::Switch>([this](auto aNewState) {mWifi->enableNtp(aNewState); mSaveReqrd = true; },
                                              mWifi->isNtpEnabled())),
      mList(AddNewElement<Widget::List>()), mKeyboard(nullptr),
      mDisplayLabel(AddNewElement<Widget::Label>("Display Mode")),
      mDisplayModeDropDown(AddNewElement<Widget::DropDown<int>>([this](int aMode) { mWifi->ntpSetDisplayMode(aMode); mSaveReqrd = true; })) {

  mEnLabel->SetSize(lv_pct(80), 15);
  mEnLabel->AlignTo(this, LV_ALIGN_TOP_LEFT, 0, 15);

  mEnSwitch->SetSize(lv_pct(20), 15);
  mEnSwitch->AlignTo(mEnLabel, LV_ALIGN_OUT_RIGHT_MID);

  mDisplayLabel->SetHeight(15);
  mDisplayLabel->AlignTo(mEnLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);

  mDisplayModeDropDown->SetHeight(30);
  mDisplayModeDropDown->SetWidth(GetContentWidth());
  mDisplayModeDropDown->AddItem("Constant", ntpDisplayMode::constant);
  mDisplayModeDropDown->AddItem("Alternating", ntpDisplayMode::alternates);
  mDisplayModeDropDown->AddItem("First 5sec", ntpDisplayMode::first5sec);
  mDisplayModeDropDown->AlignTo(mDisplayLabel, LV_ALIGN_OUT_BOTTOM_MID);
  mDisplayModeDropDown->SetSelected(mWifi->ntpGetDisplayMode());

  mList->AddItem("Server", NULL, [this] { OpenKeyboard(server, mWifi->ntpGetServer()); });
  mList->AddItem("Time Zone String", NULL, [this] { OpenKeyboard(timezone, mWifi->ntpGetTimeZone()); });
  mList->SetHeight(lv_pct(50));
  mList->AlignTo(mDisplayLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 50);
}

void NtpSettings::OpenKeyboard(ntpField aField, std::string aText) {
  // We already have a Keyboard don't launch another one.
  if (mKeyboard) {
    return;
  }
  auto keyboard = std::make_unique<Widget::Keyboard>(
      [this, aField](auto aEnteredText) {
        if (aEnteredText != "") {
          switch (aField) {
          case server:
            mWifi->ntpSetServer(aEnteredText);
            mSaveReqrd = true;
            break;
          case timezone:
            mWifi->ntpSetTimeZone(aEnteredText);
            mSaveReqrd = true;
            break;

          default:
            break;
          }
        }
        mKeyboard->AnimateOut();
      },
      aText);
  keyboard->OnKeyboardAnimatedOut([this] {
    // Keyboard is done animating out remove it and null the ref
    RemoveElement(mKeyboard);
    mKeyboard = nullptr;
  });
  mKeyboard = AddElement(std::move(keyboard));
}

void NtpSettings::SetHeight(lv_coord_t aHeight) {
  Base::SetHeight(aHeight);
};

NtpSettings::~NtpSettings() {
  if (mSaveReqrd) {
    mWifi->ntpSaveCredentials();
    mWifi->setupNtp();
  }
}
