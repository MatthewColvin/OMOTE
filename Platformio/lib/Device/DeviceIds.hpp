#pragma once

/**
 *
 * Warning this file is strictly extend only because the persistent
 * config will directly store these enums so we can restore
 *
 */

enum class DeviceType {
  CompileTime,
  HomeAssist,
  MQTT,
  JSON
};

enum class DeviceId {
  None
};