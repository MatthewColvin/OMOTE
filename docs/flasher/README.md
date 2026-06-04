# OMOTE Rev1 web flasher (GitHub Pages)

Browser-based USB flasher for **esp32_Rev1** using [ESP Web Tools](https://esphome.github.io/esp-web-tools/).

## Live site

After you enable Pages and run the deploy workflow, the site is:

`https://<your-github-user>.github.io/OMOTE-Firmware-object-oriented/`

(Only the `docs/flasher/` folder is published as the site root.)

## One-time GitHub setup (fork)

1. **Settings → Pages → Build and deployment → Source:** **GitHub Actions**.
2. **Settings → Actions → General:** allow workflows on the fork.
3. **Allow `Webui` to deploy** (required if your default branch is not `Webui`):
   - **Settings → Environments** → click **`github-pages`**
   - Under **Deployment branches and tags**, open the dropdown (often says only `main` or “Selected branches”)
   - Choose **Add deployment branch rule** → type **`Webui`** → save  
   - Or, on a personal fork only, pick **All branches** for simplicity
4. Run **Actions → Deploy OMOTE web flasher → Run workflow** (or push to `Webui`).

### “Branch Webui is not allowed to deploy to github-pages”

GitHub creates a protected **`github-pages`** environment that, by default, only lets the **default branch** (usually `main`) publish. This workflow runs on **`Webui`**, so the deploy step is blocked until you add **`Webui`** under **Settings → Environments → github-pages → Deployment branches** (see step 3 above).

Then **re-run** the failed workflow (Actions → workflow run → **Re-run all jobs**).

**Alternative:** merge the flasher into `main` and change the workflow `push.branches` to `main`, if you prefer not to widen the environment.

## Local test (optional)

Build firmware and copy artifacts into this folder, then serve over HTTPS (Web Serial requires a secure context):

```powershell
cd Platformio
pio run -e esp32_Rev1
pio run -e esp32_Rev1 -t buildfs
$out = ".pio/build/esp32_Rev1"
$dest = "../docs/flasher"
Copy-Item "$out/bootloader.bin","$out/partitions.bin","$out/firmware.bin","$out/littlefs.bin" $dest
$bootApp0 = "$out/boot_app0.bin"
if (-not (Test-Path $bootApp0)) {
  $bootApp0 = Get-ChildItem "$env:USERPROFILE\.platformio\packages" -Recurse -Filter boot_app0.bin -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match 'partitions\\boot_app0\.bin$' } | Select-Object -First 1 -ExpandProperty FullName
}
Copy-Item $bootApp0 "$dest\boot_app0.bin"
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
