/* Pure, host-testable scene-to-LED mapping. No Espressif dependencies. */
#pragma once
#include <stdint.h>
#include "scene.h"

enum { TDONGLE_LED_NORMAL, TDONGLE_LED_TRADING, TDONGLE_LED_SUCCESS };
static inline uint8_t tdongle_trade_phase(uint8_t show, uint32_t elapsed_ms,
                                          uint32_t reveal_ms)
{
    if (show == SHOW_TRADE) return TDONGLE_LED_TRADING;
    if (show == SHOW_TRADED) {
        /* The received sprite has finished its arrival animation. */
        if (elapsed_ms >= reveal_ms && elapsed_ms - reveal_ms >= 1600)
            return TDONGLE_LED_SUCCESS;
        return TDONGLE_LED_TRADING;
    }
    return TDONGLE_LED_NORMAL;
}
