# OMOTE Config Editor (local)

Browser editor for the official OMOTE OO firmware. Runs on your PC — the remote only exposes a small JSON + IR API.

## Quick start

```powershell
cd Platformio/tools/config-editor
python -m http.server 8080
```

Open http://localhost:8080 → connect to `http://omote.local` → use **Scenes**.

## Beginner flow

1. **Scenes** — pick or create a scene (e.g. “Watch TV”, “Watch Roku on Proj”). This is the same list as tapping the **top-left** of the remote status bar.
2. **Device tabs** — within the scene, add devices (TV, Roku, receiver…). Each becomes a **tab** at the bottom of the touch screen.
3. **Remote** — configure touch buttons and physical keys for the selected scene + device tab.
4. Click any touch button or physical key → **Learn from another remote** or pick an existing command.
5. **Save to remote & reboot** when done.

The editor **auto-creates and links** the JSON files (`Pages/…`, `Commands/…`, scene entries). You don’t need to know the file structure.

On the **Remote** tab, the layout is four columns: **device tabs** → **remote preview** → **widget editor** → **page commands (IR)** (far right). In the commands column, pick the command file for the active tab, assign names per button / color key / numpad digit, and **Learn** IR. Use **Full editor…** for the complete command table (Advanced).

**Settings** tab: choose **Stock OMOTE** or **3661 extended keys** under Physical remote, and configure Home Assistant. The 3661 layout matches custom Rev5 hardware (`OMOTE_KEYBRD_3661`). PCB choice is saved in the browser (not on the remote).

## Backup (.omote)

On the **Connect** tab (or **Export backup** in the footer):

- **Export .omote backup** — zips every JSON config file into one `.omote` file (same layout as on the remote: `Scenes.json`, `Pages/…`, `Commands/…`, etc.).
- **Import .omote** — loads that archive into the editor for offline editing or to copy a setup to another remote. Nothing is sent to the device until you click **Save to remote & reboot**.

When you **Connect & load** while already editing (e.g. after importing a backup), the editor compares your files to the remote. If they differ, you choose **Keep editor config**, **Load from remote**, or **Cancel** — your offline work is never silently overwritten.

Use this before a risky deploy, when the remote is offline, or to clone config to a second OMOTE.

## Firmware vs config on the remote

| Command | What it updates |
|---------|-----------------|
| `pio run -e esp32_Rev1 -t upload` | **Firmware only** (app partition) |
| `pio run -e esp32_Rev1 -t uploadfs` | **LittleFS only** — overwrites the remote with files from `Platformio/data/` |

JSON scenes/pages live on **LittleFS**, not inside the firmware binary. A normal firmware flash does **not** restore `Platformio/data/` to the device.

Scenes use two layers on LittleFS:

1. **`Scenes.json`** — registry; only these appear on the remote scene picker (status bar tap).
2. **`Scenes/Scene_*.json`** — scene definitions. A file can remain on disk after you “delete” a scene if it was only removed from the registry.

**Delete scene** (Advanced) removes the entry from `Scenes.json` and drops the scene file from the editor; **Save to remote** writes both and deletes the file on the device. If old scenes reappear in the web app after connect, leftover `Scenes/Scene_*.json` files were still on the device — use **Clean up unregistered scene files** (Advanced) then **Save to remote**.

If deleted scenes reappear after **firmware-only** flash, they were still on LittleFS (`uploadfs` restores factory `data/`, or files were never deleted on device). Use **Save to remote** after cleanup — not `uploadfs` unless you intend to reset all config.

Avoid `uploadfs` unless you intentionally want to reset the remote to the repo’s bundled `data/` folder.

## Home Assistant

On **Connect**: enter HA URL + long-lived token → **Save HA settings** (writes `HaSettings.json`) → **Test connection**.

On **Remote**: add Home Assistant widgets (toggle, label, switch, slider, momentary, climate). Entity lists load from HA **in your browser** (not via the remote). The canvas polls entity state every 8s for live preview.

Widget JSON on pages:

```json
{ "Type": "HaToggle", "Text": "Kitchen", "EntityId": "light.kitchen", "Domain": "light", "Service": "toggle", "HeightPct": 10, "AlignTo": 0 }
{ "Type": "HaLabel", "Text": "—", "EntityId": "sensor.temperature", "HeightPct": 8, "AlignTo": 1 }
{ "Type": "HaSwitch", "Text": "Fan", "EntityId": "switch.fan", "Domain": "switch", "ServiceOn": "turn_on", "ServiceOff": "turn_off", "HeightPct": 10, "AlignTo": 0 }
{ "Type": "HaSlider", "Text": "Brightness", "EntityId": "light.kitchen", "Domain": "light", "Service": "turn_on", "Attribute": "brightness", "Min": 0, "Max": 255, "HeightPct": 10, "AlignTo": 1 }
{ "Type": "HaMomentary", "Text": "Garage", "EntityId": "switch.garage", "Domain": "switch", "ServiceOn": "turn_on", "ServiceOff": "turn_off", "HeightPct": 12, "AlignTo": 2 }
{ "Type": "HaClimate", "EntityId": "climate.living_room", "HeightPct": 58, "AlignTo": 0 }
```

Deploy pages and `HaSettings.json` with **Save to remote**.

## Advanced mode

Toggle **Advanced** in the header to show:

- Linked file paths
- Raw **Commands** table
- **JSON** editor for any file
- Press types (long/repeat) on physical keys
- Scene start/exit power sequences
- Physical key shortcuts (TV / Stream / BluRay keys)

## Device templates

| Template | What you get |
|----------|----------------|
| TV | Source button, color keys, number pad, default D-pad/volume maps |
| Streaming / Roku | Copies stock Roku page + IR codes if present on device |
| AV receiver | Stock receiver page |
| Blank | Empty page — learn every button yourself |

---

# Developer guide: schema-driven device settings

This section is for firmware and tooling developers who need to **add a new setting**, expose it in the **web editor**, and have it work on the **on-device Settings UI** without building a new LVGL page from scratch.

## Overview

Device preferences use a **two-file contract** on LittleFS:

| File | Path on device | Purpose |
|------|----------------|---------|
| **`DeviceSettings.schema.json`** | `/littlefs/DeviceSettings.schema.json` | **What** to show: sections, labels, control types, defaults, menu placement |
| **`DeviceSettings.json`** | `/littlefs/DeviceSettings.json` | **Values only** (user/editor state) |

The same pair exists in the repo for factory defaults:

- `Platformio/data/DeviceSettings.schema.json`
- `Platformio/data/DeviceSettings.json`

Three surfaces read the schema:

| Surface | Implementation |
|---------|----------------|
| **Web editor** (Settings tab) | `tools/config-editor/settings-form.js` → `OmoteSettingsForm.renderDeviceSettingsForm()` |
| **On-device Settings → Device** (and menu sections) | `lib/BasicUI/page/SystemSettings.cpp` |
| **HTTP API** | `GET /api/device/settings/schema`, `GET/POST /api/device/settings` in `config_http.cpp` |

Runtime state lives in firmware here:

- `lib/esp32HalImpl/device_settings.hpp` — `device_settings::Settings` struct
- `lib/esp32HalImpl/device_settings.cpp` — load/merge/save/apply

**Important:** The schema alone does **not** make a setting work. Every new `key` must be wired in C++ (`mergeFromJson`, `applyToHardware`, and usually `toJsonDocument` / `saveToLittleFS`). The schema only drives UI generation.

```mermaid
flowchart LR
  subgraph disk [LittleFS]
    Schema[DeviceSettings.schema.json]
    Values[DeviceSettings.json]
  end
  subgraph fw [Firmware]
    Merge[device_settings::mergeFromJson]
    Apply[device_settings::applyToHardware]
    UI[SystemSettings + SettingsPage]
  end
  subgraph editor [Web editor]
    Form[settings-form.js]
  end
  Schema --> UI
  Schema --> Form
  Values --> Merge
  Form -->|POST /api/device/settings| Merge
  Merge --> Apply
  UI -->|user edits| Merge
  Merge --> Values
```

## What is still *not* schema-driven

These Settings entries are **hard-coded** in `SettingsPage.cpp` and use dedicated pages or hardware flows:

| Menu item | Why separate |
|-----------|----------------|
| **Backlight** | `DisplaySettings.cpp` — custom slider layout (also overlaps “Backlight” fields inside **Device**) |
| **Device** | `SystemSettings` with all `placement: "submenu"` sections |
| **Wifi** | `WifiSettings.cpp` — scan, connect, credentials in **ESP Preferences**, not `DeviceSettings.json` |
| **Logging** | `LoggingSettings.cpp` |
| **Battery** | `LearnBattery.cpp` |
| **IR Receiver** | `IrLearner.cpp` |

MQTT / NTP / FTP were migrated to the schema as `placement: "menu"` sections. WiFi join flow will likely stay separate until someone models scan/SSID in JSON.

## Settings menu layout (on device)

`SettingsPage` builds the list in this order:

1. Backlight (fixed)
2. Device (fixed — all `submenu` sections)
3. **Dynamic** — one row per schema section with `"placement": "menu"` (order = array order in schema)
4. Wifi, Logging, Battery, IR Receiver (fixed)

The Settings list is built when `SettingsPage` is **constructed** (first time you open Settings in a session). If you change `DeviceSettings.schema.json` on disk via the editor, menu rows update after **reboot** or after leaving Settings and reconstructing the page — not instantly while Settings is already open.

## Schema file format

Top-level object:

```json
{
  "version": 1,
  "title": "Device settings",
  "sections": [ /* ... */ ]
}
```

| Top-level key | Required | Description |
|---------------|----------|-------------|
| `version` | recommended | Informational; not heavily validated today |
| `title` | optional | Popup title for **Device** (`SystemSettings` with no section filter) |
| `sections` | **yes** | Array of section objects |

### Section object

| Key | Required | Description |
|-----|----------|-------------|
| `id` | **yes** | Stable identifier (`sleep`, `mqtt`, …). Used for menu filtering and icon fallbacks |
| `title` | recommended | Heading inside forms |
| `fields` | **yes** | Array of field objects |
| `placement` | optional | `"submenu"` (default) or `"menu"` — see below |
| `hint` | optional | Muted helper text under the section title |
| `menu_title` | optional | Settings list label when `placement` is `"menu"` (defaults to `title`) |
| `menu_icon` | optional | LVGL symbol name for menu rows — see icon table below |

**`placement`**

| Value | On-device behavior | Web editor |
|-------|-------------------|------------|
| `"submenu"` (default) | Fields appear under **Settings → Device** | Shown in collapsible section |
| `"menu"` | Own row in main **Settings** list; opens `SystemSettings(sectionId)` | Same section, collapsible + **Edit** popup |

### Field object

| Key | Required | Used by types |
|-----|----------|-------------|
| `key` | **yes** | JSON key in `DeviceSettings.json` — use **snake_case** |
| `type` | **yes** | `boolean`, `choice`, `slider`, `string` |
| `label` | recommended | UI label on device and editor |
| `default` | recommended | Default when key missing from values file |
| `options` | for `choice` and string presets | Array of `{ "label", "value" }` |
| `min`, `max` | for `slider` | Integer range (device uses `int32_t`) |

### Field types (behavior)

| `type` | Web editor | On-device UI | Value in JSON |
|--------|------------|--------------|---------------|
| `boolean` | Checkbox | Toggle switch | `true` / `false` |
| `choice` | `<select>` | Dropdown | **Number** (e.g. timeout ms) |
| `slider` | Number input | Slider | Number |
| `string` | Text input | Button → keyboard | String |
| `string` + `options` | `<select>` | Dropdown | String (e.g. NTP server) |

**Hints**

- **`choice` values must be numeric** on the device (`patchInt`). Use milliseconds, enum integers, etc.
- **`string` with `options`** is for human-readable presets (NTP host, timezone POSIX string). Custom values not in the list are still shown on device if already saved.
- **Brightness sliders** (`lcd_*`, `kbd_*`): value `0` means “do not change” in `applyToHardware` (only applies when ≥ 10). Schema hint documents this for the display section.

### `menu_icon` names

Mapped in `SettingsPage.cpp` to LVGL symbols:

| `menu_icon` | Symbol |
|-------------|--------|
| `wifi` | WiFi |
| `home` | Home (MQTT default) |
| `refresh` | Refresh (NTP default) |
| `directory` | Directory (FTP default) |
| `list` | List |
| `battery`, `battery_3` | Battery |
| `eye_open` | Eye open |
| `settings` | Gear |
| `charge`, `usb`, `close` | As named |

If `menu_icon` is omitted, `mqtt` / `ntp` / `ftp` still get icons by `id` fallback.

---

## End-to-end checklist: add a new setting

Use this every time you add a user-visible preference.

### 1. Choose the `key` and section

- Pick a unique **snake_case** key (e.g. `beep_on_wake`).
- Add the field to an existing section in `DeviceSettings.schema.json`, or create a new section.
- Set `default` in the schema **and** add the key to `Platformio/data/DeviceSettings.json` for factory `uploadfs`.

### 2. Extend firmware state

In `lib/esp32HalImpl/device_settings.hpp`:

- Add a member to `device_settings::Settings`.

### 3. Load and save JSON

In `lib/esp32HalImpl/device_settings.cpp`:

| Function | What to add |
|----------|-------------|
| `mergeFromJson()` | Read your key from the posted/loaded JSON object |
| `toJsonDocument()` | Write your key (for API GET and UI readback) |
| `saveToLittleFS()` | Same keys as `toJsonDocument()` (keep in sync) |

Unknown keys in JSON are **silently ignored** until you add merge support.

### 4. Apply to hardware

In `applyToHardware()`:

- Call the right HAL / wifi / display API when the value changes.
- If the setting is read-only at runtime, implement `syncFromHardware()` too (used when `DeviceSettings.json` is missing on boot).

### 5. Schema UI only (no extra UI code)

If you only added schema + merge/apply:

- **Web editor** picks it up automatically on refresh.
- **Device → SystemSettings** renders it from type.
- **Menu section** only if you set `"placement": "menu"` **and** rebuild/reopen Settings.

### 6. Deploy and test

| Step | Command / action |
|------|------------------|
| Firmware changed | `pio run -e esp32_Rev1 -t upload` |
| JSON defaults changed | `pio run -e esp32_Rev1 -t uploadfs` **or** editor **Save device settings** |
| Editor only | Save to remote — no reboot for values; schema reloads on device via `config_reload` |

**Test matrix**

1. Change value on device → leave page → reboot → value persisted in `DeviceSettings.json`.
2. **Pull from remote** in editor → value matches.
3. Change in editor → **Save device settings** → device behavior updates without full scene reboot.
4. If MQTT/NTP/FTP: confirm legacy files (`mqtt.json`, `ntp.json`, `ftp.json`) still update when expected.

---

## Worked example A: boolean in Device submenu

**Goal:** “Beep on wake” toggle under sleep settings.

**1. Schema** — add to `sleep` section in `DeviceSettings.schema.json`:

```json
{
  "key": "beep_on_wake",
  "type": "boolean",
  "label": "Beep on wake",
  "default": false
}
```

**2. Factory values** — `Platformio/data/DeviceSettings.json`:

```json
"beep_on_wake": false
```

**3. C++** (illustrative):

```cpp
// device_settings.hpp
bool beepOnWake = false;

// mergeFromJson
if (doc.HasMember("beep_on_wake") && doc["beep_on_wake"].IsBool())
  sSettings.beepOnWake = doc["beep_on_wake"].GetBool();

// toJsonDocument + saveToLittleFS
d.AddMember("beep_on_wake", sSettings.beepOnWake, a);

// applyToHardware
hw.buzzer()->setWakeBeep(sSettings.beepOnWake);  // your API
```

No changes to `SystemSettings.cpp` if type is `boolean`.

---

## Worked example B: numeric choice (timeout)

Already used for `display_timeout_ms`:

```json
{
  "key": "display_timeout_ms",
  "type": "choice",
  "label": "Screen off after",
  "options": [
    { "label": "1 min", "value": 60000 },
    { "label": "10 min", "value": 600000 }
  ],
  "default": 60000
}
```

`mergeFromJson` uses `tryGetUint32(doc, "display_timeout_ms", ...)`.  
`applyToHardware` calls `hw.setSleepTimeout(sSettings.displayTimeoutMs)`.

**Business rule:** `clampDeepSleep()` forces `deep_sleep_timeout_ms >= display_timeout_ms + 60000`. If you add related timeouts, reuse or extend that helper.

---

## Worked example C: new top-level Settings menu (like MQTT)

**Goal:** “Audio” section as its own Settings row.

```json
{
  "id": "audio",
  "placement": "menu",
  "menu_title": "Audio",
  "menu_icon": "settings",
  "title": "Audio",
  "fields": [
    {
      "key": "beep_volume",
      "type": "slider",
      "label": "Beep volume",
      "min": 0,
      "max": 100,
      "default": 50
    }
  ]
}
```

Requires full checklist: struct field `beepVolume`, merge, apply, etc.  
User opens **Settings → Audio** (not under Device).

---

## Worked example D: string with presets (NTP-style)

```json
{
  "key": "ntp_server",
  "type": "string",
  "label": "Server",
  "default": "pool.ntp.org",
  "options": [
    { "label": "pool.ntp.org", "value": "pool.ntp.org" },
    { "label": "Google", "value": "time.google.com" }
  ]
}
```

- Editor: dropdown in `settings-form.js` (`appendStringOptions`).
- Device: dropdown in `SystemSettings.cpp`; free-text still available via keyboard if you omit `options`.

POSIX timezone strings for `timezone` use the same pattern — see existing NTP section in schema.

---

## Runtime lifecycle (firmware)

| When | What happens |
|------|----------------|
| **Boot** (`HardwareRevX.cpp`) | Load schema + `DeviceSettings.json`; if JSON OK → `applyToHardware()`, else `syncFromHardware()` from current HW |
| **User edits on device** | `SystemSettings` patches JSON via `mergeFromJson` + `applyToHardware`; saves on page destroy |
| **Editor POST** | `config_http.cpp` → merge, apply, save, `markDeviceSettingsDirty()` |
| **Main loop** (`JsonUI.cpp`) | Consumes dirty flags → reload JSON/schema and re-apply |

Leaving a `SystemSettings` popup runs destructor save:

```cpp
device_settings::saveToLittleFS();
HardwareFactory::getAbstract().saveSettings();  // NVS preferences (non-JSON)
```

---

## Web editor integration

| File | Role |
|------|------|
| `settings-form.js` | Renders schema; collapsible sections; per-section **Edit** modal |
| `app.js` | `loadDeviceSettingsSchemaDoc()`, save/pull/push, `deviceSettingsFilePayload()` strips API-only fields |

**Save device settings** (Settings tab):

1. Collects form → `DeviceSettings.json` in editor memory.
2. `POST /api/device/settings` with values object.
3. Optionally re-reads from device FS.

**Pull from remote**: loads schema from `/api/device/settings/schema` or FS read, then values.

**Bundled fallback**: if schema file missing locally, `DEFAULT_DEVICE_SETTINGS_SCHEMA` in `settings-form.js` is minimal — ship updates in `Platformio/data/` for full layout.

---

## HTTP API (device)

| Endpoint | Method | Body / response |
|----------|--------|-----------------|
| `/api/device/settings/schema` | GET | Full schema JSON |
| `/api/device/settings` | GET | Values + extras (`wifi_connected`, `display_off`, …) |
| `/api/device/settings` | POST | JSON object of keys to merge (partial patch OK) |
| `/api/fs/read?path=DeviceSettings.json` | GET | Raw file |
| `/api/fs/write` | POST | Raw file (marks dirty flags) |

Editor should prefer **POST /api/device/settings** so `applyToHardware()` runs immediately.

---

## Dual persistence warning (MQTT / NTP / FTP)

`applyToHardware()` still writes **legacy** sidecar files:

| Keys | Also written to |
|------|-----------------|
| MQTT fields | `mqtt.json` + Preferences (`MqttSettings` enabled flag) |
| NTP fields | `ntp.json` |
| FTP fields | `ftp.json` |

`DeviceSettings.json` is the **editor-first** source of truth for those values when using the schema UI. If you add new network settings, decide whether they live only in `DeviceSettings.json` or also need a legacy file for backward compatibility.

---

## Warnings and pitfalls

### Schema / firmware mismatch

| Mistake | Symptom |
|---------|---------|
| Added field to schema only | UI shows control but changing it does nothing or reverts |
| Added merge but not `applyToHardware` | Value saves to JSON but hardware unchanged |
| Typo in `key` | Silent ignore on merge; editor and device disagree |
| `choice` with string `value` in JSON | Device dropdown may not match; use numbers for `choice` |

### Flash / filesystem

| Mistake | Symptom |
|---------|---------|
| `uploadfs` on a customized remote | **Wipes** scenes and user JSON; restores repo `data/` only |
| Firmware-only flash expecting new defaults | Old `DeviceSettings.json` on LittleFS unchanged |
| Forgetting `uploadfs` after adding new keys to bundled JSON | New devices OK; dev board missing factory defaults until pull/save |

### UI / UX

| Topic | Note |
|-------|------|
| Settings menu order | Fixed items + schema `sections` array order |
| Schema hot-reload | Values reload in loop; **Settings menu rows** need Settings page rebuild |
| Duplicate backlight | **Backlight** page vs sliders in **Device** — intentional overlap for now |
| Label height | `Label` widgets use content height; do not set `lv_pct(100)` on field labels |
| WiFi credentials | Not in `DeviceSettings.json`; still **Wifi** page + NVS |

### Validation

There is **no JSON Schema validator** at runtime. Invalid types are skipped or mis-parsed. Test on device after every schema change.

---

## File reference (quick)

| Path | Purpose |
|------|---------|
| `Platformio/data/DeviceSettings.schema.json` | Shipped schema |
| `Platformio/data/DeviceSettings.json` | Shipped default values |
| `lib/esp32HalImpl/device_settings.hpp` | Settings struct |
| `lib/esp32HalImpl/device_settings.cpp` | Merge, apply, save |
| `lib/esp32HalImpl/device_settings_schema.cpp` | Load schema from LittleFS |
| `lib/BasicUI/page/SystemSettings.cpp` | Dynamic form UI |
| `lib/BasicUI/page/SettingsPage.cpp` | Settings menu + menu sections |
| `lib/esp32HalImpl/wifiHandler/config_http.cpp` | REST API |
| `lib/JsonConfigUI/JsonUI.cpp` | Hot-reload dirty flags |
| `lib/esp32HalImpl/HardwareRevX.cpp` | Boot load/apply |
| `tools/config-editor/settings-form.js` | Editor form renderer |
| `tools/config-editor/app.js` | Editor save/pull/push |

---

## Current keys (reference)

As of the schema in repo — always verify `device_settings.hpp` and `mergeFromJson()` for the authoritative list:

| Key | Section | Notes |
|-----|---------|-------|
| `display_timeout_ms` | sleep | Screen off delay |
| `dim_lead_ms` | sleep | Dim before off |
| `deep_sleep_timeout_ms` | sleep | Clamped vs display timeout |
| `motion_wake_enabled` | sleep | IMU wake |
| `key_wake_enabled` | sleep | Key wake |
| `light_sleep_enabled` | sleep | Scene light sleep |
| `light_sleep_timeout_ms` | sleep | |
| `lcd_day_brightness` | display | 0 = keep saved |
| `lcd_night_brightness` | display | |
| `kbd_day_brightness` | display | |
| `kbd_night_brightness` | display | |
| `mqtt_*` | mqtt | menu section |
| `ntp_*`, `timezone` | ntp | menu section; string presets |
| `ftp_*` | ftp | menu section |

---

## API (device) — summary

`/api/fs/*`, `/api/ir/learn/*`, `/api/device/settings` (GET/POST), `/api/device/settings/schema` (GET), `/api/device/reboot`, `/api/device/sync-mode` (editor sync).

---

## Related docs

- Scene/page JSON: use **Scenes** / **Remote** tabs in this editor; different file layout (`Pages/`, `Commands/`).
- HA: `HaSettings.json` — separate from device settings.
- Editor sync mode: holds UI on sync page; reduces MQTT/FTP/NTP churn while connected to config editor.

When in doubt: **schema = UI**, **device_settings.cpp = behavior**, **DeviceSettings.json = stored values**.
