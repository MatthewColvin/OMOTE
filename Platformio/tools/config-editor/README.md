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
