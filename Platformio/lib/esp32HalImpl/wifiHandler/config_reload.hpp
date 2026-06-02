#pragma once

namespace config_reload {

/** Set when page JSON on LittleFS changes (editor deploy). */
void markPagesDirty();

/** Set when HaSettings.json changes (no full scene rebuild needed). */
void markHaSettingsDirty();

/** Set when DeviceSettings.json changes (sleep/brightness apply without reboot). */
void markDeviceSettingsDirty();

/** Set when DeviceSettings.schema.json changes (rebuild settings UI). */
void markDeviceSettingsSchemaDirty();

/** Returns true once per mark; JsonUI should reload the active scene. */
bool consumePagesDirty();

/** Returns true once per mark; JsonUI should reload HA credentials from disk. */
bool consumeHaSettingsDirty();

/** Returns true once per mark; apply sleep/brightness from DeviceSettings.json. */
bool consumeDeviceSettingsDirty();

bool consumeDeviceSettingsSchemaDirty();

} // namespace config_reload
