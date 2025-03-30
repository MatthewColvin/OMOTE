#pragma once

#include "../IDevice.hpp"
#include "WebSocket/Api.hpp"
#include <memory>
#include <string>

namespace HomeAssist {

class HomeAssistDeviceFactory {
public:
  static IDevice::Ptr Create(const std::string &aEntityString, WebSocket::Api &mHaApi);

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
  HomeAssistDeviceFactory() = default;
};

} // namespace HomeAssist
