#pragma once

#include "IDevice.hpp"
#include "WebSocket/Api.hpp"

namespace HomeAssist::Device {

class Base : public IDevice {
public:
  Base(const std::string &entityId, WebSocket::Api &api);
  ~Base() override = default;

  // IDevice interface implementation
  std::string GetName() const override { return mEntityId; }
  bool IsDeleteAble() const override { return false; }
  DeviceType GetType() const override { return DeviceType::HomeAssist; }
  DeviceId GetId() const override { return DeviceId::None; }

  rapidjson::Document GetExtraConfig() const override;
  void SetExtraConfig(const rapidjson::Document &config) override;

protected:
  std::string mEntityId;
  WebSocket::Api &mApi;
};

} // namespace HomeAssist::Device
