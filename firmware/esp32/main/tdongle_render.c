/* PokéLDN T-Dongle S3 — v0.6 "Pocket Calm": English 160x80 UI.
   PokéLDN sends one-bit sprites; we rebuild bounded colour regions using
   flood fill, preserving the original dark details. During trades, the two
   sprites use the original v0.1 one-bit pixel rendering during trades,
   with a restrained high-contrast palette and a flying centre Poké Ball.
   No new USB command or change to the PokéLDN wireless protocol is required. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "scene.h"
#include "screen.h"
#include "tdongle_render.h"

#define RGB8(r,g,b) ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))
#define NAVY    RGB8(18, 43, 67)
#define NAVY2   RGB8(34, 61, 85)
#define CREAM   RGB8(247, 238, 214)
#define WHITE   RGB8(255, 251, 236)
#define RED     RGB8(212, 77, 74)
#define GOLD    RGB8(232, 182, 78)
#define SHADE   RGB8(174, 158, 134)
#define SLATE   RGB8(88, 105, 135)
#define SUCCESS RGB8(65, 130, 97)
#define C_PINK  RGB8(244, 171, 188)
#define C_BLUE  RGB8(79, 160, 214)
#define C_GREEN RGB8(89, 167, 111)
#define C_LIME  RGB8(166, 198, 103)
#define C_ORANGE RGB8(235, 148, 67)
#define C_PURPLE RGB8(145, 112, 177)
#define C_BROWN RGB8(170, 119, 80)
#define C_YELLOW RGB8(253, 208, 86)

static void put(uint8_t *out, int x, int y, uint16_t color)
{
    if (x < 0 || y < 0 || x >= TDONGLE_W || y >= TDONGLE_H) return;
    const int p = (y * TDONGLE_W + x) * 2;
    out[p] = (uint8_t)(color >> 8);
    out[p + 1] = (uint8_t)color;
}

static void fill(uint8_t *out, int x, int y, int w, int h, uint16_t color)
{
    for (int py = y; py < y + h; ++py)
        for (int px = x; px < x + w; ++px) put(out, px, py, color);
}

static void border(uint8_t *out, int x, int y, int w, int h, uint16_t color)
{
    fill(out, x, y, w, 1, color);
    fill(out, x, y + h - 1, w, 1, color);
    fill(out, x, y, 1, h, color);
    fill(out, x + w - 1, y, 1, h, color);
}

/* The original PokéLDN 5x7 font is rendered to a tiny temporary mask.  This
   keeps the UI pixel-perfect without embedding a second font in flash. */
static void text(uint8_t *out, int x, int y, const char *source, int scale, uint16_t color)
{
    uint8_t mask[SCREEN_BYTES];
    char ascii[22];
    int n = 0;
    if (!source) return;
    while (*source && n < (int)sizeof(ascii) - 1) {
        const unsigned char c = (unsigned char)*source++;
        if (c >= 'a' && c <= 'z') ascii[n++] = (char)(c - 'a' + 'A');
        else if (c >= 32 && c <= 126) ascii[n++] = (char)c;
        else if (c == 0xc3 && *source) {
            /* Latin-1 accents in French Pokémon names: display the base glyph
               without consuming two character cells on this 160px LCD. */
            const unsigned char d = (unsigned char)*source++;
            if ((d >= 0x80 && d <= 0x85) || (d >= 0xa0 && d <= 0xa5)) ascii[n++] = 'A';
            else if (d == 0x87 || d == 0xa7) ascii[n++] = 'C';
            else if ((d >= 0x88 && d <= 0x8b) || (d >= 0xa8 && d <= 0xab)) ascii[n++] = 'E';
            else if ((d >= 0x8c && d <= 0x8f) || (d >= 0xac && d <= 0xaf)) ascii[n++] = 'I';
            else if (d == 0x91 || d == 0xb1) ascii[n++] = 'N';
            else if ((d >= 0x92 && d <= 0x96) || (d >= 0xb2 && d <= 0xb6)) ascii[n++] = 'O';
            else if ((d >= 0x99 && d <= 0x9c) || (d >= 0xb9 && d <= 0xbc)) ascii[n++] = 'U';
            else ascii[n++] = '?';
        } else if (c >= 0x80) ascii[n++] = '?';
    }
    ascii[n] = 0;
    fb_clear(mask);
    fb_text(mask, 0, 0, ascii, scale);
    const int width = fb_text_width(ascii, scale);
    for (int py = 0; py < 7 * scale; ++py)
        for (int px = 0; px < width && px < SCREEN_W; ++px)
            if (fb_get(mask, px, py)) put(out, x + px, y + py, color);
}

static void star(uint8_t *out, int x, int y, uint16_t color)
{
    put(out, x, y, color);
    put(out, x - 2, y, color);
    put(out, x + 2, y, color);
    put(out, x, y - 2, color);
    put(out, x, y + 2, color);
}

static void ball(uint8_t *out, int cx, int cy, int r)
{
    for (int dy = -r; dy <= r; ++dy)
        for (int dx = -r; dx <= r; ++dx) {
            const int dd = dx * dx + dy * dy;
            if (dd > r * r) continue;
            uint16_t color = dy < -1 ? RED : dy > 1 ? WHITE : NAVY;
            if (dd >= (r - 2) * (r - 2)) color = NAVY;
            put(out, cx + dx, cy + dy, color);
        }
    for (int dy = -2; dy <= 2; ++dy)
        for (int dx = -2; dx <= 2; ++dx)
            if (dx * dx + dy * dy <= 4) put(out, cx + dx, cy + dy, WHITE);
    put(out, cx, cy, NAVY);
}

static void corners(uint8_t *out, int x, int y, int w, int h, uint16_t color)
{
    for (int d = 0; d < 3; ++d) {
        put(out, x + 2 + d, y + 2, color);
        put(out, x + 2, y + 2 + d, color);
        put(out, x + w - 3 - d, y + 2, color);
        put(out, x + w - 3, y + 2 + d, color);
        put(out, x + 2 + d, y + h - 3, color);
        put(out, x + 2, y + h - 3 - d, color);
        put(out, x + w - 3 - d, y + h - 3, color);
        put(out, x + w - 3, y + h - 3 - d, color);
    }
}

static void header(uint8_t *out, const char *title)
{
    fill(out, 0, 0, 160, 80, NAVY);
    fill(out, 5, 24, 150, 52, CREAM);
    border(out, 5, 24, 150, 52, WHITE);
    corners(out, 6, 25, 148, 50, SHADE);
    ball(out, 17, 12, 8);
    text(out, 31, 5, title, 2, WHITE);
    for (int j = 0; j < 3; ++j)
        for (int y = 5; y < 20; ++y) {
            const int x = 145 - j * 7 - (y - 5) / 2;
            fill(out, x, y, 3, 1, RED);
        }
}

/* Simplified art-directed palettes. Names may be English or localized by the
   PC; this changes colour only and never affects a Pokemon's identity. */
typedef struct { const char *match; uint16_t body, light, accent; } palette_t;
static const palette_t palettes[] = {
    {"zorua",      SLATE, NAVY2, RED},
    {"pikachu",    C_YELLOW, GOLD, RED},
    {"eevee",      C_BROWN, CREAM, C_ORANGE}, {"voli", C_BROWN, CREAM, GOLD},
    {"charizard",  C_ORANGE, C_YELLOW, C_BLUE}, {"dracaufeu", C_ORANGE, C_YELLOW, C_BLUE},
    {"charmander", C_ORANGE, C_YELLOW, RED}, {"salameche", C_ORANGE, C_YELLOW, RED},
    {"squirtle",   C_BLUE, CREAM, C_BROWN}, {"carapuce", C_BLUE, CREAM, C_BROWN},
    {"bulbasaur",  C_GREEN, C_LIME, C_BLUE}, {"bulbizarre", C_GREEN, C_LIME, C_BLUE},
    {"gengar",     C_PURPLE, SLATE, RED}, {"ectoplasma", C_PURPLE, SLATE, RED},
    {"mewtwo",     CREAM, WHITE, C_PURPLE}, {"mew", C_PINK, CREAM, RED},
    {"lucario",    C_BLUE, CREAM, SLATE},
    {"gardevoir",  CREAM, WHITE, C_GREEN},
    {"rayquaza",   C_GREEN, C_LIME, GOLD},
    {"magikarp",   C_ORANGE, C_YELLOW, RED}, {"magicarpe", C_ORANGE, C_YELLOW, RED},
    {"gyarados",   C_BLUE, CREAM, C_YELLOW}, {"leviator", C_BLUE, CREAM, GOLD},
    {"riolu",      C_BLUE, CREAM, SLATE},
    {"torchic",    C_ORANGE, C_YELLOW, C_BROWN}, {"poussifeu", C_ORANGE, C_YELLOW, C_BROWN},
    {"greninja",   C_BLUE, SLATE, C_PINK}, {"amphinobi", C_BLUE, SLATE, C_PINK},
    {"snorlax",    SLATE, CREAM, C_BLUE}, {"ronflex", SLATE, CREAM, C_BLUE},
    {"psyduck",    C_YELLOW, CREAM, C_ORANGE}, {"psykokwak", C_YELLOW, CREAM, C_ORANGE},
    {"vulpix",     C_ORANGE, CREAM, RED}, {"goupix", C_ORANGE, CREAM, RED},
    {"jigglypuff", C_PINK, CREAM, C_BLUE}, {"rondoudou", C_PINK, CREAM, C_BLUE},
    {"scorbunny",  CREAM, WHITE, C_ORANGE}, {"flambino", CREAM, WHITE, C_ORANGE},
    {"sprigatito", C_GREEN, C_LIME, CREAM}, {"poussacha", C_GREEN, C_LIME, CREAM},
};

static bool contains_ci(const char *haystack, const char *needle)
{
    if (!haystack || !needle || !*needle) return false;
    for (const char *s = haystack; *s; ++s) {
        const char *a = s, *b = needle;
        while (*a && *b) {
            const unsigned char ca = (unsigned char)*a;
            const unsigned char cb = (unsigned char)*b;
            if ((ca >= 'A' && ca <= 'Z' ? ca + 32 : ca) !=
                (cb >= 'A' && cb <= 'Z' ? cb + 32 : cb)) break;
            ++a;
            ++b;
        }
        if (!*b) return true;
    }
    return false;
}

static palette_t palette_for(const char *name)
{
    for (size_t i = 0; i < sizeof(palettes) / sizeof(palettes[0]); ++i)
        if (contains_ci(name, palettes[i].match)) return palettes[i];
    return (palette_t){"", GOLD, CREAM, C_ORANGE};
}

/* Each zero pixel connected to the source sprite's border is transparent.
   Other zero pixels lie INSIDE its contour and receive the species palette.
   This restores a readable painted body and retains inked facial details.
   Two statically allocated buffers avoid large allocations on ESP32's stack. */
static uint8_t outside[SPRITE_W * SPRITE_H];
static uint16_t queue[SPRITE_W * SPRITE_H];

static bool ink(const uint8_t *bits, int stride, int x, int y)
{
    return (bits[y * stride + x / 8] >> (7 - x % 8) & 1u) != 0;
}

static void mark_outside(const uint8_t *bits, int w, int h, int stride)
{
    const int n = w * h;
    memset(outside, 0, (size_t)n);
    int head = 0, tail = 0;
    /* Seed all transparent border pixels (no artificial white rectangles). */
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            if (x && y && x < w - 1 && y < h - 1) continue;
            const int i = y * w + x;
            if (!ink(bits, stride, x, y) && !outside[i]) {
                outside[i] = 1;
                queue[tail++] = (uint16_t)i;
            }
        }
    while (head < tail) {
        const int at = queue[head++];
        const int x = at % w, y = at / w;
        const int near[4] = {at - 1, at + 1, at - w, at + w};
        for (int d = 0; d < 4; ++d) {
            if ((d == 0 && x == 0) || (d == 1 && x == w - 1) ||
                (d == 2 && y == 0) || (d == 3 && y == h - 1)) continue;
            const int j = near[d];
            if (!outside[j] && !ink(bits, stride, j % w, j / w)) {
                outside[j] = 1;
                queue[tail++] = (uint16_t)j;
            }
        }
    }
}

static bool sprite_art(uint8_t *out, uint8_t slot, int cx, int bottom,
                       int maxw, int maxh, const char *name, bool monochrome)
{
    uint8_t w = 0, h = 0;
    const uint8_t *bits = scene_sprite_bits(slot, &w, &h);
    if (!bits || !w || !h) return false;
    const int fw = maxw * 256 / w, fh = maxh * 256 / h;
    const int factor = fw < fh ? fw : fh;
    const int dw = w * factor / 256 > 0 ? w * factor / 256 : 1;
    const int dh = h * factor / 256 > 0 ? h * factor / 256 : 1;
    const int stride = (w + 7) / 8;
    const palette_t pal = monochrome
        ? (palette_t){"", SLATE, CREAM, SLATE}
        : palette_for(name);
    mark_outside(bits, w, h, stride);
    /* Rounded shadow under the Pokemon anchors the enlarged sprite. */
    fill(out, cx - dw / 2 + 3, bottom + 1, dw - 6 > 0 ? dw - 6 : 1, 2, SHADE);
    for (int y = 0; y < dh; ++y)
        for (int x = 0; x < dw; ++x) {
            const int sx = x * w / dw, sy = y * h / dh;
            const bool dark = ink(bits, stride, sx, sy);
            const bool interior = !outside[sy * w + sx];
            if (!interior) continue;
            uint16_t color;
            if (dark) {
                /* Solid 1-bit blocks are filled; sparse lines remain ink. */
                int density = 0;
                for (int yy = sy > 0 ? sy - 1 : 0; yy <= sy + 1 && yy < h; ++yy)
                    for (int xx = sx > 0 ? sx - 1 : 0; xx <= sx + 1 && xx < w; ++xx)
                        density += ink(bits, stride, xx, yy);
                color = density >= 8 ? pal.body : NAVY;
            } else {
                color = sy > h * 3 / 4 ? pal.light : pal.body;
            }
            if (!monochrome && color != NAVY &&
                sy < h / 3 && sx > w / 4 && sx < w * 3 / 4)
                color = pal.accent;
            put(out, cx - dw / 2 + x, bottom - dh + y, color);
        }
    return true;
}

static bool sprite(uint8_t *out, uint8_t slot, int cx, int bottom,
                   int maxw, int maxh, const char *name)
{
    return sprite_art(out, slot, cx, bottom, maxw, maxh, name, false);
}

static void home(uint8_t *out, const scene_visual_t *v, uint32_t now)
{
    header(out, "POKELDN");
    const bool linked = v && v->radio_mode == SCENE_JOINED;
    const bool searching = v && v->radio_mode == SCENE_JOINING;
    const bool hosting = v && v->radio_mode == SCENE_HOSTING;
    const bool sniffing = v && v->radio_mode == SCENE_SNIFFING;
    const char *status = linked ? "LINKED" : searching ? "SCAN" :
                         hosting ? "HOST" : sniffing ? "SNIFF" : "READY";
    fill(out, 16, 35, 72, 32, GOLD);
    fill(out, 18, 37, 68, 28, NAVY);
    text(out, 18 + (68 - (int)strlen(status) * 12) / 2, 43, status, 2, GOLD);
    ball(out, 121, 50 + (int)(now / 500 % 2), 15);
    if ((now / 350) % 3 != 0) {
        fill(out, 98, 39, 2, 15, GOLD);
        fill(out, 143, 39, 2, 15, GOLD);
    }
    if ((now / 350) % 3 == 2) {
        fill(out, 94, 35, 2, 23, GOLD);
        fill(out, 147, 35, 2, 23, GOLD);
    }
}

static void small_name(uint8_t *out, int x, int y, int width, const char *name)
{
    fill(out, x, y, width, 13, NAVY);
    border(out, x, y, width, 13, GOLD);
    char cut[22] = {0};
    if (!name || !*name) name = "POKEMON";
    int i = 0;
    while (i < (width - 8) / 6 && i < 20 && name[i]) {
        cut[i] = name[i];
        ++i;
    }
    text(out, x + 4, y + 3, cut, 1, WHITE);
}

/* The v0.1 sprite used the exact bits supplied by PokéLDN, without
   flood-fill, colour guessing or outline rewriting. Keep that look: cream
   pixels on navy. Only crop empty margins and resize with nearest-neighbour
   (for Pokemon bigger than the two 60x48 trade windows). */
static bool sprite_v01(uint8_t *out, uint8_t slot, int x0, int y0,
                       int maxw, int maxh)
{
    uint8_t w = 0, h = 0;
    const uint8_t *bits = scene_sprite_bits(slot, &w, &h);
    if (!bits || !w || !h) return false;
    const int stride = (w + 7) / 8;
    int xmin = w, ymin = h, xmax = -1, ymax = -1;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            if (ink(bits, stride, x, y)) {
                if (x < xmin) xmin = x;
                if (x > xmax) xmax = x;
                if (y < ymin) ymin = y;
                if (y > ymax) ymax = y;
            }
    if (xmax < xmin) return false;
    const int sw = xmax - xmin + 1, sh = ymax - ymin + 1;
    int dw = sw, dh = sh;
    if (dw > maxw || dh > maxh) {
        const int scale_w = maxw * 1024 / sw;
        const int scale_h = maxh * 1024 / sh;
        const int scale = scale_w < scale_h ? scale_w : scale_h;
        dw = sw * scale / 1024;
        dh = sh * scale / 1024;
    } else {
        /* At most 2x for tiny pixel sprites; no smoothing. */
        while (dw * 2 <= maxw && dh * 2 <= maxh) { dw *= 2; dh *= 2; }
    }
    if (dw < 1) dw = 1;
    if (dh < 1) dh = 1;
    const int dx = x0 + (maxw - dw) / 2;
    const int dy = y0 + (maxh - dh) / 2;
    for (int y = 0; y < dh; ++y)
        for (int x = 0; x < dw; ++x)
            if (ink(bits, stride, xmin + x * sw / dw, ymin + y * sh / dh))
                put(out, dx + x, dy + y, WHITE);
    return true;
}

/* Compact trade header and name plates leave nearly 50 vertical pixels of
   uninterrupted room for each Pokemon. The centre lane is 26 pixels wide. */
static void trading(uint8_t *out, const scene_visual_t *v, uint32_t now)
{
    fill(out, 0, 0, 160, 80, CREAM);
    fill(out, 0, 0, 160, 17, NAVY);
    ball(out, 12, 8, 6);
    text(out, 57, 5, "TRADE", 1, WHITE);
    fill(out, 4, 19, 62, 50, NAVY);
    fill(out, 94, 19, 62, 50, NAVY);
    border(out, 3, 18, 64, 52, GOLD);
    border(out, 93, 18, 64, 52, GOLD);

    const char *ours = scene_sprite_name(SLOT_OURS);
    const char *theirs = scene_sprite_name(SLOT_THEIRS);
    if (!ours || !*ours) ours = scene_label();
    if (!theirs || !*theirs) theirs = "INCOMING";
    if (!sprite_v01(out, SLOT_OURS, 5, 20, 60, 47))
        ball(out, 35, 43, 11);
    if (!sprite_v01(out, SLOT_THEIRS, 95, 20, 60, 47))
        ball(out, 125, 43, 11);
    small_name(out, 3, 68, 64, ours);
    small_name(out, 93, 68, 64, theirs);

    /* Keep the flying ball from v0.5. The narrow movement avoids obscuring
       either side's original v0.1 sprite pixels. */
    const bool traded = v && v->show == SHOW_TRADED;
    const bool leaving = traded && v->elapsed_ms < 2100;
    const bool arriving = traded && v->elapsed_ms >= v->reveal_ms;
    int bx = 80, by = 44;
    if (leaving) {
        const uint32_t step = v->elapsed_ms > 2100 ? 2100 : v->elapsed_ms;
        bx = 73 + (int)(step * 14 / 2100);
        by = 44 - (int)(step / 400 % 3);
    } else if (arriving) {
        const uint32_t elapsed = v->elapsed_ms - v->reveal_ms;
        const uint32_t step = elapsed > 1600 ? 1600 : elapsed;
        bx = 87 - (int)(step * 14 / 1600);
        by = 43 + (int)(step / 350 % 3);
    } else {
        by = 43 + (int)(now / 600 % 2);
    }
    for (int x = 72; x <= 89; x += 7)
        put(out, x, 60, GOLD);
    if ((now / 500 % 3) == 0)
        star(out, 80, 26, GOLD);
    ball(out, bx, by, 7);
}

static void gift(uint8_t *out, uint32_t now)
{
    header(out, "GIFT");
    /* The gift box stays clearly legible even when PokéLDN sends no sprite. */
    fill(out, 18, 40, 50, 30, RED);
    border(out, 18, 40, 50, 30, NAVY);
    fill(out, 15, 35, 56, 8, RED);
    border(out, 15, 35, 56, 8, NAVY);
    fill(out, 40, 35, 6, 35, GOLD);
    fill(out, 18, 40, 50, 2, WHITE);
    fill(out, 87, 32, 59, 35, NAVY);
    fill(out, 89, 34, 55, 31, CREAM);
    text(out, 94, 38, "PRESENT", 1, NAVY);
    char name[10] = {0};
    const char *label = scene_label();
    for (int i = 0; i < 9 && label && label[i]; ++i) name[i] = label[i];
    text(out, 94, 52, name[0] ? name : "MYSTERY", 1, RED);
    if ((now / 250) % 2) star(out, 76, 29, GOLD);
}

static void raid(uint8_t *out, uint32_t now)
{
    header(out, "RAID");
    if (!sprite(out, SLOT_OURS, 42, 69, 69, 43, scene_sprite_name(SLOT_OURS)))
        ball(out, 44, 48 + (int)(now / 400 % 2), 16);
    fill(out, 83, 34, 63, 36, NAVY);
    fill(out, 85, 36, 59, 32, WHITE);
    text(out, 90, 39, "PLAYERS", 1, NAVY);
    const char number[] = {(char)('0' + scene_lobby_joined()), '/',
                           (char)('0' + scene_lobby_seats()), '\0'};
    text(out, 102, 49, number, 1, RED);
    const char *code = scene_lobby_code();
    if (code && code[0]) {
        char cut[10] = {0};
        for (int i = 0; i < 9 && code[i]; ++i) cut[i] = code[i];
        text(out, 88, 60, cut, 1, NAVY);
    }
}

static void received(uint8_t *out, const scene_visual_t *v, uint32_t now)
{
    const bool gift = v && v->show == SHOW_GIFTED;
    fill(out, 0, 0, 160, 80, NAVY);
    fill(out, 3, 3, 154, 74, RED);
    fill(out, 7, 7, 146, 66, CREAM);
    border(out, 9, 9, 142, 62, RED);
    corners(out, 10, 10, 140, 60, GOLD);
    const uint8_t slot = gift ? SLOT_GIFT : SLOT_THEIRS;
    const char *name = scene_sprite_name(slot);
    if (!name || !*name) name = scene_label();
    if (!sprite(out, slot, 44, 70, 72, 60, name))
        ball(out, 43, 47, 18);
    fill(out, 80, 25, 68, 24, RED);
    border(out, 81, 26, 66, 22, WHITE);
    text(out, gift ? 87 : 91, 30, gift ? "SENT" : "GOT!", 2, WHITE);
    fill(out, 80, 54, 68, 17, NAVY);
    fill(out, 82, 56, 64, 13, WHITE);
    char cut[11] = {0};
    if (!name || !*name) name = "POKEMON";
    for (int i = 0; i < 10 && name[i]; ++i) cut[i] = name[i];
    text(out, 85, 59, cut, 1, NAVY);
    const int step = (int)(now / 170 % 3);
    star(out, 18, 30 + step, GOLD);
    star(out, 72, 55 - step, GOLD);
    star(out, 13, 63 - step, GOLD);
}

void tdongle_render(const uint8_t *frame, uint8_t *out,
                    const scene_visual_t *visual, uint32_t now_ms)
{
    (void)frame; /* The original monochrome frame still drives other panels. */
    if (visual && (visual->show == SHOW_GIFTED ||
       (visual->show == SHOW_TRADED &&
        visual->elapsed_ms >= visual->reveal_ms + 1600)))
        received(out, visual, now_ms);
    else if (visual && visual->show == SHOW_GIFT)
        gift(out, now_ms);
    else if (visual && visual->show == SHOW_LOBBY)
        raid(out, now_ms);
    else if (visual && (visual->show == SHOW_TRADE || visual->show == SHOW_TRADED))
        trading(out, visual, now_ms);
    else
        home(out, visual, now_ms);
}
