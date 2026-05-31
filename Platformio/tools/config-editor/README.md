# OMOTE Config Editor (local)

Browser editor for the official OMOTE OO firmware. Runs on your PC — the remote only exposes a small JSON + IR API.

## Quick start

```powershell
cd Platformio/tools/config-editor
python -m http.server 8080
```

Open http://localhost:8080 → connect to `http://omote.local` → use **My setup**.

## Beginner flow

1. **My setup** — create an **activity** (e.g. “Watch TV”). Each activity is what you pick on the remote home screen.
2. **Add device** — each device becomes a **tab** at the bottom of the touch screen (TV, Roku, Receiver…). Pick a template or start blank.
3. **Configure remote** — one screen with:
   - **Touch screen** preview (top) — on-screen buttons
   - **Physical remote face** (bottom) — matches the real OMOTE layout (same as greenfield editor)
4. Click any touch button or physical key → **Learn from another remote** or pick an existing command.
5. **Save to remote & reboot** when done.

The editor **auto-creates and links** the JSON files (`Pages/…`, `Commands/…`, scene entries). You don’t need to know the file structure.

## Advanced mode

Toggle **Advanced** in the header to show:

- Linked file paths
- Raw **Commands** table
- **JSON** editor for any file
- Press types (long/repeat) on physical keys
- Activity start/exit power sequences

## Device templates

| Template | What you get |
|----------|----------------|
| TV | Source button, color keys, number pad, default D-pad/volume maps |
| Streaming / Roku | Copies stock Roku page + IR codes if present on device |
| AV receiver | Stock receiver page |
| Blank | Empty page — learn every button yourself |

## API (device)

See previous README section — `/api/fs/*`, `/api/ir/learn/*`, `/api/device/reboot`.
