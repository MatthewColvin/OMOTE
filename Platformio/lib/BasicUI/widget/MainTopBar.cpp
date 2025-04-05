#include "MainTopBar.hpp"

#include "ActiveDeviceList.hpp"
#include "Colors.hpp"
#include "ScreenManager.hpp"
// TODO MatthewColvin/OMOTE#30
// Need to merge some of this stuff into status bar

namespace UI::Widget {

MainTopBar::MainTopBar(ActiveDevices &aActiveDevices)
    : Base(ID::Widgets::MainTopBar),
      mActiveDevices(aActiveDevices),
      mDeviceLabel(AddNewElement<Widget::Label>("")),
      mLed(AddNewElement<Widget::LED>()) {
  SetHeight(Height);
  // Pad left so content stays on screen
  SetPadding(GetPadding().Left(6));

  // Configure LED
  mLed->SetHeight(GetContentHeight() / 3);
  mLed->SetWidth(GetContentHeight() / 3); // Make it square

  // Configure device label
  mDeviceLabel->SetHeight(mLed->GetContentHeight());
  mDeviceLabel->SetWidth(GetContentWidth() / 2);
  mDeviceLabel->SetPadding(mDeviceLabel->GetPadding().Left(10));
  mDeviceLabel->SetTextStyle(
      mDeviceLabel->GetTextStyle().Font(&lv_font_montserrat_16));

  // Align Internal widgets
  mLed->AlignTo(this, LV_ALIGN_LEFT_MID);
  mDeviceLabel->AlignTo(mLed, LV_ALIGN_OUT_RIGHT_MID);

  DisplayTopPrioityDeviceInfo();

  mDeviceChangeHandler.SetNotification(
      mActiveDevices.getListUpdateNotification());
  mDeviceChangeHandler = [this](auto listChangeEvent) {
    DisplayTopPrioityDeviceInfo();
  };
}

void MainTopBar::DisplayTopPrioityDeviceInfo() {
  const auto &devices = mActiveDevices.getDevices();
  DisplayDeviceInfo(devices.empty() ? nullptr : devices.back());
}

void MainTopBar::DisplayDeviceInfo(std::shared_ptr<IDevice> aDeviceToDisplay) {
  if (!aDeviceToDisplay) {
    mDeviceLabel->SetText("No Device");
    mLed->SetColor(Color::GREY);
    mLed->SetBrightness(0);
    return;
  }
  auto name = aDeviceToDisplay->GetName();
  mDeviceLabel->SetText(name.empty() ? "Unnamed Device" : name);
  mLed->SetColor(aDeviceToDisplay->GetDisplayColor());
}

void MainTopBar::OpenActiveDevicesPage() {
}

void MainTopBar::OnLvglEvent(lv_event_t *aEvent) {
  auto code = lv_event_get_code(aEvent);
  if (code == LV_EVENT_PRESSED) {
    OpenActiveDevicesPage();
  }
}

} // namespace UI::Widget
