#pragma once
#include <stdint.h>
#include <stddef.h>

/* Экран BrickLua: 160x160 (10x20 сетка кубиков + панель текста) */
#define BRICKLUA_W 160
#define BRICKLUA_H 160

/* Прямой доступ к 0/1 framebuffer (BRICKLUA_W × BRICKLUA_H байт, row-major).
 * Caller отвечает за bounds-check. Используется для быстрых примитивов. */
unsigned char *bricklua_video_fb(void);

void        bricklua_video_init(void);
void        bricklua_video_deinit(void);

/* Залить весь fb цветом (0 или 1). */
void        bricklua_video_clear(int color);

/* Поставить кирпич (0 или 1), клипается по границам. */
void        bricklua_video_bset(int x, int y, int color);

/* Прочитать кирпич, 0 или 1. */
int         bricklua_video_bget(int x, int y);

/* Готовый буфер для video_cb() — XRGB8888, width*height*4 байт. */
const void *bricklua_video_get_framebuffer(void);
unsigned    bricklua_video_get_width(void);
unsigned    bricklua_video_get_height(void);
size_t      bricklua_video_get_pitch(void);   /* width * 4 */