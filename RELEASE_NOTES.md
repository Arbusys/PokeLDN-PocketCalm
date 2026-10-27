# PokéLDN Pocket Calm v0.7 — Raid Lobby Edition (unofficial)

Windows one-click installer for the **original LILYGO T-Dongle S3** (ESP32-S3, integrated 160×80 LCD).

## New

- Raid lobby screen using **only existing PokéLDN Windows events**: boss sprite, title when supplied, stars, guest count, and Link Code.
- Distinct restrained headings for **Tera Raid**, **Max Raid**, **GMax Raid** and **Dynamax Adventure** when the sent lobby title identifies them. Generic fallback otherwise.
- Guest counter updates when `JOINED` is sent. Long Pokémon names scroll slowly.
- Keeps the v0.6 exchange interface, original one-bit sprites, flying Poké Ball, and calm LED **unchanged**.

## Compatibility

No modifications to the original PokéLDN application. If an event isn't sent by the application, this firmware can't show it. Gigantamax, Tera type, timer, results and guest-mode raids aren't inferred. The screen may remain on Home until a hosted `LOBBY` packet arrives.

## How to install

1. Download `PokeLDN-PocketCalm-v0.7-Installer.exe` from the GitHub Release or Actions artifact.
2. Close PokéLDN and connect the original T-Dongle S3.
3. Open the installer, select the correct port and confirm.
4. Wait until flashing is complete; reconnect the dongle and open PokéLDN.

Keep the known-good v0.6 release available for rollback until this build is tested on real raids.

Unofficial, unaffiliated fan-made firmware. AGPL-3.0; see the source LICENSE.
