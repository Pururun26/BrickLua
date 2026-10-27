#pragma once
#include <stdint.h>
#include <stddef.h>

/* Save: 64 слота × int64 = 512 байт. */
#define BRICKLUA_SAVE_SLOTS  64

/* Спрайты: 8x8, 256 слотов. */
#define BRICKLUA_NUM_SPRITES  256
#define BRICKLUA_SPRITE_SIZE  8
#define BRICKLUA_TRANSPARENT   255

#define BRICKLUA_FPS 30

/* Графика */
void bricklua_cls  (int color);
void bricklua_pset (int x, int y, int color);
int  bricklua_pget (int x, int y);
void bricklua_brick(int gx, int gy, int color);
void bricklua_print(const char *s, int x, int y, int color);

/* Ввод: 6 кнопок (0..5). */
void bricklua_btn_update(int mask);   /* 1 раз в кадр, до update() */
int  bricklua_btn (int i);

/* Время */
int64_t bricklua_time (void);    /* ms since cartridge load */
int64_t bricklua_utime(void);    /* Unix timestamp, seconds */
void    bricklua_time_reset(void);

/* Сохранение */
void    bricklua_dset(int slot, int64_t value);
int64_t bricklua_dget(int slot);

/* Для libretro: указатель на 512-байтный массив (retro_get_memory_*) */
void   *bricklua_save_data(void);
size_t  bricklua_save_size(void);


/* Звук */
void bricklua_beep(int freq, int dur, int vol);

/* Заполнить буфер interleaved stereo int16 (L,R,L,R,...). */
void bricklua_audio_render(int16_t *out, int frames);

/* Сколько сэмплов в секунду — для retro_get_system_av_info. */
#define BRICKLUA_SAMPLE_RATE 44100