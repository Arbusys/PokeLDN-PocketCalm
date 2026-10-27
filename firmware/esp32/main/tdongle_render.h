/* Hardware-independent palette conversion for the T-Dongle-S3's ST7735. */
#pragma once

#include <stdint.h>
#include "scene.h"

#define TDONGLE_W 160
#define TDONGLE_H 80
#define TDONGLE_PIXEL_BYTES (TDONGLE_W * TDONGLE_H * 2)

/* RGB565 big-endian from the unchanged 128x64 PokéLDN source frame.
   Read-only visual hints colour the panel without new desktop packets. */
void tdongle_render(const uint8_t *frame, uint8_t *out,
                    const scene_visual_t *visual, uint32_t now_ms);
