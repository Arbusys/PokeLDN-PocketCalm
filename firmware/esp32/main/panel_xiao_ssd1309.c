/* Pocket Calm experimental display profile — Andrew's XIAO ESP32S3 Plus.
 * 2.42-inch 128x64 SSD1309 I2C module, PCB marked "2.42OLED-IIC VER:1.1".
 * Connections verified from photographs/labels, not electrical tests:
 * 3V3->VDD, GND->GND, D4/GPIO5->SDA, D5/GPIO6->SCL.
 * This module/controller identity is seller-reported; NOT hardware verified.
 *
 * SSD1309 differs from SSD1306 in page positioning. The SSD1306 code uses
 * 0x21/0x22 horizontal window commands; this driver uses SSD1309 page mode,
 * with a command prefix and 128 data bytes for each changed 8-pixel page.
 * It retains PokéLDN's 128x64, 1-bit framebuffer and event protocol.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/i2c_master.h"
#include "panel.h"
#include "scene.h"
#include "screen.h"

#define OLED_SDA GPIO_NUM_5
#define OLED_SCL GPIO_NUM_6
#define OLED_HZ 400000
/* Change to 2 if the first 2 columns appear shifted on some SSD1309 panels. */
#define OLED_COL_OFFSET 0

static const uint8_t INIT[] = {
    0x00,           /* I2C control byte: commands */
    0xAE,           /* display off */
    0xD5, 0xA0,     /* clock divide / oscillator */
    0xA8, 0x3F,     /* 64 multiplex rows */
    0xD3, 0x00,     /* display offset */
    0x40,           /* start line */
    0x20, 0x02,     /* PAGE addressing (not SSD1306 horizontal mode) */
    0xA1, 0xC8,     /* segment remap / COM scan reversed */
    0xDA, 0x12,     /* COM pin configuration */
    0x81, 0x6F,     /* moderate brightness */
    0xD9, 0xD3,     /* SSD1309 precharge */
    0xDB, 0x20,     /* VCOMH */
    0x2E,           /* scrolling off */
    0xA4, 0xA6,     /* follow RAM contents, normal polarity */
    0xAF,           /* on */
};
static const uint8_t POWER_ON[]  = {0x00, 0x81, 0x6F, 0xAF};
static const uint8_t POWER_DIM[] = {0x00, 0x81, 0x08, 0xAF};
static const uint8_t POWER_OFF[]= {0x00, 0xAE};

static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_device;
static uint8_t s_sent[SCREEN_BYTES];
static bool s_has_sent;
static uint8_t s_page[1 + SCREEN_W] = {0x40};

static bool transmit(const uint8_t *bytes, size_t n, int wait_ms)
{
    return s_device && i2c_master_transmit(s_device, bytes, n, wait_ms) == ESP_OK;
}

bool panel_init(void)
{
    const i2c_master_bus_config_t cfg = {
        .i2c_port = -1,
        .sda_io_num = OLED_SDA, .scl_io_num = OLED_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    if (i2c_new_master_bus(&cfg, &s_bus) != ESP_OK) return false;

    uint8_t address = 0x3C;
    if (i2c_master_probe(s_bus, address, 100) != ESP_OK) {
        address = 0x3D;
        if (i2c_master_probe(s_bus, address, 100) != ESP_OK) {
            i2c_del_master_bus(s_bus);
            s_bus = NULL;
            return false;
        }
    }
    const i2c_device_config_t device_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = OLED_HZ,
    };
    if (i2c_master_bus_add_device(s_bus, &device_cfg, &s_device) != ESP_OK) {
        i2c_del_master_bus(s_bus);
        s_bus = NULL;
        return false;
    }
    if (!transmit(INIT, sizeof(INIT), 200)) {
        i2c_master_bus_rm_device(s_device);
        i2c_del_master_bus(s_bus);
        s_device = NULL;
        s_bus = NULL;
        return false;
    }
    s_has_sent = false;
    scene_panel(PANEL_128X64);
    return true;
}

void panel_present(const uint8_t *frame)
{
    if (!s_device || !frame) return;
    /* SSD1309 controller uses page commands, then the data for that page.
       Skip unchanged pages to cut I2C traffic during idle scenes. */
    for (uint8_t page = 0; page < SCREEN_H / 8; ++page) {
        const uint8_t *data = frame + (size_t)page * SCREEN_W;
        if (s_has_sent && memcmp(data, s_sent + (size_t)page * SCREEN_W, SCREEN_W) == 0)
            continue;
        const uint8_t col = OLED_COL_OFFSET;
        const uint8_t cmd[] = {0x00,
            (uint8_t)(0xB0 | page),
            (uint8_t)(0x00 | (col & 0x0F)),
            (uint8_t)(0x10 | ((col >> 4) & 0x0F))};
        if (!transmit(cmd, sizeof(cmd), 100)) {
            s_has_sent = false;
            return;
        }
        memcpy(s_page + 1, data, SCREEN_W);
        if (!transmit(s_page, sizeof(s_page), 100)) {
            s_has_sent = false;
            return;
        }
        memcpy(s_sent + (size_t)page * SCREEN_W, data, SCREEN_W);
    }
    s_has_sent = true;
}

bool panel_power(uint8_t power)
{
    if (!s_device) return false;
    switch (power) {
        case SCENE_ON:  return transmit(POWER_ON, sizeof(POWER_ON), 50);
        case SCENE_DIM: return transmit(POWER_DIM, sizeof(POWER_DIM), 50);
        case SCENE_OFF: return transmit(POWER_OFF, sizeof(POWER_OFF), 50);
        default: return false;
    }
}
