#pragma once

#include <memory>
#include <string>

#include "Hardware/KeyPressAbstract.hpp"
#include "IDevice.hpp"
#include "WebSocket/Api.hpp"
#include "WebSocket/Message/Message.hpp"

namespace HomeAssist::Device {

class Light : public IDevice {
public:
  Light(const std::string &entityId, WebSocket::Api &api);
  ~Light() override = default;

  // IDevice interface implementation
  std::string GetName() const override { return mEntityId; }
  lv_color_t GetDisplayColor() const override {
    return lv_color_make(255, 214, 0);
  }
  bool IsDeleteAble() const override { return false; }
  bool HandleKeyEvent(KeyPressAbstract::KeyEvent event) override;
  std::unique_ptr<UI::Page::Base> GetControlPage() override;

  // Light specific methods
  void TurnOn();
  void TurnOff();
  void Toggle();
  bool IsOn() const { return mIsOn; }

private:
  void SetupStateSubscription();
  void HandleStateChange(const WebSocket::Message &message);
  void SendLightCommand(const std::string &service);

  std::string mEntityId;
  WebSocket::Api &mApi;
  bool mIsOn = false;
};

} // namespace HomeAssist::Device
