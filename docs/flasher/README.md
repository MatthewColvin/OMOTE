# OMOTE Rev1 web flasher (GitHub Pages)

Browser-based USB flasher for **esp32_Rev1** using [ESP Web Tools](https://esphome.github.io/esp-web-tools/).

## Live site

After you enable Pages and run the deploy workflow, the site is:

`https://<your-github-user>.github.io/OMOTE-Firmware-object-oriented/`

(Only the `docs/flasher/` folder is published as the site root.)

## One-time GitHub setup (fork)

1. **Settings → Pages → Build and deployment → Source:** **GitHub Actions**.
2. **Settings → Actions → General:** allow workflows on the fork.
3. Run **Actions → Deploy OMOTE web flasher → Run workflow** (or push to `Webui`).

## Local test (optional)

Build firmware and copy artifacts into this folder, then serve over HTTPS (Web Serial requires a secure context):

```powershell
cd Platformio
pio run -e esp32_Rev1
pio run -e esp32_Rev1 -t buildfs
$out = ".pio/build/esp32_Rev1"
$dest = "../docs/flasher"
Copy-Item "$out/bootloader.bin","$out/partitions.bin","$out/boot_app0.bin","$out/firmware.bin","$out/littlefs.bin" $dest
```

Use any local HTTPS static server, or rely on GitHub Pages.

## Flash map (Rev1Partitions.csv)

| File            | Offset   |
|-----------------|----------|
| bootloader.bin  | 0x1000   |
| partitions.bin  | 0x8000   |
| boot_app0.bin   | 0xE000   |
| firmware.bin    | 0x10000  |
| littlefs.bin    | 0x24F000 |

## Later

Add more `builds[]` entries (Rev5, ESP32-S3, etc.) when those variants are ready.
