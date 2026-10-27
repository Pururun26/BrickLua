#pragma once

#include "bricklua_conf.h"
#include "raylib.h"

#include <stdint.h>

extern unsigned char g_fb[BRICKLUA_WIDTH * BRICKLUA_HEIGHT];
extern Color         g_palette[BRICKLUA_COLOR_COUNT];

/* InitWindow() -> bricklua_api_init() -> ... -> bricklua_api_shutdown() -> CloseWindow() */
void bricklua_api_init(void);
void bricklua_api_shutdown(void);

/* Вызывать между BeginDrawing() и EndDrawing(). */
void bricklua_present(void);

void bricklua_fps_frame(void);
int  bricklua_fps_get(void);

void bricklua_cls (int color);
void bricklua_print(const char *s, int x, int y, int color);
void bricklua_pset(int x, int y, int color);
int  bricklua_pget(int x, int y);

void bricklua_sprdata(int id, const char *art);
void bricklua_spr(int id, int gx, int gy);

void bricklua_line(int x0, int y0, int x1, int y1);
void bricklua_rect(int x, int y, int w, int h, int filled);
void bricklua_oval(int x, int y, int w, int h, int filled);

/* --- Сетка Brick Game (Кубики 8x8) --- */
void bricklua_brick(int gx, int gy, int color);
void bricklua_cls_grid(int color);

int64_t bricklua_time (void);
int64_t bricklua_utime(void);
void    bricklua_time_reset(void);

void bricklua_poll_buttons(void);
int  bricklua_btn (int i);

void bricklua_beep(int freq, int dur, int vol);