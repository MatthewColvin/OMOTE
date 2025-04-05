#pragma once

#include "HomeAssistDevices/Base.hpp"
#include "WebSocket/Message/Message.hpp"

namespace HomeAssist::Device {

class Light : public Base {
public:
  Light(const std::string &entityId, WebSocket::Api &api);
  ~Light() override = default;

  // IDevice interface implementation
  lv_color_t GetDisplayColor() const override {
    return lv_color_make(255, 214, 0);
  }
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

  bool mIsOn = false;
};

} // namespace HomeAssist::Device
