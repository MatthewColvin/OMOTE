#include "FtpSettings.hpp"
#include "HardwareFactory.hpp"
#include "Keyboard.hpp"
#include "Label.hpp"
#include "List.hpp"
#include "LvglResourceManager.hpp"
#include "Switch.hpp"

using namespace UI;
using namespace UI::Page;

FtpSettings::FtpSettings(std::shared_ptr<wifiHandlerInterface> aWifi)
    : Base(ID::Pages::FtpSettings), mWifi(aWifi),
      mEnLabel(AddNewElement<Widget::Label>("Enable")),
      mEnSwitch(AddNewElement<Widget::Switch>([this](auto aNewState) { mWifi->enableFtp(aNewState); mSaveReqrd = true; },
                                              mWifi->isFtpEnabled())),
      mList(AddNewElement<Widget::List>()), mKeyboard(nullptr),
      mLabel(AddNewElement<Widget::Label>("IP:  " + mWifi->GetStatus().IP)) {

  mEnLabel->SetSize(lv_pct(80), 15);
  mEnLabel->AlignTo(this, LV_ALIGN_TOP_LEFT, 0, 15);

  mEnSwitch->SetSize(lv_pct(20), 15);
  mEnSwitch->AlignTo(mEnLabel, LV_ALIGN_OUT_RIGHT_MID);

  mLabel->SetHeight(15);
  mLabel->AlignTo(mEnLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);

  mList->AddItem("mDNS name", NULL, [this] { OpenKeyboard(mDNS_name, mWifi->mDNSGetName()); });
  mList->AddItem("User", NULL, [this] { OpenKeyboard(ftp_user, mWifi->ftpGetUser()); });
  mList->AddItem("Password", NULL, [this] { OpenKeyboard(ftp_password, mWifi->ftpGetPassword()); });
  mList->SetHeight(lv_pct(50));
  mList->AlignTo(mLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);
}

void FtpSettings::OpenKeyboard(ftpField aField, std::string aText) {
  // We already have a Keyboard don't launch another one.
  if (mKeyboard) {
    return;
  }
  auto keyboard = std::make_unique<Widget::Keyboard>(
      [this, aField](auto aEnteredText) {
        if (aEnteredText != "") {
          switch (aField) {
          case mDNS_name:
            mWifi->mDNSSetName(aEnteredText);
            mSaveReqrd = true;
            break;
          case ftp_user:
            mWifi->ftpSetUser(aEnteredText);
            mSaveReqrd = true;
            break;
          case ftp_password:
            mWifi->ftpSetPassword(aEnteredText);
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

void FtpSettings::SetHeight(lv_coord_t aHeight) {
  Base::SetHeight(aHeight);
};

FtpSettings::~FtpSettings() {
  if (mSaveReqrd)
    mWifi->ftpSaveCredentials();
}
