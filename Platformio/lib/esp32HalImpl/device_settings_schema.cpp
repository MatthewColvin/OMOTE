#include "device_settings_schema.hpp"

#include "RapidJsonUtilty.hpp"

#ifndef FS_PATH
#define FS_PATH "/littlefs/"
#endif

namespace device_settings_schema {

namespace {
rapidjson::Document sSchema;
bool sLoaded = false;
} // namespace

const rapidjson::Document &document() { return sSchema; }

bool isLoaded() { return sLoaded; }

bool loadFromLittleFS() {
  const auto doc = OMOTE::JSON::GetDocument(
      std::filesystem::path(FS_PATH "DeviceSettings.schema.json"));
  if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("sections")) {
    sLoaded = false;
    return false;
  }
  sSchema.CopyFrom(doc, sSchema.GetAllocator());
  sLoaded = true;
  return true;
}

} // namespace device_settings_schema
