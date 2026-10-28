# Building Pocket Calm from source

## Overview

The source and the bundled Windows installer are built in GitHub Actions. The firmware is based on the upstream PokéLDN ESP32-S3 firmware and targets the original LILYGO T-Dongle S3 display. The public `.exe` bundles a merged firmware image and Espressif's flashing tool. Source code is included in this repository under the applicable licences.

## GitHub Actions — maintainers

1. Select the intended branch in **Actions → Build portable Windows firmware installer → Run workflow**.
2. The workflow tests the desktop renderer, builds the ESP-IDF 6.1 firmware, merges flash images, bundles the upstream esptool for Windows and compiles the Go Windows GUI.
3. Check that the job is **green** and download the generated `.exe` from **Artifacts**. Inspect and test the executable before publishing it to Releases.

The **release-safe workflow** supplied in the publication kit intentionally does **not** auto-publish when you create a `v*` tag. Publishing the release is a separate, manual step so its assets and notes can be reviewed first.

## Local firmware build (developers)

Install ESP-IDF 6.1 and open its configured shell:

```powershell
.\build_tdongle_windows.ps1
# Optional: after checking the correct connected board and COM port
.\build_tdongle_windows.ps1 -Port COM5 -Flash
```

The COM port may be different. Do not flash unknown ESP32 devices. End users **do not** need ESP-IDF.

## Full source and reproducibility notes

Keep the original upstream copyright notices and component-specific licence files intact. Check the exact upstream versions and checksums of externally downloaded build dependencies before each public release. Generated GitHub Actions artifacts are *not* a substitute for preserving the corresponding source under the release tag.

For attribution, see [CREDITS.md](CREDITS.md).
