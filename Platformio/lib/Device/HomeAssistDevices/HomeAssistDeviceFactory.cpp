#include "HomeAssistDeviceFactory.hpp"
#include "HomeAssistDevices/Light.hpp"
#include "WebSocket/Api.hpp"

namespace HomeAssist {

IDevice::Ptr HomeAssistDeviceFactory::Create(const std::string &aEntityString, WebSocket::Api &mHaApi) {
  using namespace HomeAssist::Device;
  switch (GetType(aEntityString)) {
  case EntityType::Light:
    return std::make_shared<Light>(aEntityString, mHaApi);
  default:
    return nullptr;
  }
}

HomeAssistDeviceFactory::EntityType HomeAssistDeviceFactory::GetType(const std::string &aEntityString) {
  if (aEntityString.rfind("light.", 0) == 0) {
    return EntityType::Light;
  } else if (aEntityString.rfind("switch.", 0) == 0) {
    return EntityType::Switch;
  } else if (aEntityString.rfind("sensor.", 0) == 0) {
    return EntityType::Sensor;
  } else if (aEntityString.rfind("binary_sensor.", 0) == 0) {
    return EntityType::BinarySensor;
  } else if (aEntityString.rfind("automation.", 0) == 0) {
    return EntityType::Automation;
  }
  return EntityType::Unknown;
}

} // namespace HomeAssist
