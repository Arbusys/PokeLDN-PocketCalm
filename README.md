# Pocket Calm — Andrew XIAO SSD1309 Test Edition

> **Experimental firmware for one user's soldered hardware. NOT the official Pocket Calm v1.0 release.**

This isolated branch targets a **Seeed Studio XIAO ESP32S3 Plus** with an external 2.42-inch **128 × 64 I²C SSD1309 OLED** (`2.42OLED-IIC VER:1.1`). This controller designation is from the product listing; it has **not been verified on a physical unit**.

**Wiring:** OLED `VDD → 3V3`, `GND → GND`, `SDA → GPIO5 (D4)`, `SCL → GPIO6 (D5)`. The driver probes 7-bit addresses `0x3C` and `0x3D`. It sends SSD1309 page addressing commands and avoids updating unchanged 8-pixel pages.

The new native 128 × 64 monochrome UI is rendered from the firmware's actual C graphics implementation: **home / radio status, trade animation with two silhouettes and a moving Poké Ball, raid lobby with transmitted boss, rank, guests and code, received Pokémon and Mystery Gift**. No PC PokéLDN changes or new wire packets are required. Demo captures use invented sample sprites and data.

### Build with GitHub Actions

1. Create a branch named **`andrew-ssd1309`** from `main` (not an orphan branch).
2. Upload the extracted contents of this snapshot in batches of under 100 files. **Do not upload the ZIP itself.** Existing files in the branch should be replaced as appropriate.
3. Create `.github/workflows/build-andrew-ssd1309.yml` in GitHub's browser editor and paste the contents of `WORKFLOW_A_COLLER.yml`. Windows may hide `.github` when dragging folders.
4. Commit that workflow **last**. It triggers an automatic build on `andrew-ssd1309`.
5. Open **Actions**, wait for a green **Build Andrew SSD1309 installer**, download the `.exe` in **Artifacts**.

The branch-specific workflow never publishes releases; it does not change the stable v1.0 release. The existing default workflow must not be used for this hardware.

### Flashing warning

An ESP32-S3 signature **does not identify the specific XIAO board**, and a COM port may belong to another device. Confirm the physical card and I²C wiring before flashing. The firmware will replace the existing image and can erase its current application. Back up first. Keep the OLED supplied by **3.3 V**; never put 5 V on unverified GPIO. If the screen stays black, verify wiring and I²C response before repeatedly flashing.

### Testing

The pixel renderer can be compiled and previewed on a desktop. Full ESP-IDF build and physical-screen testing are pending. A successful Actions build is not proof of hardware compatibility.

Pocket Calm is an unofficial project based on PokéLDN; see [LICENSE](LICENSE). Not affiliated with Nintendo, The Pokémon Company or the PokéLDN team.
