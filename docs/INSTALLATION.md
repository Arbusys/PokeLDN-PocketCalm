# Install Pocket Calm on Windows

This guide is for the **original LILYGO T-Dongle S3 with integrated 160 × 80 display**, used alongside the regular [PokéLDN Windows application](https://pokeldn.app/).

## Before you start

- A Windows 10 or Windows 11 computer (64-bit).
- The original supported LILYGO board, connected by USB. An adapter or hub must support USB data, not just power.
- The `PokeLDN-PocketCalm-v1.0-Installer.exe` from [GitHub Releases](https://github.com/Arbusys/PokeLDN-PocketCalm/releases/latest).
- Close PokéLDN, serial monitors, and tools that could be using the port.

**Caution:** This writes firmware to your board and replaces its previous installed firmware. It is not intended for visually similar or generic ESP32-S3 products; the same chip does not mean the same screen or pin wiring.

## Flashing

1. Plug the dongle into your Windows PC.
2. Start the Pocket Calm installer (no ESP-IDF or Python required).
3. Allow its **automatic USB/COM detection** to finish. If it offers more than one candidate, verify the dongle in *Device Manager → Ports (COM & LPT)* and choose the correct port manually.
4. Verify the physical hardware is the supported T-Dongle S3 and that the installer selected the expected COM port.
5. Choose **INSTALL FIRMWARE** and confirm the warning.
6. Keep the board connected until you see **Firmware installed successfully!**. Do not unplug during flashing.
7. Unplug and reconnect the dongle. Launch PokéLDN and confirm it recognises the connected board.

## Choosing a theme

After the dongle boots, briefly press the **rear BOOT button** to cycle Classic → Modern Minimal → Game Boy Color → Crimson Ruby → Ocean Teal. The last theme is saved automatically. Avoid holding BOOT while plugging in the device; that can start its bootloader mode.

## Updating or reinstalling

Download a newer official Pocket Calm Release and repeat the same procedure. Review its release notes first; a firmware update can replace previous settings or third-party firmware. Keep a copy of a known-good installer in case you need to roll back.

## Finding help

See [Troubleshooting](TROUBLESHOOTING.md), or open a [GitHub issue](https://github.com/Arbusys/PokeLDN-PocketCalm/issues) with your OS version, T-Dongle model, installer version and *non-sensitive* error log. Never publish private keys, account credentials, personal files or other secrets.
