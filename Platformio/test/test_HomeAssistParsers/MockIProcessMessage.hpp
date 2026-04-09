#pragma once

#include <gmock/gmock.h>

#include <memory>

#include "lib/RapidJsonUtility/IProcessMessage.hpp"

namespace HAL::WebSocket::Json {

class MockIProcessMessage : public ::Json::IProcessMessage {
public:
  explicit MockIProcessMessage(std::unique_ptr<::Json::IChunkProcessor> processor)
      : ::Json::IProcessMessage(nullptr, std::move(processor)) {}

  using ::Json::IProcessMessage::GetUnProcessedBufferCapacity;
};

} // namespace HAL::WebSocket::Json
