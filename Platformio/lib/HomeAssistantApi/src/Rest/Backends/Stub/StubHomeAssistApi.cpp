#include "Rest/Backends/Stub/StubHomeAssistApi.hpp"

namespace HomeAssist {

std::string StubHomeAssistApi::SendUpdate(const std::string & /*anApiPath*/,
                                          const std::string & /*aUpdateJson*/) {
  return R"({"message":"API running."})";
}

std::string StubHomeAssistApi::SendRequest(const std::string & /*anApiPath*/) {
  return R"({"message":"API running."})";
}

} // namespace HomeAssist
