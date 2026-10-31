#include "bricklua_api.h"
#include "bricklua_font3x4.h"
#include "video.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

void bricklua_cls(void) {
    bricklua_video_clear(0);
}

void bricklua_pixel(int x, int y) {
    bricklua_video_bset(x, y, 1);
}

int bricklua_bget(int x, int y) {
    // Сетка 10x20
    if (x < 0 || x >= 10 || y < 0 || y >= 20) return 0;
    
    // Переводим в пиксели и берем центр кирпича
    int px = x * 8 + 3;
    int py = y * 8 + 3;
    
    // Проверяем пиксель (используем функцию из video.c для надежности)
    return bricklua_video_bget(px, py);
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

void bricklua_brick(int gx, int gy) {
    // Сетка 10x20
    if (gx < 0 || gx >= 10 || gy < 0 || gy >= 20) return;

    int px = gx * 8;
    int py = gy * 8;

    for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 7; x++) {
            if (CUBE_PATTERN[y][x] == '1') {
                bricklua_pixel(px + x, py + y);
            }
        }
    }
}

/* ---------- BRICK SPRITES (4x4) ---------- */
static uint8_t g_brick_sprites[MAX_BRICK_SPRITES][BRICK_SPRITE_SIZE][BRICK_SPRITE_SIZE];

void bricklua_sprdata(int id, const char *art) {
    if (id < 0 || id >= MAX_BRICK_SPRITES || !art) return;

    // Очищаем спрайт перед загрузкой
    memset(g_brick_sprites[id], 0, sizeof(g_brick_sprites[id]));

    int row = 0, col = 0;
    for (const unsigned char *p = (const unsigned char *)art; *p; ) {
        if (*p == '\n') {
            if (col > 0) { row++; col = 0; } // Переход на новую строку
            p++;
            continue;
        }
        if (*p == '\r') { p++; continue; }

        unsigned char v;
        // # или 1 — это кирпич
        if (p[0] == '#' || p[0] == '1') {
            v = 1;
        } 
        // 0, точка или пробел — пустота (фон)
        else if (p[0] == '0' || p[0] == '.' || p[0] == ' ') {
            v = 0;
        } 
        // Неизвестный символ — пропускаем, чтобы не ломать строку
        else { 
            p++; 
            continue; 
        }

        if (row < BRICK_SPRITE_SIZE && col < BRICK_SPRITE_SIZE) {
            g_brick_sprites[id][row][col] = v;
        }

        col++;
        p++;
    }
}

void bricklua_spr(int id, int gx, int gy) {
    if (id < 0 || id >= MAX_BRICK_SPRITES) return;

    for (int y = 0; y < BRICK_SPRITE_SIZE; y++) {
        for (int x = 0; x < BRICK_SPRITE_SIZE; x++) {
            // Рисуем только если там есть кирпич (1)
            if (g_brick_sprites[id][y][x] == 1) {
                bricklua_brick(gx + x, gy + y);
            }
        }
    }
}


/* ---------- line (bricks) ---------- */
void bricklua_line(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        bricklua_brick(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* ---------- rect (bricks) ---------- */
void bricklua_rect(int x, int y, int w, int h, int filled) {
    if (w <= 0 || h <= 0) return;
    int x_end = x + w - 1;
    int y_end = y + h - 1;

    if (filled) {
        for (int yy = y; yy <= y_end; yy++) {
            for (int xx = x; xx <= x_end; xx++) {
                bricklua_brick(xx, yy);
            }
        }
    } else {
        for (int xx = x; xx <= x_end; xx++) {
            bricklua_brick(xx, y);     // Верх
            bricklua_brick(xx, y_end); // Низ
        }
        for (int yy = y + 1; yy < y_end; yy++) {
            bricklua_brick(x, yy);     // Лево
            bricklua_brick(x_end, yy); // Право
        }
    }
}

/* ---------- oval (bricks) ---------- */
void bricklua_oval(int x, int y, int w, int h, int filled) {
    if (w <= 0 || h <= 0) return;

    double cx = x + w / 2.0;
    double cy = y + h / 2.0;
    double rx = w / 2.0;
    double ry = h / 2.0;
    if (rx < 0.5) rx = 0.5;
    if (ry < 0.5) ry = 0.5;

    double inner_rx = rx - 1.0;
    double inner_ry = ry - 1.0;

    for (int py = y; py < y + h; py++) {
        double dy = (py + 0.5 - cy) / ry;
        if (dy < -1.0 || dy > 1.0) continue;
        double dx = sqrt(1.0 - dy * dy) * rx;

        int xs = (int)(cx - dx + 0.5);
        int xe = (int)(cx + dx + 0.5);

        int inner_xs = 9999;
        int inner_xe = -9999;

        if (!filled && inner_rx > 0.0 && inner_ry > 0.0) {
            double inner_dy = (py + 0.5 - cy) / inner_ry;
            if (inner_dy >= -1.0 && inner_dy <= 1.0) {
                double inner_dx = sqrt(1.0 - inner_dy * inner_dy) * inner_rx;
                inner_xs = (int)(cx - inner_dx + 0.5);
                inner_xe = (int)(cx + inner_dx + 0.5);
            }
        }

        for (int px = xs; px <= xe; px++) {
            if (filled) {
                bricklua_brick(px, py);
            } else {
                if (px < inner_xs || px > inner_xe) {
                    bricklua_brick(px, py);
                }
            }
        }
    }
}


/* ---------- print ---------- */
void bricklua_print(const char *s, int x, int y) {
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
                if ((bits >> (FONT_CELL_W - 1 - col)) & 1) {
                    bricklua_pixel(cx + col, cy + row);
                }
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
#define BTN_REPEAT_RATE  4  // Скорость движения после паузы

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
                // Белый шум (как на десктопе)
                v = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;   
            } else {
                // ТРЕУГОЛЬНАЯ ВОЛНА (Triangle Wave)
                // g_tone.phase уже в диапазоне от 0.0 до 1.0
                v = 4.0f * fabsf(g_tone.phase - 0.5f) - 1.0f; 
                
                // Двигаем фазу (0.0 .. 1.0)
                g_tone.phase += (float)g_tone.freq / (float)BRICKLUA_SAMPLE_RATE;
                if (g_tone.phase >= 1.0f) {
                    g_tone.phase -= 1.0f;
                }
            }
            
            // Громкость (100.0f = ~30% от максимума)
            s = (int16_t)(v * g_tone.vol * 100.0f);
            
            g_tone.remaining--;
        }

        out[i * 2]     = s;   /* L */
        out[i * 2 + 1] = s;   /* R */
    }
}