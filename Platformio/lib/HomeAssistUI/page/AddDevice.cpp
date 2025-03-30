#include "AddDevice.hpp"

using namespace UI::Page;

AddDevice::AddDevice(ActiveDevices& aActiveDevices,
                     const std::vector<std::string>& aDeviceNames,
                     DeviceCreatorTy aDeviceCreationCallback)
    : Base(ID::Pages::AddDevicePage),
      mInstructionLabel(AddNewElement<Widget::Label>("Scroll to device:")),
      mDeviceTypeRoller(AddNewElement<Widget::Roller<std::string>>(nullptr)),
      mAddButton(
          AddNewElement<Widget::Button>([this]() { OnAddButtonClicked(); })),
      mActiveDevices(aActiveDevices),
      mDeviceCreator(aDeviceCreationCallback) {
  // Configure instruction label
  mInstructionLabel->SetHeight(lv_pct(10));
  mInstructionLabel->SetWidth(GetContentWidth());

  mDeviceTypeRoller->SetWidth(GetContentWidth());
  mDeviceTypeRoller->SetHeight(lv_pct(50));
  for (auto& deviceName : aDeviceNames) {
    mDeviceTypeRoller->AddItem(deviceName, deviceName);
  }

  mAddButton->SetText("Add Device");
  mAddButton->SetWidth(GetContentWidth() / 2);
  mAddButton->SetHeight(lv_pct(20));

  mInstructionLabel->AlignTo(this, LV_ALIGN_TOP_MID);
  mDeviceTypeRoller->AlignTo(mInstructionLabel, LV_ALIGN_OUT_BOTTOM_MID);
  mAddButton->AlignTo(mDeviceTypeRoller, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void AddDevice::OnAddButtonClicked() {
  auto deviceName = mDeviceTypeRoller->GetSelectedData();
  if (auto device = mDeviceCreator(deviceName); device) {
    mActiveDevices.addDevice(device);
  } else {
    // TODO: Push error message to user about failing to create device?
  }
}
