#include "JsonDevices/JsonDevice.hpp"

namespace Json {

JsonDevice::JsonDevice(File &aDeviceJsonFile) {
  constexpr auto maxDeviceFileSize = 2000;
  if (aDeviceJsonFile.size() > maxDeviceFileSize) {
    mIsValid = false;
    return;
  }

  auto deviceJsonStr = aDeviceJsonFile.read(maxDeviceFileSize);
  MemConsciousDocument deviceJson;
  deviceJson.Parse(deviceJsonStr.c_str());

  mName = deviceJson["name"].GetString();
  mId = DeviceId::None;
}

std::string JsonDevice::GetName() const {
  return mName;
}

lv_color_t JsonDevice::GetDisplayColor() const {
  // TODO Add parsing to specify device color
  return UI::Color::BLUE;
}

bool JsonDevice::IsDeleteAble() const {
  return true;
}

DeviceType JsonDevice::GetType() const {
  return DeviceType::JSON;
}

DeviceId JsonDevice::GetId() const {
  return DeviceId::None;
}

MemConsciousDocument JsonDevice::GetExtraConfig() const {
  // TODO add File path to Device so it can be restored on boot
  return {};
}

void JsonDevice::SetExtraConfig(const MemConsciousDocument &config) {
  // TODO restore from file
}

bool JsonDevice::HandleKeyEvent(KeyPressAbstract::KeyEvent aEvent) {
  if (!mKeyActions.contains(aEvent.mId)) {
    return false;
  }
  // TODO should we have ExecuteAction and execute return bool?
  auto &keyHandler = mKeyActions[aEvent.mId];
  keyHandler.ExecuteAction(aEvent.mType);
  return true;
}

std::unique_ptr<UI::Page::Base> JsonDevice::GetControlPage() {
  // Return nullptr for now - this would need to be implemented based on your UI system
  // Maybe device could define page json?
  return nullptr;
}

bool JsonDevice::isValid() const {
  return mIsValid;
}

} // namespace Json
