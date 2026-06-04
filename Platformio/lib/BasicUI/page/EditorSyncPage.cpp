#include "EditorSyncPage.hpp"

#include "HardwareFactory.hpp"
#include "Label.hpp"
#include "editor_sync_mode.hpp"
#ifndef IS_SIMULATOR
#include "WiFi.h"
#endif

using namespace UI::Page;

EditorSyncPage::EditorSyncPage()
    : Base(ID::Pages::EditorSyncPage),
      mTitle(AddNewElement<Widget::Label>("Editor sync")),
      mBody(AddNewElement<Widget::Label>("")),
      mIp(AddNewElement<Widget::Label>("")) {

  SetBgColor(Color::BLACK);
  mTitle->SetHeight(lv_pct(12));
  mTitle->AlignTo(this, LV_ALIGN_TOP_MID, 0, 8);

  mBody->SetHeight(LV_SIZE_CONTENT);
  mBody->SetWidth(lv_pct(92));
  mBody->SetText(
      "Config editor session active.\n\n"
      "The remote stays awake while you edit on your PC.\n"
      "Touch still works — use Power when finished to reboot.\n\n"
      "Or disconnect from the editor Leave session button.");
  mBody->AlignTo(mTitle, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);

  std::string ipLine = "Waiting for WiFi…";
#ifdef IS_SIMULATOR
  const auto st = HardwareFactory::getAbstract().wifi()->GetStatus();
  if (st.isConnected)
    ipLine = std::string("IP: ") + st.IP;
#else
  if (WiFi.isConnected())
    ipLine = std::string("IP: ") + WiFi.localIP().toString().c_str();
#endif
  mIp->SetText(ipLine);
  mIp->SetHeight(LV_SIZE_CONTENT);
  mIp->AlignTo(mBody, LV_ALIGN_OUT_BOTTOM_MID, 0, 16);
}

bool EditorSyncPage::OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) {
  if (aKeyEvent.mId == KeyPressAbstract::KeyId::Power &&
      aKeyEvent.mType == KeyPressAbstract::KeyEvent::Type::Press) {
    editor_sync_mode::exit(true);
    return true;
  }
  return true;
}
