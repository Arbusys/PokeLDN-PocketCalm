# Credits, licensing and trademarks

**Pocket Calm** is an unofficial, community-maintained display and installer adaptation for the original LILYGO T-Dongle S3.

## Upstream projects

- **PokéLDN** — upstream ESP32 radio firmware and desktop protocol: https://github.com/Decryptu/pokeldn . Pocket Calm is built on that project's sources and is not maintained or endorsed by its authors.
- **Espressif** — ESP-IDF, ESP32-S3 tools and `esptool`: https://github.com/espressif/esp-idf and https://github.com/espressif/esptool . The installer fetches the Windows flashing tool from an upstream Espressif release during its build.
- **LILYGO** — manufacturer of the T-Dongle S3 hardware: https://lilygo.cc/ .
- **zstd** — third-party code maintained with its separate notice in `vendor/zstd/LICENSE` and `vendor/zstd/UPSTREAM.md`.

## Licence

The project contains modified source derived from **PokéLDN**, covered by the **GNU Affero General Public License v3.0** (see the root [LICENSE](../LICENSE)). Retain upstream notices and source availability when redistributing source or compiled firmware. Different bundled components may have their own applicable licensing terms; consult their upstream repositories and included notices.

The firmware works with external monochrome sprite data received through the PokéLDN protocol; the repository is not intended to supply a proprietary Pokémon sprite library.

## Trademarks and endorsement

**Pokémon**, game titles, Nintendo and The Pokémon Company names and marks belong to their respective owners. Product names such as **LILYGO**, **Espressif** and **PokéLDN** identify compatibility or attribution only. Pocket Calm is **not affiliated with or endorsed by** those organisations.

Do not represent this project as a Nintendo, Pokémon, LILYGO or official PokéLDN product. A compatible community firmware is not an official hardware accessory.
