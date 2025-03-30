#pragma once

#include <gmock/gmock.h>

#include <memory>

#include "IProcessMessage.hpp"

namespace HAL::WebSocket::Json {

class MockIProcessMessage : public IProcessMessage {
 public:
  explicit MockIProcessMessage(std::unique_ptr<IChunkProcessor> processor)
      : IProcessMessage(nullptr, std::move(processor)) {}

  using IProcessMessage::GetUnProcessedBufferCapacity;
};

}  // namespace HAL::WebSocket::Json
