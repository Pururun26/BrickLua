#include "bricklua_api.h"
#include "bricklua_font3x4.h"
#include "video.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

void bricklua_cls(int color) {
    bricklua_video_clear(color);
}

void bricklua_pset(int x, int y, int color) {
    bricklua_video_pset(x, y, color);
}

int bricklua_pget(int x, int y) {
    return bricklua_video_pget(x, y);
}


// ДОБАВЛЯЕМ БЛОК BRICK (КУБИКИ)
static const char* CUBE_PATTERN[7] = {
    "1111111",
    "1000001",
    "1011101",
    "1011101",
    "1011101",
    "1000001",
    "1111111"
};

void bricklua_brick(int gx, int gy, int color) {
    // Сетка строго 20x20 (левая часть экрана)
    if (gx < 0 || gx >= 20 || gy < 0 || gy >= 20) return;

    int px = gx * 8;
    int py = gy * 8;

    if (color == 0) {
        // Стираем кубик (заливаем фоном)
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 7; x++) {
                bricklua_video_pset(px + x, py + y, 0);
            }
        }
    } else {
        // Рисуем кубик
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 7; x++) {
                if (CUBE_PATTERN[y][x] == '1') {
                    bricklua_video_pset(px + x, py + y, color);
                }
            }
        }
    }
}

/* ---------- print ---------- */
void bricklua_print(const char *s, int x, int y, int color) {
    if (!s) return;
    int cx = x, cy = y;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (*p == '\n') { cx = x; cy += FONT_CELL_H + 1; continue; }
        if (*p == '\r') continue;

        unsigned char u = *p;
        if (u >= 'a' && u <= 'z') u -= 32;

        int idx = (int)u - FONT_FIRST;
        if (idx < 0 || idx >= FONT_COUNT) idx = '?' - FONT_FIRST;

        int w = bricklua_font_w[idx];
        for (int row = 0; row < FONT_CELL_H; row++) {
            unsigned char bits = bricklua_font[idx][row];
            for (int col = 0; col < FONT_CELL_W; col++) {
                if ((bits >> (FONT_CELL_W - 1 - col)) & 1)
                    bricklua_pset(cx + col, cy + row, color);
            }
        }

        cx += w + 1;
    }
}

/* ---------- input ---------- */
static int g_btn_curr = 0;
static int g_btn_prev = 0;

void bricklua_btn_update(int mask) {
    g_btn_prev = g_btn_curr;
    g_btn_curr = mask;
}

// Настройки автоповтора (DAS - Delayed Auto Shift)
#define BTN_REPEAT_DELAY 10 // Пауза перед началом движения (в кадрах)
#define BTN_REPEAT_RATE  5  // Скорость движения после паузы

static int btn_timer[6] = {0, 0, 0, 0, 0, 0};
static int btn_held[6]  = {0, 0, 0, 0, 0, 0};

// Умный автоповтор (заменяет старую логику)
int bricklua_btn(int i) {
    if (i < 0 || i >= 6) return 0;

    int is_down = (g_btn_curr & (1 << i)) ? 1 : 0;
    int was_down = (g_btn_prev & (1 << i)) ? 1 : 0;

    if (!is_down) {
        // Кнопка отпущена — сбрасываем таймеры
        btn_held[i] = 0;
        btn_timer[i] = 0;
        return 0;
    }

    if (!was_down) {
        // Кнопку только что нажали — срабатываем сразу (эффект btnp)
        btn_held[i] = 1;
        btn_timer[i] = 0;
        return 1;
    }

    // Кнопку держат
    if (btn_held[i]) {
        btn_timer[i]++;
        
        if (btn_timer[i] >= BTN_REPEAT_DELAY) {
            // Наступило время автоповтора
            if ((btn_timer[i] - BTN_REPEAT_DELAY) % BTN_REPEAT_RATE == 0) {
                return 1;
            }
        }
    }

    return 0;
}

/* ---------- time (ms since start) ---------- */

static int64_t bricklua_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + (int64_t)ts.tv_nsec / 1000000;
}

static int64_t g_start_ms = 0;

void bricklua_time_reset(void) {
    g_start_ms = bricklua_now_ms();
}

int64_t bricklua_time(void) {
    return bricklua_now_ms() - g_start_ms;
}

/* ---------- utime ---------- */
int64_t bricklua_utime(void) {
    return (int64_t)time(NULL);
}


/* ---------- save ---------- */
static int64_t g_save[BRICKLUA_SAVE_SLOTS];

void bricklua_dset(int slot, int64_t value) {
    if (slot < 0 || slot >= BRICKLUA_SAVE_SLOTS) return;
    g_save[slot] = value;
}

int64_t bricklua_dget(int slot) {
    if (slot < 0 || slot >= BRICKLUA_SAVE_SLOTS) return 0;
    return g_save[slot];
}

void  *bricklua_save_data(void) { return g_save; }
size_t bricklua_save_size(void) { return sizeof(g_save); }


/* ---------- beep ---------- */
typedef struct {
    int     active;
    float   phase;        /* 0..1 */
    int64_t remaining;    /* сэмплов до конца */
    int     freq;         /* 0 = noise */
    int     vol;          /* 0..100 */
} Tone;

static Tone g_tone = {0};

void bricklua_beep(int freq, int dur, int vol) {
    if (dur <= 0 || vol <= 0) {
        g_tone.active = 0;
        g_tone.remaining = 0;
        return;
    }
    if (freq < 0) freq = 0;
    if (freq > 8000) freq = 8000;
    if (dur  > 255) dur  = 255;
    if (vol  > 100) vol  = 100;

    float dur_sec = (float)dur / (float)BRICKLUA_FPS;
    int64_t samples = (int64_t)(BRICKLUA_SAMPLE_RATE * dur_sec);
    if (samples <= 0) return;

    g_tone.active    = 1;
    g_tone.phase     = 0.0f;
    g_tone.remaining = samples;
    g_tone.freq      = freq;
    g_tone.vol       = vol;
}

void bricklua_audio_render(int16_t *out, int frames) {
    for (int i = 0; i < frames; i++) {
        int16_t s = 0;

        if (g_tone.active && g_tone.remaining > 0) {
            float v;
            if (g_tone.freq == 0) {
                v = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;   /* noise */
            } else {
                v = (g_tone.phase < 0.5f) ? 1.0f : -1.0f;              /* square */
                g_tone.phase += (float)g_tone.freq / (float)BRICKLUA_SAMPLE_RATE;
                if (g_tone.phase >= 1.0f) g_tone.phase -= 1.0f;
            }
            s = (int16_t)(v * g_tone.vol * 327.67f);
            g_tone.remaining--;
        }

        out[i * 2]     = s;   /* L */
        out[i * 2 + 1] = s;   /* R */
    }
}