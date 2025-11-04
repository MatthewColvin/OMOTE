#include "AddDevice.hpp"

using namespace UI::Page;

AddDevice::AddDevice(ActiveDevices &aActiveDevices,
                     const std::vector<std::string> &aDeviceNames,
                     DeviceCreatorTy aDeviceCreationCallback)
    : Base(ID::Pages::AddDevicePage),
      mActiveDevices(aActiveDevices),
      mDeviceCreator(aDeviceCreationCallback) {
  InitWidgets();
  for (auto &deviceName : aDeviceNames) {
    mDeviceTypeRoller->AddItem(deviceName, deviceName);
  }
}

AddDevice::AddDevice(ActiveDevices &aActiveDevices, std::vector<std::shared_ptr<IDevice>> aDevices)
    : Base(ID::Pages::AddDevicePage),
      mActiveDevices(aActiveDevices),
      mDevices(std::move(aDevices)) {
  InitWidgets();
  for (auto &device : mDevices) {
    mDeviceTypeRoller->AddItem(device->GetName(), device->GetName());
  }
  mDeviceCreator = [this](const std::string &deviceName) {
    for (auto &device : mDevices) {
      if (device->GetName() == deviceName) {
        return device;
      }
    }
    return std::shared_ptr<IDevice>(nullptr);
  };
}

void AddDevice::InitWidgets() {
  mInstructionLabel = AddNewElement<Widget::Label>("Scroll to device:");
  mDeviceTypeRoller = AddNewElement<Widget::Roller<std::string>>(nullptr);
  mAddButton = AddNewElement<Widget::Button>([this]() { OnAddButtonClicked(); });

  // Configure instruction label
  mInstructionLabel->SetHeight(lv_pct(10));
  mInstructionLabel->SetWidth(GetContentWidth());

  mDeviceTypeRoller->SetWidth(GetContentWidth());
  mDeviceTypeRoller->SetHeight(lv_pct(50));

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
