/* Host-only display regression harness. No ESP32 libraries required.
   Run using tools/render_raid_previews.py. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scene.h"
#include "screen.h"
#include "tdongle_render.h"

static uint8_t mono[SCREEN_BYTES];
static uint8_t pixels[TDONGLE_PIXEL_BYTES];
static int send_lobby(const char *title, const char *boss, const char *code,
                      uint8_t stars, uint8_t joined, uint8_t seats, uint32_t when)
{
    uint8_t data[110];
    size_t i = 0;
    data[i++] = DISPLAY_OP_LOBBY; data[i++] = stars;
    data[i++] = joined; data[i++] = seats;
    const char *a[3] = { title, boss, code };
    for (int f = 0; f < 3; ++f) {
        const size_t n = strlen(a[f]);
        if (n > 21 || i + n + 1 > sizeof(data)) return 0;
        memcpy(data + i, a[f], n); i += n;
        data[i++] = 0;
    }
    return scene_command(data, i, when);
}
static void draw(uint32_t now, const char *filename)
{
    scene_radio_t r = {.mode = SCENE_HOSTING, .stations = 1};
    scene_visual_t v;
    scene_draw(mono, &r, now);
    scene_visual(&v, now);
    tdongle_render(mono, pixels, &v, now);
    FILE *fp = fopen(filename, "wb");
    if (!fp) exit(3);
    if (fwrite(pixels, 1, sizeof(pixels), fp) != sizeof(pixels)) exit(4);
    fclose(fp);
}
static void example_sprite(void)
{
    /* 40x38 monochrome sprite-shaped diagnostic; the real app sends sprite data. */
    const int w = 40, h = 38, stride = 5;
    uint8_t data[4 + 5 * 38] = {DISPLAY_OP_SPRITE, SLOT_OURS, 40, 38};
    for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) {
        int dx = x - 20, dy = y - 20;
        int head = (dx * dx) / 230 + (dy * dy) / 300 <= 1;
        int ear = y < 17 && ((x > 6 && x < 14) || (x > 26 && x < 34));
        int eye = y >= 16 && y <= 19 && ((x >= 13 && x <= 15) || (x >= 24 && x <= 26));
        if ((head || ear) && !eye) data[4 + y * stride + x/8] |= (uint8_t)(0x80 >> (x%8));
    }
    if (!scene_command(data, sizeof(data), 0)) exit(5);
}
int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    char filename[256];
    const char *titles[] = {"Tera Raid", "Max Raid", "Gigantamax Raid", "Dynamax Adventure", "Raid"};
    const char *bosses[] = {"Charizard", "Duraludon", "Gengar", "Suicune", "Mystery boss"};
    const uint8_t stars[] = {7, 5, 5, 0, 3};
    const char *codes[] = {"AB12 CD34", "1234 5678", "4242 4242", "", ""};
    for (int i = 0; i < 5; ++i) {
        scene_reset(); example_sprite();
        if (!send_lobby(titles[i], bosses[i], codes[i], stars[i], (uint8_t)(i % 3), 3, 100)) exit(6);
        snprintf(filename, sizeof(filename), "%s/raid_%d.rgb565", argv[1], i);
        draw(500, filename);
    }
    scene_reset(); example_sprite();
    if (!send_lobby("Tera Raid", "Longer than nineteen", "AB12 CD34", 7, 0, 3, 100)) exit(7);
    const uint8_t joined_packet[] = {DISPLAY_OP_JOINED, 2};
    if (!scene_command(joined_packet, sizeof(joined_packet), 650)) exit(8);
    snprintf(filename, sizeof(filename), "%s/joined.rgb565", argv[1]);
    draw(700, filename);
    scene_reset();
    const uint8_t show_trade[] = {DISPLAY_OP_SHOW, SHOW_TRADE, 0, 0, 'S','w','/','S','h',0,'E','E','V','E','E',0};
    if (!scene_command(show_trade, sizeof(show_trade), 800)) exit(9);
    snprintf(filename, sizeof(filename), "%s/trade.rgb565", argv[1]);
    draw(900, filename);
    return 0;
}
