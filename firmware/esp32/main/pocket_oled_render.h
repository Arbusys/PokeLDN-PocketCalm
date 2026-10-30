/* Pocket Calm 128x64 monochrome UI (SSD1306). Host-buildable renderer. */
#pragma once
#include <stdint.h>
#include "scene.h"

/* The scene state is already updated by scene_draw(); this overlays the
 * Pocket Calm layout without modifying packets, radio or ESP32 drivers. */
void pocket_oled_render(uint8_t *frame, const scene_radio_t *radio,
                        const scene_visual_t *visual, uint32_t now_ms);
