#pragma once
#include "ActiveDevices.hpp"
#include "Button.hpp"
#include "IDevice.hpp"
#include "Label.hpp"
#include "PageBase.hpp"
#include "Roller.hpp"

namespace UI::Page {

class AddDevice : public Base {
 public:
  using DeviceCreatorTy =
      std::function<std::shared_ptr<IDevice>(const std::string&)>;

  AddDevice(ActiveDevices& aActiveDevices,
            const std::vector<std::string>& aDeviceNames,
            DeviceCreatorTy aDeviceCreationCallback);

  virtual ~AddDevice() = default;

  // Override from Base so title is shown to user
  std::string GetTitle() override { return "Add Device"; }

 private:
  void OnAddButtonClicked();

  Widget::Label* mInstructionLabel;
  Widget::Roller<std::string>* mDeviceTypeRoller;
  Widget::Button* mAddButton;
  ActiveDevices& mActiveDevices;
  DeviceCreatorTy mDeviceCreator;
};

}  // namespace UI::Page
