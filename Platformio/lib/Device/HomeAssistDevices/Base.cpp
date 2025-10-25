#include "HomeAssistDevices/Base.hpp"

namespace HomeAssist::Device {

Base::Base(const std::string &entityId, WebSocket::Api &api)
    : mEntityId(entityId), mApi(api) {}

rapidjson::Document Base::GetExtraConfig() const {
  rapidjson::Document doc;
  doc.SetObject();

  rapidjson::Value entityId;
  entityId.SetString(mEntityId.c_str(), doc.GetAllocator());

  doc.AddMember("entityId", entityId, doc.GetAllocator());
  return doc;
}

void Base::SetExtraConfig(const rapidjson::Document &config) {}

} // namespace HomeAssist::Device
