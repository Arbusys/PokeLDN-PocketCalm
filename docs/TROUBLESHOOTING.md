# Troubleshooting

Pocket Calm is a community firmware for **one specific device**, not a general ESP32 flashing tool.

## No dongle / no COM port detected

- Verify the T-Dongle S3 is plugged into a working USB data connection; try another USB port.
- Close PokéLDN and other applications that may hold the port open.
- Open *Windows Device Manager → Ports (COM & LPT)* and check which COM entry appears or disappears as you reconnect the dongle.
- If Windows lists the device without a working serial interface, consult the [upstream PokéLDN Windows USB-driver guidance](https://github.com/Decryptu/pokeldn).
- If no automatic match is found, select the correct COM port **manually**. Do not choose a random port.

## The installer detects a different ESP32-S3

**Stop.** Identifying an ESP32-S3 chip is not sufficient to verify the LILYGO T-Dongle S3 screen, GPIO layout or model. Use Pocket Calm only on the original supported board.

## 'Port busy' / 'Access denied'

Close PokéLDN, serial monitors and other programs. Disconnect/reconnect the board, then retry. Do not run two flashing programs simultaneously.

## Flash stalls or fails

- Keep USB connected while flashing; try a direct USB port rather than an unreliable hub.
- Read the installer log and copy the **first meaningful error**, not just the final 'flash failed' message.
- If the board unexpectedly enters download mode, disconnect it and reconnect normally without holding BOOT.
- Avoid repeated flashing until the error is understood; open a GitHub issue with a sanitized log.

## Firmware installed, but display does not look right

Confirm you are using the **original LILYGO T-Dongle S3 with its 160 × 80 screen**. Other displays use different drivers and GPIO pin layouts. A colour difference between a preview image and the LCD can be normal.

## Rear theme button appears not to work

- Use a **short press after boot**, not while connecting USB.
- Verify that you flashed the v1.0.0 installer, not an older development build.
- Check that the screen is powered and that PokéLDN still recognises the dongle.

## Raid screen does not appear

The firmware can show raid-lobby information **only when the stock PokéLDN application actually sends its `LOBBY`/`JOINED` display event**. Not every game phase sends a display update. Battle timers, win/loss and Tera types are not transmitted in the supported display protocol.

## Windows SmartScreen warns about the installer

A community-built Windows `.exe` may be unsigned and display a reputation warning. **Do not bypass a security warning solely because this README says so.** Check that you downloaded the file from the project's GitHub Releases, review the matching source, and confirm its published checksum if one is provided. When in doubt, do not run it.

## Reporting a bug

Provide: Windows version, exact hardware model, installer version, what you expected, what happened and relevant log lines (with secrets removed). Do not publish Switch `prod.keys`, credentials or private data.
