#include "HomeAssistDevices/Base.hpp"

namespace HomeAssist::Device {

Base::Base(const std::string &entityId, WebSocket::Api &api)
    : mEntityId(entityId), mApi(api) {}

MemConsciousDocument Base::GetExtraConfig() const {
  MemConsciousDocument doc;
  doc.SetObject();

  MemConciousValue entityId;
  entityId.SetString(mEntityId.c_str(), doc.GetAllocator());

  doc.AddMember("entityId", entityId, doc.GetAllocator());
  return doc;
}

void Base::SetExtraConfig(const MemConsciousDocument &config) {}

} // namespace HomeAssist::Device
