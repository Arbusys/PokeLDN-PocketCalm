/* Pocket Calm Andrew prototype: 128x64 SSD1309 monochrome layout.
 * No GPIO, I2C, serial, radio, WiFi or Espressif dependencies.
 * Consumes existing PokéLDN DISPLAY state only (no invented raid data).
 * Layout and sprite geometry are designed for 128x64 pixels.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pocket_oled_render.h"
#include "screen.h"

#define W SCREEN_W
#define H SCREEN_H

static void text_fit(uint8_t *fb, int x, int y, const char *s, unsigned max_chars)
{
    char buf[22] = {0};
    if (!s) return;
    unsigned i = 0;
    while (s[i] && i < max_chars && i < sizeof(buf) - 1) {
        char c = s[i];
        /* 5x7 font is ASCII; avoid control bytes from host labels. */
        buf[i] = (c >= 32 && c <= 126) ? c : '?';
        i++;
    }
    fb_text(fb, x, y, buf, 1);
}

static void label_right(uint8_t *fb, int y, const char *s, unsigned max_chars)
{
    char buf[22] = {0};
    if (!s) return;
    unsigned i = 0;
    while (s[i] && i < max_chars && i < sizeof(buf) - 1) {
        char c = s[i];
        buf[i] = (c >= 32 && c <= 126) ? c : '?';
        i++;
    }
    fb_text(fb, W - 2 - fb_text_width(buf, 1), y, buf, 1);
}

/* Tiny 7px Poké Ball: legible even on 0.96-inch OLED. */
static void ball(uint8_t *fb, int x, int y, int r)
{
    fb_circle(fb, x, y, r, false, true);
    fb_line(fb, x - r, y, x + r, y);
    for (int dy = -r + 1; dy < 0; ++dy)
        for (int dx = -r + 1; dx <= r - 1; ++dx)
            if (dx*dx + dy*dy <= (r-1)*(r-1)) fb_pixel(fb, x+dx, y+dy, true);
    fb_circle(fb, x, y, r > 5 ? 2 : 1, true, false);
    fb_circle(fb, x, y, r > 5 ? 2 : 1, false, true);
}

static void frame(uint8_t *fb, const char *title, const char *status)
{
    fb_clear(fb);
    fb_fill(fb, 0, 0, W, 12, true);
    /* Inverted text: create the letters as gaps in the filled top bar. */
    uint8_t mask[SCREEN_BYTES];
    fb_clear(mask);
    fb_text(mask, 3, 2, title, 1);
    if (status) {
        char cut[13] = {0};
        snprintf(cut, sizeof(cut), "%.11s", status);
        fb_text(mask, W - 3 - fb_text_width(cut, 1), 2, cut, 1);
    }
    for (int y = 0; y < 12; ++y)
        for (int x = 0; x < W; ++x)
            if (fb_get(mask, x, y)) fb_pixel(fb, x, y, false);
    fb_fill(fb, 0, 55, W, 1, true);
}

static void arrow(uint8_t *fb, int cx, int y, uint32_t t)
{
    fb_line(fb, cx - 6, y, cx + 5, y);
    fb_line(fb, cx + 5, y, cx + 2, y - 3);
    fb_line(fb, cx + 5, y, cx + 2, y + 3);
    if ((t / 380) % 2 == 0) fb_pixel(fb, cx - 8, y, true);
}

/* Source sprites are max 64x64 and 1-bit (MSB-first rows); we crop blank
 * margins and scale with nearest-neighbour so the two 41x35 slots stay crisp. */
static bool sprite_in(uint8_t *fb, unsigned slot, int x, int y, int box_w, int box_h)
{
    uint8_t w = 0, h = 0;
    const uint8_t *bits = scene_sprite_bits((uint8_t)slot, &w, &h);
    if (!bits || !w || !h || w > SPRITE_W || h > SPRITE_H) return false;
    int x0 = w, y0 = h, x1 = -1, y1 = -1;
    const int stride = (w + 7) / 8;
    for (int sy = 0; sy < h; ++sy)
        for (int sx = 0; sx < w; ++sx)
            if (bits[sy * stride + sx / 8] & (0x80u >> (sx % 8))) {
                if (sx < x0) x0 = sx;
                if (sy < y0) y0 = sy;
                if (sx > x1) x1 = sx;
                if (sy > y1) y1 = sy;
            }
    if (x1 < x0) return false;
    const int sw = x1 - x0 + 1, sh = y1 - y0 + 1;
    int factor = (box_w * 1024 / sw) < (box_h * 1024 / sh)
               ? (box_w * 1024 / sw) : (box_h * 1024 / sh);
    /* Downscaling is allowed. No memory or image allocated. */
    if (factor < 1) factor = 1;
    int dw = sw * factor / 1024;
    int dh = sh * factor / 1024;
    if (dw < 1) dw = 1;
    if (dh < 1) dh = 1;
    if (dw > box_w) dw = box_w;
    if (dh > box_h) dh = box_h;
    const int dx = x + (box_w - dw) / 2;
    const int dy = y + (box_h - dh) / 2;
    for (int py = 0; py < dh; ++py)
        for (int px = 0; px < dw; ++px) {
            int sx = x0 + (px * sw / dw);
            int sy = y0 + (py * sh / dh);
            if (bits[sy * stride + sx / 8] & (0x80u >> (sx % 8)))
                fb_pixel(fb, dx + px, dy + py, true);
        }
    return true;
}

static void home(uint8_t *fb, const scene_radio_t *r, uint32_t now)
{
    const char *status = "READY";
    if (r->mode == SCENE_JOINED) status = "LINKED";
    else if (r->mode == SCENE_JOINING) status = "SCAN";
    else if (r->mode == SCENE_HOSTING) status = "HOST";
    else if (r->mode == SCENE_SNIFFING) status = "SNIFF";
    frame(fb, "POCKET CALM", status);
    fb_rect(fb, 3, 16, 49, 35);
    ball(fb, 27, 33 + (now / 650 % 2), 12);
    fb_text(fb, 61, 18, "POKELDN", 1);
    fb_text(fb, 61, 29, r->mode == SCENE_JOINED ? "CONNECTED" :
                            r->mode == SCENE_JOINING ? "SEARCHING" :
                            r->mode == SCENE_HOSTING ? "HOSTING" : "STANDBY", 1);
    char stats[22];
    (void)snprintf(stats, sizeof(stats), "RX:%lu", (unsigned long)r->rx);
    text_fit(fb, 61, 41, stats, 11);
    fb_text(fb, 3, 57, "POKELDN OLED TEST", 1);
}

static void trading(uint8_t *fb, const scene_visual_t *v)
{
    const bool sending = v->show == SHOW_TRADED && v->elapsed_ms < v->reveal_ms;
    frame(fb, "POCKET CALM", v->show == SHOW_TRADE ? "OFFER" : "TRADE");
    fb_rect(fb, 2, 14, 47, 36);
    fb_rect(fb, 79, 14, 47, 36);
    if (!sprite_in(fb, SLOT_OURS, 5, 16, 41, 32)) ball(fb, 25, 31, 9);
    if (!sprite_in(fb, SLOT_THEIRS, 82, 16, 41, 32)) ball(fb, 102, 31, 9);
    arrow(fb, 64, 33, v->elapsed_ms);
    /* Move a 3px ball inside central lane, not across sprites. */
    const int shift = (int)(v->elapsed_ms / 180 % 17) - 8;
    if (v->show == SHOW_TRADED) ball(fb, 64 + shift, 22, 3);
    label_right(fb, 57, sending ? "SENDING..." : "EXCHANGING", 11);
    text_fit(fb, 2, 57, scene_sprite_name(SLOT_OURS), 7);
}

static const char *raid_kind(const char *title)
{
    /* A visual label inferred only from the existing title, not game state. */
    if (!title) return "RAID";
    char lc[22] = {0};
    for (int i = 0; i < 21 && title[i]; ++i) {
        char c = title[i];
        lc[i] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
    }
    if (strstr(lc, "tera") || strstr(lc, "cristal") || strstr(lc,"crystal")) return "TERA";
    if (strstr(lc, "giganta") || strstr(lc, "gmax") || strstr(lc, "g-max")) return "GMAX";
    if (strstr(lc, "dynamax") || strstr(lc, "max raid")) return "MAX";
    return "RAID";
}

static void raid(uint8_t *fb)
{
    frame(fb, "POCKET CALM", raid_kind(scene_title()));
    fb_rect(fb, 2, 14, 47, 39);
    if (!sprite_in(fb, SLOT_OURS, 5, 16, 41, 35)) ball(fb, 25, 32, 10);
    uint8_t stars = scene_lobby_stars();
    if (stars) {
        fb_text(fb, 56, 16, "RANK", 1);
        if (stars > 7) stars = 7;
        for (int i = 0; i < stars; ++i) {
            int x = 60 + i * 9;
            fb_pixel(fb, x, 28, true);
            fb_pixel(fb, x - 2, 28, true);
            fb_pixel(fb, x + 2, 28, true);
            fb_pixel(fb, x, 26, true);
            fb_pixel(fb, x, 30, true);
        }
    } else fb_text(fb, 56, 19, "LOBBY", 1);
    char guests[18];
    (void)snprintf(guests, sizeof(guests), "PLAYERS %u/%u",
                   (unsigned)scene_lobby_joined(), (unsigned)scene_lobby_seats());
    text_fit(fb, 55, 37, guests, 12);
    const char *code = scene_lobby_code();
    if (code && *code) {
        fb_text(fb, 55, 46, "CODE", 1);
        text_fit(fb, 84, 46, code, 7);
    } else fb_text(fb, 55, 46, "WAITING...", 1);
    text_fit(fb, 3, 57, scene_label(), 20);
}

static void received(uint8_t *fb, const scene_visual_t *v)
{
    const bool gift = v->show == SHOW_GIFTED;
    frame(fb, "POCKET CALM", gift ? "GIFT" : "DONE");
    fb_rect(fb, 3, 15, 55, 38);
    if (!sprite_in(fb, gift ? SLOT_GIFT : SLOT_THEIRS, 6, 17, 49, 34))
        ball(fb, 31, 33, 10);
    fb_text(fb, 67, 21, gift ? "DELIVERED" : "RECEIVED", 1);
    fb_text(fb, 67, 34, "SUCCESS!", 1);
    text_fit(fb, 67, 46, scene_label(), 10);
    fb_text(fb, 3, 57, "POKELDN", 1);
}

static void gift(uint8_t *fb)
{
    frame(fb, "POCKET CALM", "GIFT");
    fb_rect(fb, 4, 16, 52, 37);
    if (!sprite_in(fb, SLOT_GIFT, 7, 19, 46, 31)) {
        fb_rect(fb, 18, 29, 24, 19);
        fb_fill(fb, 16, 25, 28, 6, true);
        fb_fill(fb, 29, 25, 3, 23, false);
        fb_fill(fb, 30, 25, 1, 23, true);
    }
    fb_text(fb, 62, 22, "MYSTERY", 1);
    fb_text(fb, 62, 33, "GIFT", 1);
    text_fit(fb, 62, 44, scene_label(), 10);
    fb_text(fb, 3, 57, "POKELDN", 1);
}

void pocket_oled_render(uint8_t *fb, const scene_radio_t *radio,
                        const scene_visual_t *visual, uint32_t now_ms)
{
    if (!fb || !radio || !visual) return;
    if (visual->show == SHOW_LOBBY) raid(fb);
    else if (visual->show == SHOW_GIFTED) received(fb, visual);
    else if (visual->show == SHOW_GIFT) gift(fb);
    else if (visual->show == SHOW_TRADE || visual->show == SHOW_TRADED) {
        if (visual->show == SHOW_TRADED &&
            visual->elapsed_ms >= visual->reveal_ms + 1600u) received(fb, visual);
        else trading(fb, visual);
    } else home(fb, radio, now_ms);
}
