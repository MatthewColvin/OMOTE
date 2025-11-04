#pragma once

#include "IProcessMessage.hpp"
#include "WebSocket/Api.hpp"
#include "rapidjson/document.h"

namespace HomeAssist::WebSocket {

class ResponseHandler : public Json::IProcessMessage {
public:
  ResponseHandler(HomeAssist::WebSocket::Api &aApi);
  virtual ~ResponseHandler();

  bool ProcessResponseDoc(const rapidjson::Document &aDoc);

  /**
   * @return true - chunk processor in a session handled the doc
   *         fasle - session didn't want the doc processed that way
   */
  bool HandleRedirectToChunkProcessor(const rapidjson::Document &aDoc);

private:
  HomeAssist::WebSocket::Api &mApi;
};

} // namespace HomeAssist::WebSocket