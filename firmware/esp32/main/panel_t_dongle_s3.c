/* LILYGO T-Dongle-S3: 0.96-inch ST7735S IPS panel, 160x80 landscape.
   Retains pokeldn's existing 128x64 animation protocol and renders it as
   colour-accented pixel art with time-based scene effects. No host changes.

   Original 80x160 glass: CS=4, MOSI=3, CLK=5, DC=2, RST=1, backlight=38
   (active low). In landscape orientation the ST7735 RAM offset is (1,26).
   GPIO39/40 drive the separate APA102 LED in led.c; GPIO21 is SDMMC D2,
   not a user LED on this board.
 */
#include <string.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_attr.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "panel.h"
#include "scene.h"
#include "screen.h"
#include "tdongle_render.h"

#define LCD_CS          GPIO_NUM_4
#define LCD_DC          GPIO_NUM_2
#define LCD_SCK         GPIO_NUM_5
#define LCD_MOSI        GPIO_NUM_3
#define LCD_RST         GPIO_NUM_1
#define LCD_BACKLIGHT   GPIO_NUM_38
#define LCD_W           TDONGLE_W
#define LCD_H           TDONGLE_H
#define LCD_X_OFFSET    1
#define LCD_Y_OFFSET    26

static spi_device_handle_t s_spi;
static DMA_ATTR uint8_t s_pixels[TDONGLE_PIXEL_BYTES];
static uint8_t s_previous[SCREEN_BYTES];
static bool s_drawn;
static uint32_t s_last_present_ms;

static bool send(const void *bytes, size_t length)
{
    spi_transaction_t t = {.length = length * 8, .tx_buffer = bytes};
    return spi_device_transmit(s_spi, &t) == ESP_OK;
}

static bool command(uint8_t op, const uint8_t *args, size_t length)
{
    gpio_set_level(LCD_DC, 0);
    if (!send(&op, 1)) return false;
    gpio_set_level(LCD_DC, 1);
    return !length || send(args, length);
}

static bool window(unsigned x, unsigned y, unsigned width, unsigned height)
{
    x += LCD_X_OFFSET;
    y += LCD_Y_OFFSET;
    const uint8_t cols[] = {x >> 8, x & 255, (x + width - 1) >> 8, (x + width - 1) & 255};
    const uint8_t rows[] = {y >> 8, y & 255, (y + height - 1) >> 8, (y + height - 1) & 255};
    return command(0x2a, cols, sizeof(cols)) && command(0x2b, rows, sizeof(rows)) &&
           command(0x2c, NULL, 0);
}

static bool backlight_setup(void)
{
    const ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .timer_num = LEDC_TIMER_1,
        .duty_resolution = LEDC_TIMER_10_BIT, .freq_hz = 5000, .clk_cfg = LEDC_AUTO_CLK,
    };
    const ledc_channel_config_t channel = {
        .gpio_num = LCD_BACKLIGHT, .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1, .timer_sel = LEDC_TIMER_1, .duty = 0,
        .hpoint = 0, .flags.output_invert = 1,  /* low turns this backlight on */
    };
    return ledc_timer_config(&timer) == ESP_OK && ledc_channel_config(&channel) == ESP_OK;
}

bool panel_init(void)
{
    const gpio_config_t io = {
        .pin_bit_mask = (1ULL << LCD_CS) | (1ULL << LCD_DC) | (1ULL << LCD_RST),
        .mode = GPIO_MODE_OUTPUT,
    };
    if (gpio_config(&io) != ESP_OK || !backlight_setup()) return false;
    gpio_set_level(LCD_CS, 1);
    gpio_set_level(LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(150));

    const spi_bus_config_t bus = {
        .mosi_io_num = LCD_MOSI, .miso_io_num = -1, .sclk_io_num = LCD_SCK,
        .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = sizeof(s_pixels),
    };
    const spi_device_interface_config_t dev = {
        .clock_speed_hz = 26000000, .mode = 0, .spics_io_num = -1, .queue_size = 1,
    };
    if (spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK ||
        spi_bus_add_device(SPI2_HOST, &dev, &s_spi) != ESP_OK) return false;

    gpio_set_level(LCD_CS, 0);
    bool ok = command(0x01, NULL, 0);  /* SWRESET */
    vTaskDelay(pdMS_TO_TICKS(150));
    ok = ok && command(0x11, NULL, 0); /* SLPOUT */
    vTaskDelay(pdMS_TO_TICKS(120));
    const uint8_t fmt = 0x55;         /* 16-bit RGB565 */
    const uint8_t madctl = 0x68;      /* landscape, MX+MV, BGR */
    ok = ok && command(0x3a, &fmt, 1) && command(0x36, &madctl, 1) &&
         command(0x21, NULL, 0) &&  /* inversion required by the IPS glass */
         command(0x13, NULL, 0) &&  /* normal display mode */
         command(0x29, NULL, 0);     /* display ON */
    memset(s_pixels, 0, sizeof(s_pixels));
    ok = ok && window(0, 0, LCD_W, LCD_H) && send(s_pixels, sizeof(s_pixels));
    gpio_set_level(LCD_CS, 1);
    if (ok) panel_power(SCENE_ON);
    return ok;
}

void panel_present(const uint8_t *frame)
{
    const uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    scene_visual_t visual = {0};
    scene_visual(&visual, now);
    /* Retain fast source animation, but refresh even static frames every
       200ms so the coloured status animation keeps moving. */
    if (s_drawn && memcmp(s_previous, frame, SCREEN_BYTES) == 0 &&
        now - s_last_present_ms < (visual.show == SHOW_TRADED ? 50u : 200u))
        return;
    tdongle_render(frame, s_pixels, &visual, now);
    gpio_set_level(LCD_CS, 0);
    const bool ok = window(0, 0, LCD_W, LCD_H) && send(s_pixels, sizeof(s_pixels));
    gpio_set_level(LCD_CS, 1);
    if (ok) {
        memcpy(s_previous, frame, SCREEN_BYTES);
        s_last_present_ms = now;
    }
    s_drawn = ok;
}

bool panel_power(uint8_t power)
{
    const uint32_t duty = power == SCENE_ON ? 850 : power == SCENE_DIM ? 40 : 0;
    return ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty) == ESP_OK &&
           ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1) == ESP_OK;
}
