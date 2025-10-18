#pragma once

#include "IDevice.hpp"
#include "JsonDevices/KeyAction.hpp"
#include "RapidJsonUtilty.hpp"

#include <filesystem>

namespace Json {

class JsonDevice : public IDevice {
public:
  enum class ParseResult {
    Success,
    GeneralInfoError,
    KeyActionsError,
    FileError,
    Invalid
  };

  explicit JsonDevice(std::filesystem::path aDeviceJsonFilePath);
  virtual ~JsonDevice() = default;

  // Core device information
  std::string GetName() const override;
  lv_color_t GetDisplayColor() const override;
  bool IsDeleteAble() const override;

  // Device identification
  DeviceType GetType() const override;
  DeviceId GetId() const override;

  // Data used on bootup to restore json device
  MemConsciousDocument GetExtraConfig() const override;

  // Interaction handlers
  bool HandleKeyEvent(KeyPressAbstract::KeyEvent event) override;
  std::unique_ptr<UI::Page::Base> GetControlPage() override;

  bool isValid() const;

protected:
  ParseResult parse(const MemConsciousDocument &aJson);
  bool parseGeneralInfo(const MemConsciousDocument &aJson);
  bool parseKeyActions(const MemConciousValue &aKeysJson);

private:
  std::string mName;
  std::filesystem::path mFilePath;
  std::map<KeyPressAbstract::KeyId, std::unique_ptr<KeyAction>> mKeyActions;

  DeviceId mId;
  DeviceType mType;

  ParseResult mParseResult = ParseResult::Invalid;
};

} // namespace Json
