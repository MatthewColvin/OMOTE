#include "JsonDevices/JsonDevice.hpp"
#include "magic_enum.hpp"

namespace Json {

JsonDevice::JsonDevice(File &aDeviceJsonFile) {
  constexpr auto maxDeviceFileSize = 2000;
  if (aDeviceJsonFile.size() > maxDeviceFileSize) {
    mParseResult = ParseResult::FileError;
    return;
  }

  auto deviceJsonStr = aDeviceJsonFile.read(maxDeviceFileSize);
  MemConsciousDocument deviceJson;
  deviceJson.Parse(deviceJsonStr.c_str());

  mParseResult = parse(deviceJson);
}

JsonDevice::ParseResult JsonDevice::parse(const MemConsciousDocument &aDeviceJson) {
  if (!parseGeneralInfo(aDeviceJson)) {
    return ParseResult::GeneralInfoError;
  }
  if (aDeviceJson.HasMember("keys") && !parseKeyActions(aDeviceJson["keys"])) {
    return ParseResult::KeyActionsError;
  }
  return ParseResult::Success;
}

bool JsonDevice::parseGeneralInfo(const MemConsciousDocument &aDeviceJson) {
  // Todo add color info and checks
  mName = aDeviceJson["name"].GetString();
  mId = DeviceId::None;
  return true;
}

bool JsonDevice::parseKeyActions(const MemConciousValue &aKeysJson) {
  if (!aKeysJson.IsObject()) {
    return false;
  }
  for (auto keyEnum : magic_enum::enum_values<KeyPressAbstract::KeyId>()) {
    auto keyIdStr = magic_enum::enum_name(keyEnum);
    if (aKeysJson.HasMember(keyIdStr.data())) {
      auto keyAction = std::make_unique<KeyAction>(aKeysJson[keyIdStr.data()]);
      if (keyAction->isValid()) {
        mKeyActions[keyEnum] = std::move(keyAction);
      }
    }
  }
  return true;
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
  MemConsciousDocument extraInfo;
  auto &alloc = extraInfo.GetAllocator();
  extraInfo.SetObject();

  MemConciousValue filePath;
  filePath.SetString(mFilePath.c_str(), alloc);

  extraInfo.AddMember("file_path", filePath, alloc);

  return extraInfo;
}

bool JsonDevice::HandleKeyEvent(KeyPressAbstract::KeyEvent aEvent) {
  if (!mKeyActions.contains(aEvent.mId)) {
    return false;
  }
  // TODO should we have ExecuteAction and execute return bool?
  auto &keyHandler = mKeyActions[aEvent.mId];
  keyHandler->ExecuteAction(aEvent.mType);
  return true;
}

std::unique_ptr<UI::Page::Base> JsonDevice::GetControlPage() {
  // Return nullptr for now - this would need to be implemented based on your UI system
  // Maybe device could define page json?
  return nullptr;
}

bool JsonDevice::isValid() const {
  return mParseResult == ParseResult::Success;
}

} // namespace Json
