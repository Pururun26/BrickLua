#include "video.h"
#include "palette.h"
#include <string.h>

static uint8_t  g_fb[BRICKLUA_W * BRICKLUA_H];        /* 0/1 на пиксель */
static uint32_t g_pixels[BRICKLUA_W * BRICKLUA_H];    /* XRGB8888 */

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

void bricklua_video_pset(int x, int y, int color) {
    if (x < 0 || x >= BRICKLUA_W || y < 0 || y >= BRICKLUA_H) return;
    g_fb[y * BRICKLUA_W + x] = (uint8_t)(color & 1);
}

int bricklua_video_pget(int x, int y) {
    if (x < 0 || x >= BRICKLUA_W || y < 0 || y >= BRICKLUA_H) return 0;
    return g_fb[y * BRICKLUA_W + x];
}

const void *bricklua_video_get_framebuffer(void) {
    /* Конвертируем fb (0/1) в XRGB8888 и сразу рисуем "железную" рамку */
    for (int i = 0; i < BRICKLUA_W * BRICKLUA_H; i++) {
        int x = i % BRICKLUA_W;
        int y = i / BRICKLUA_W;

        // Условия для рамки (всегда цвет переднего плана, индекс 1)
        // Левая стена (x=0), правая (x=239), верх (y=0), низ (y=159)
        // И разделитель между игрой и текстом (x=159)
        if (x == 0 || x == 239 || y == 0 || y == 159 || x == 159) {
            g_pixels[i] = bricklua_palette[1];
        } else {
            g_pixels[i] = bricklua_palette[g_fb[i] & 1];
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