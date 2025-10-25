#pragma once

#include <string>

#include "RapidJsonUtilty.hpp"
#include "rapidjson/document.h"

namespace HomeAssist::WebSocket {

class Request {
public:
  Request(rapidjson::Document &&aRequestMessage); // Changed constructor
  ~Request() = default;

  std::string GetRequestMessage();

  void SetId(int aId);

private:
  rapidjson::Document mRequestMessage;
};

inline Request::Request(rapidjson::Document &&aRequestMessage)
    : mRequestMessage(std::move(aRequestMessage)) {}

inline std::string Request::GetRequestMessage() {
  return ToString(mRequestMessage);
}

inline void Request::SetId(int aId) {
  auto &alloc = mRequestMessage.GetAllocator();
  if (!mRequestMessage.HasMember("id")) {
    mRequestMessage.AddMember("id", rapidjson::Value().SetInt(aId), alloc);
  }
  mRequestMessage["id"] = aId;
}

} // namespace HomeAssist::WebSocket
