#include "JsonDevices/JsonDevice.hpp"
#include "magic_enum.hpp"
#include <fstream>
#include <sstream>

namespace Json {

JsonDevice::JsonDevice(std::filesystem::path aDeviceJsonFilePath) {
  constexpr auto maxDeviceFileSize = 5000;

  if (std::filesystem::file_size(aDeviceJsonFilePath) > maxDeviceFileSize) {
    mParseResult = ParseResult::FileError;
    return;
  }
  std::ifstream deviceJsonStream(aDeviceJsonFilePath);
  if (!deviceJsonStream.is_open()) {
    mParseResult = ParseResult::FileError;
    return;
  }

  std::stringstream deviceSS;
  deviceSS << deviceJsonStream.rdbuf();
  auto deviceJsonStr = deviceSS.str();
  rapidjson::Document deviceJson;
  deviceJson.Parse(deviceJsonStr.c_str());
  mParseResult = parse(deviceJson);

  if (mParseResult == ParseResult::Success) {
    mFilePath = aDeviceJsonFilePath;
  }
}

JsonDevice::ParseResult JsonDevice::parse(const rapidjson::Document &aDeviceJson) {
  if (!parseGeneralInfo(aDeviceJson)) {
    return ParseResult::GeneralInfoError;
  }
  if (aDeviceJson.HasMember("keys") && !parseKeyActions(aDeviceJson["keys"])) {
    return ParseResult::KeyActionsError;
  }
  return ParseResult::Success;
}

bool JsonDevice::parseGeneralInfo(const rapidjson::Document &aDeviceJson) {
  // Todo add color info and checks
  mName = aDeviceJson["name"].GetString();
  mId = DeviceId::None;
  return true;
}

bool JsonDevice::parseKeyActions(const rapidjson::Value &aKeysJson) {
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

rapidjson::Document JsonDevice::GetExtraConfig() const {
  rapidjson::Document extraInfo;
  auto &alloc = extraInfo.GetAllocator();
  extraInfo.SetObject();

  rapidjson::Value filePath;
  const char *filePathStr = mFilePath.string().c_str();
  filePath.SetString(filePathStr, alloc);

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
