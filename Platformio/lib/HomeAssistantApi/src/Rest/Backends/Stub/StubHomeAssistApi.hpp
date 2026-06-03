#pragma once

#include "Rest/Access/IHomeAssistApi.hpp"

namespace HomeAssist {

/** No-op HA client for the desktop simulator (no libcurl required). */
class StubHomeAssistApi : public IHomeAssistApi {
public:
  std::string SendUpdate(const std::string &anApiPath,
                         const std::string &aUpdateJson) override;
  std::string SendRequest(const std::string &anApiPath = "") override;
};

} // namespace HomeAssist
