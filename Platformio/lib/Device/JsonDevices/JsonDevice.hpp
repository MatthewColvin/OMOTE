#pragma once

#include "IDevice.hpp"
#include "JsonDevices/KeyAction.hpp"
#include "RapidJsonUtilty.hpp"

namespace Json {

class JsonDevice : public IDevice {
public:
  // TODO Need to update to use filepath here so
  // it can be stored off when we save active devices
  explicit JsonDevice(const MemConciousValue &aConfig);
  virtual ~JsonDevice() = default;

  // Core device information
  std::string GetName() const override;
  lv_color_t GetDisplayColor() const override;
  bool IsDeleteAble() const override;

  // Device identification
  DeviceType GetType() const override;
  DeviceId GetId() const override;

  void SetExtraConfig(const MemConsciousDocument &config);
  MemConsciousDocument GetExtraConfig() const;

  // Interaction handlers
  bool HandleKeyEvent(KeyPressAbstract::KeyEvent event) override;
  std::unique_ptr<UI::Page::Base> GetControlPage() override;

private:
  std::string mName;
  std::map<KeyPressAbstract::KeyId, KeyAction> mKeyActions;

  DeviceId mId;
  DeviceType mType;
};

} // namespace Json
