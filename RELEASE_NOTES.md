# Pocket Calm v1.0.0 — First stable release

**The T-Dongle S3 display companion for PokéLDN.** Pocket Calm brings Pokémon trading and raid-lobby visuals to the original LILYGO T-Dongle S3 (160 × 80 LCD), using the existing PokéLDN Windows application without modifications.

### What's included

- **A redesigned Windows installer** with a navy-and-cream UI, custom icon, progress display, and **automatic COM-port selection** with manual fallback.
- **Five switchable themes:** Classic, Modern Minimal, Game Boy Color, Crimson Ruby and Ocean Teal. Use the rear button; your selection survives power cycles.
- **Trade visuals:** readable monochrome Pokémon silhouettes on both sides, with a flying Poké Ball animation and arrival confirmation.
- **Raid lobby screens:** show available boss details, stars, guest counts and link codes when the existing desktop app sends the event. Tera / Max / GMax / Dynamax Adventure titles appear only when supported by the incoming title.
- **Softer LED feedback and reduced idle activity**, retaining PokéLDN's existing USB/radio behaviour.

### Download

Under **Assets** below, choose **`PokeLDN-PocketCalm-v1.0-Installer.exe`**. This is the **Windows 10/11 (64-bit)** installer for the **original LILYGO T-Dongle S3 with integrated 160 × 80 display**. The automatically generated `Source code (zip)` and `Source code (tar.gz)` are for developers, **not** installers.

### Quick install

1. Plug your T-Dongle S3 into your PC.
2. Close the PokéLDN desktop application.
3. Start the installer, verify the detected COM port and select **INSTALL FIRMWARE**.
4. Wait for success, reconnect the dongle, then open PokéLDN.
5. Briefly press the rear button to cycle display themes.

**Warning:** flashing replaces the connected board's firmware. Chip detection alone cannot verify the precise LILYGO model. Do not use this installer on other ESP32-S3 boards.

### Compatibility & limitations

- **No desktop application changes.** Raid data is shown only when PokéLDN supplies it; battle timers, Tera types and unreported states cannot be displayed.
- This release has been tested on the maintainer's board; community reports from other Windows PCs and T-Dongle S3 units are appreciated.
- No bundled Pokémon game content or proprietary sprite collection.

### Credits and source

Built on the open-source [PokéLDN firmware](https://github.com/Decryptu/pokeldn). Pocket Calm is an **unofficial community project**, not affiliated with Nintendo, The Pokémon Company, LILYGO, Espressif, or the PokéLDN authors. Licensed as described in [LICENSE](https://github.com/Arbusys/PokeLDN-PocketCalm/blob/v1.0.0/LICENSE) and [credits](https://github.com/Arbusys/PokeLDN-PocketCalm/blob/v1.0.0/docs/CREDITS.md). The release's full corresponding source is available at its Git tag.

For complete instructions, visit [README](https://github.com/Arbusys/PokeLDN-PocketCalm#readme) and [Installation](https://github.com/Arbusys/PokeLDN-PocketCalm/blob/v1.0.0/docs/INSTALLATION.md).
