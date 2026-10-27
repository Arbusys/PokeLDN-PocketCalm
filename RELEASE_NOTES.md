# PokéLDN Pocket Calm v0.6 (unofficial)

Windows one-click firmware installer for **the original LILYGO T-Dongle S3**, ESP32-S3 with its built-in 160×80 LCD.

1. Download `PokeLDN-PocketCalm-v0.6-Installer.exe` from this release.
2. Connect the dongle by USB, **close PokéLDN**.
3. Open the EXE, confirm the COM port displayed in Windows Device Manager.
4. Select **Install v0.6** and wait for success. Do not disconnect during flashing.
5. Reconnect the dongle and open PokéLDN.

Do **not** use it with a different ESP32 board. The installer checks the ESP32-S3 chip family but cannot conclusively identify the exact LILYGO board variant. Flashing replaces the installed firmware. The application includes Espressif esptool and the complete firmware: no ESP-IDF/Python is required for end users.

**Unofficial**: Not associated with Nintendo, The Pokémon Company, Espressif, LILYGO or PokéLDN developers.

Source is licensed AGPL-3.0; vendored third-party licences are included in source. No warranty. Consult the source repository for build details.
