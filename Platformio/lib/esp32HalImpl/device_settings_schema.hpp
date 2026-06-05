#pragma once

#include <rapidjson/document.h>

namespace device_settings_schema {

/** Loaded from /littlefs/DeviceSettings.schema.json */
const rapidjson::Document &document();
bool loadFromLittleFS();
bool isLoaded();

} // namespace device_settings_schema
