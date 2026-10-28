#include "video.h"
#include "palette.h"
#include <string.h>

static uint8_t  g_fb[BRICKLUA_W * BRICKLUA_H];        /* 0/1 на пиксель */
static uint32_t g_pixels[BRICKLUA_W * BRICKLUA_H];    /* XRGB8888 */

/* Паттерн кубика для призрачной сетки (7x7) */
static const char* CUBE_PATTERN[7] = {
    "1111111",
    "1000001",
    "1011101",
    "1011101",
    "1011101",
    "1000001",
    "1111111"
};

/* Проверяем, попадает ли пиксель в контур кубика */
static int is_ghost_pixel(int x, int y) {
    int lx = x % 8;
    int ly = y % 8;
    if (lx < 7 && ly < 7) {
        return (CUBE_PATTERN[ly][lx] == '1');
    }
    return 0;
}

void bricklua_video_init(void) {
    memset(g_fb, 0, sizeof(g_fb));
    memset(g_pixels, 0, sizeof(g_pixels));
}

void bricklua_video_deinit(void) {
    /* ничего */
}

void bricklua_video_clear(int color) {
    uint8_t c = (uint8_t)(color & 1);
    memset(g_fb, c, sizeof(g_fb));
}

void bricklua_video_bset(int x, int y, int color) {
    if (x < 0 || x >= BRICKLUA_W || y < 0 || y >= BRICKLUA_H) return;
    g_fb[y * BRICKLUA_W + x] = (uint8_t)(color & 1);
}

int bricklua_video_bget(int x, int y) {
    if (x < 0 || x >= BRICKLUA_W || y < 0 || y >= BRICKLUA_H) return 0;
    return g_fb[y * BRICKLUA_W + x];
}

const void *bricklua_video_get_framebuffer(void) {
    // Цвета для Libretro (XRGB8888)
    uint32_t color_bg    = bricklua_palette[0]; // Фон (например, #9ead86)
    uint32_t color_fg    = bricklua_palette[1]; // Передний план (#1a1a1a)
    uint32_t color_ghost = 0x00879372;          // Призрачные кирпичи
    uint32_t color_black = 0x00000000;          // Черный корпус (полосы)

    for (int i = 0; i < BRICKLUA_W * BRICKLUA_H; i++) {
        int x = i % BRICKLUA_W;
        int y = i / BRICKLUA_W;

        // 1. Нижняя граница и центральный разделитель (черный корпус)
        if (y == 159 || x == 159) {
            g_pixels[i] = color_black;
        } 
        // 2. Левая панель (Игровое поле, x от 0 до 158)
        else if (x < 159) {
            if (g_fb[i] & 1) {
                g_pixels[i] = color_fg;    // Активный кирпич
            } else if (is_ghost_pixel(x, y)) {
                g_pixels[i] = color_ghost; // Призрачный кирпич
            } else {
                g_pixels[i] = color_bg;    // Пустота
            }
        } 
        // 3. Правая панель (Текст, x от 160 до 239)
        else {
            g_pixels[i] = (g_fb[i] & 1) ? color_fg : color_bg;
        }
    }
    return g_pixels;
}

unsigned bricklua_video_get_width(void)  { return BRICKLUA_W; }
unsigned bricklua_video_get_height(void) { return BRICKLUA_H; }
size_t   bricklua_video_get_pitch(void)  { return BRICKLUA_W * sizeof(uint32_t); }

unsigned char *bricklua_video_fb(void) {
    return g_fb;
}