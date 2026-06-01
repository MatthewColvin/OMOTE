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

If deleted scenes reappear after **firmware-only** flash, they were still on LittleFS (changes were never saved to the remote, or the editor still had the old copy). Use **Save to remote** after edits.

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

## API (device)

See previous README section — `/api/fs/*`, `/api/ir/learn/*`, `/api/device/reboot`.
