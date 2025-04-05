#pragma once

#include "../IDevice.hpp"
#include "WebSocket/Api.hpp"
#include <memory>
#include <string>

namespace HomeAssist {

class HomeAssistDeviceFactory {
public:
  explicit HomeAssistDeviceFactory(WebSocket::Api &aHaApi) : mHaApi(aHaApi) {}

  IDevice::Ptr Create(const std::string &aEntityString);

  enum class EntityType : uint8_t {
    Light,
    Button,
    Sensor,
    BinarySensor,
    Automation,
    Switch,
    Unknown
  };

  static EntityType GetType(const std::string &aEntityString);

private:
  WebSocket::Api &mHaApi;
};

} // namespace HomeAssist
