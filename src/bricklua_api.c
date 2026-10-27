#include "bricklua_api.h"
#include "bricklua_font3x4.h"

#include <stdlib.h>   /* abs */
#include <math.h>     /* sqrt */
#include <string.h>   /* memset */
#include <time.h>
#include <stdint.h>

unsigned char g_fb[BRICKLUA_WIDTH * BRICKLUA_HEIGHT];
Color         g_palette[BRICKLUA_COLOR_COUNT];

static Color   g_pixels[BRICKLUA_WIDTH * BRICKLUA_HEIGHT];
static Texture2D g_tex;

void bricklua_api_init(void) {
    bricklua_time_reset();               /* <-- сброс g_start_ms */
    g_palette[0] = GetColor(BRICKLUA_DEFAULT_COLOR_0);
    g_palette[1] = GetColor(BRICKLUA_DEFAULT_COLOR_1);

    Image img = {
        .data    = g_pixels,
        .width   = BRICKLUA_WIDTH,
        .height  = BRICKLUA_HEIGHT,
        .mipmaps = 1,
        .format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };
    g_tex = LoadTextureFromImage(img);
    SetTextureFilter(g_tex, TEXTURE_FILTER_POINT);
}

void bricklua_api_shutdown(void) {
    UnloadTexture(g_tex);
}

void bricklua_present(void) {
    // 1. Формируем массив пикселей с рамкой
    for (int i = 0; i < BRICKLUA_WIDTH * BRICKLUA_HEIGHT; i++) {
        int x = i % BRICKLUA_WIDTH;
        int y = i / BRICKLUA_WIDTH;

        // Условия для рамки (цвет переднего плана, g_palette[1]):
        // - Левая стена (x == 0)
        // - Правая стена всего экрана (x == 239)
        // - Верхняя стена (y == 0)
        // - Нижняя стена (y == 159)
        // - Разделитель между игрой и текстом (x == 159)
        if (x == 0 || x == 239 || y == 0 || y == 159 || x == 159) {
            g_pixels[i] = g_palette[1];
        } else {
            // Внутри рамок выводим буфер игры
            g_pixels[i] = g_palette[g_fb[i] & (BRICKLUA_COLOR_COUNT - 1)];
        }
    }
    
    // 2. Обновляем текстуру
    UpdateTexture(g_tex, g_pixels);

    // 3. Вычисляем масштаб и позицию для отрисовки
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int sx = sw / BRICKLUA_WIDTH;
    int sy = sh / BRICKLUA_HEIGHT;
    int s  = (sx < sy) ? sx : sy;
    if (s < 1) s = 1;

    int dw = BRICKLUA_WIDTH  * s;
    int dh = BRICKLUA_HEIGHT * s;
    int ox = (sw - dw) / 2;
    int oy = (sh - dh) / 2;

    // 4. Рисуем на экран
    DrawTexturePro(g_tex,
        (Rectangle){ 0, 0, (float)BRICKLUA_WIDTH, (float)BRICKLUA_HEIGHT },
        (Rectangle){ (float)ox, (float)oy, (float)dw, (float)dh },
        (Vector2){ 0, 0 }, 0.0f, WHITE);
}


static int g_fps_value  = 0;
static int g_fps_frames = 0;
static long long g_fps_last = 0;

void bricklua_fps_frame(void) {
    g_fps_frames++;
    long long now = bricklua_utime();
    if (g_fps_last == 0) g_fps_last = now;
    if (now - g_fps_last >= 1) {
        g_fps_value  = g_fps_frames;
        g_fps_frames = 0;
        g_fps_last   = now;
    }
}

int bricklua_fps_get(void) {
    return g_fps_value;
}

void bricklua_cls(int color) {
    unsigned char c = (unsigned char)(color & (BRICKLUA_COLOR_COUNT - 1));
    for (int i = 0; i < BRICKLUA_WIDTH * BRICKLUA_HEIGHT; i++)
        g_fb[i] = c;
}

void bricklua_pset(int x, int y, int color) {
    if (x < 0 || x >= BRICKLUA_WIDTH || y < 0 || y >= BRICKLUA_HEIGHT) return;
    g_fb[y * BRICKLUA_WIDTH + x] = (unsigned char)(color & (BRICKLUA_COLOR_COUNT - 1));
}

int bricklua_pget(int x, int y) {
    if (x < 0 || x >= BRICKLUA_WIDTH || y < 0 || y >= BRICKLUA_HEIGHT) return 0;
    return (int)g_fb[y * BRICKLUA_WIDTH + x];
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


/* ---------- BRICK (7x7 block, 1px gap, no offsets) ---------- */
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
    // Теперь сетка 20x20 (левая часть 160x160)
    if (gx < 0 || gx >= 20 || gy < 0 || gy >= 20) return;
    
    int px = gx * 8;
    int py = gy * 8;
    
    if (color == 0) {
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 7; x++) {
                bricklua_pset(px + x, py + y, 0);
            }
        }
    } else {
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 7; x++) {
                if (CUBE_PATTERN[y][x] == '1') {
                    bricklua_pset(px + x, py + y, color);
                }
            }
        }
    }
}

// Очистка ТОЛЬКО левой части (игрового поля)
void bricklua_cls_grid(int color) {
    unsigned char c = (unsigned char)(color & (BRICKLUA_COLOR_COUNT - 1));
    for (int y = 0; y < 96; y++) {
        for (int x = 0; x < 96; x++) {
            g_fb[y * BRICKLUA_WIDTH + x] = c;
        }
    }
}


/* ---------- BRICK SPRITES (4x4) ---------- */
#define MAX_BRICK_SPRITES 64
#define BRICK_SPRITE_SIZE 4

static uint8_t g_brick_sprites[MAX_BRICK_SPRITES][BRICK_SPRITE_SIZE][BRICK_SPRITE_SIZE];

void bricklua_sprdata(int id, const char *art) {
    if (id < 0 || id >= MAX_BRICK_SPRITES || !art) return;

    // Очищаем спрайт перед загрузкой (0 = пустота)
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
                bricklua_brick(gx + x, gy + y, 1);
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
        bricklua_brick(x0, y0, 1);
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
                bricklua_brick(xx, yy, 1);
            }
        }
    } else {
        for (int xx = x; xx <= x_end; xx++) {
            bricklua_brick(xx, y, 1);     // Верх
            bricklua_brick(xx, y_end, 1); // Низ
        }
        for (int yy = y + 1; yy < y_end; yy++) {
            bricklua_brick(x, yy, 1);     // Лево
            bricklua_brick(x_end, yy, 1); // Право
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

    // Радиус внутреннего круга (для создания пустоты)
    double inner_rx = rx - 1.0;
    double inner_ry = ry - 1.0;

    for (int py = y; py < y + h; py++) {
        double dy = (py + 0.5 - cy) / ry;
        if (dy < -1.0 || dy > 1.0) continue;
        double dx = sqrt(1.0 - dy * dy) * rx;

        int xs = (int)(cx - dx + 0.5);
        int xe = (int)(cx + dx + 0.5);

        // По умолчанию считаем, что внутренней дырки на этой строке нет
        int inner_xs = 9999;
        int inner_xe = -9999;

        if (!filled && inner_rx > 0.0 && inner_ry > 0.0) {
            double inner_dy = (py + 0.5 - cy) / inner_ry;
            // Если мы внутри внутреннего круга по вертикали
            if (inner_dy >= -1.0 && inner_dy <= 1.0) {
                double inner_dx = sqrt(1.0 - inner_dy * inner_dy) * inner_rx;
                inner_xs = (int)(cx - inner_dx + 0.5);
                inner_xe = (int)(cx + inner_dx + 0.5);
            }
        }

        // Рисуем строку
        for (int px = xs; px <= xe; px++) {
            if (filled) {
                bricklua_brick(px, py, 1);
            } else {
                // Рисуем только если пиксель НЕ попал внутрь дырки
                if (px < inner_xs || px > inner_xe) {
                    bricklua_brick(px, py, 1);
                }
            }
        }
    }
}


/* ---------- buttons ---------- */
static int prev_btn = 0;
static int curr_btn = 0;

static int host_read_buttons(void) {
    int b = 0;
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) b |= 1 << 0;   /* LEFT  */
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) b |= 1 << 1;   /* RIGHT */
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) b |= 1 << 2;   /* UP    */
    if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) b |= 1 << 3;   /* DOWN  */
    if (IsKeyDown(KEY_Z)     || IsKeyDown(KEY_J)) b |= 1 << 4;   /* A     */
    if (IsKeyDown(KEY_X)     || IsKeyDown(KEY_K)) b |= 1 << 5;   /* B     */
    return b;
}

void bricklua_poll_buttons(void) {
    prev_btn = curr_btn;
    curr_btn = host_read_buttons();
}

// Настройки автоповтора (DAS - Delayed Auto Shift)
#define BTN_REPEAT_DELAY 10 // Пауза перед началом движения (в кадрах)
#define BTN_REPEAT_RATE  5  // Скорость движения после паузы

static int btn_timer[6] = {0, 0, 0, 0, 0, 0};
static int btn_held[6]  = {0, 0, 0, 0, 0, 0};

// НОВАЯ bricklua_btn: Умный автоповтор
int bricklua_btn(int i) {
    if (i < 0 || i >= 6) return 0;

    int is_down = (curr_btn & (1 << i)) ? 1 : 0;
    int was_down = (prev_btn & (1 << i)) ? 1 : 0;

    if (!is_down) {
        btn_held[i] = 0;
        btn_timer[i] = 0;
        return 0;
    }

    if (!was_down) {
        btn_held[i] = 1;
        btn_timer[i] = 0;
        return 1;
    }

    if (btn_held[i]) {
        btn_timer[i]++;
        if (btn_timer[i] >= BTN_REPEAT_DELAY) {
            if ((btn_timer[i] - BTN_REPEAT_DELAY) % BTN_REPEAT_RATE == 0) {
                return 1;
            }
        }
    }
    return 0;
}


/* ---------- beep ---------- */
#define BEEP_SAMPLE_RATE 44100

static Sound g_beep_sound;
static int   g_beep_used = 0;

static void beep_gen_square(short *s, int n, float freq, int vol) {
    for (int i = 0; i < n; i++) {
        float t     = (float)i / BEEP_SAMPLE_RATE;
        float phase = fmodf(t * freq, 1.0f);
        float v     = (phase < 0.5f) ? 1.0f : -1.0f;   /* 50% duty — квадрат */
        s[i] = (short)(v * vol * 327.67f);
    }
}

static void beep_gen_noise(short *s, int n, int vol) {
    for (int i = 0; i < n; i++) {
        float v = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;   /* -1..+1 */
        s[i] = (short)(v * vol * 327.67f);
    }
}

static void beep_stop(void) {
    if (g_beep_used) {
        StopSound(g_beep_sound);
        UnloadSound(g_beep_sound);
        g_beep_used = 0;
    }
}

void bricklua_beep(int freq, int dur, int vol) {
    if (!IsAudioDeviceReady()) return;

    /* тишина: dur=0 или vol=0. freq=0 — это шум, не тишина */
    if (dur <= 0 || vol <= 0) {
        beep_stop();
        return;
    }

    /* клампим, чтобы не словить переполнение/огромный malloc */
    if (freq <  0)    freq = 0;
    if (freq >  8000) freq = 8000;
    if (dur  >   255) dur  = 255;
    if (vol  >   100) vol  = 100;

    float duration_sec = (float)dur / (float)BRICKLUA_FPS;
    int   n = (int)(BEEP_SAMPLE_RATE * duration_sec);
    if (n <= 0) return;

    short *samples = (short *)malloc(n * sizeof(short));
    if (!samples) return;

    if (freq == 0) beep_gen_noise  (samples, n, vol);              /* шум */
    else           beep_gen_square (samples, n, (float)freq, vol); /* квадрат */

    Wave wave = {
        .frameCount = (unsigned int)n,
        .sampleRate = BEEP_SAMPLE_RATE,
        .sampleSize = 16,
        .channels   = 1,
        .data       = samples,
    };
    Sound snd = LoadSoundFromWave(wave);
    free(samples);
    if (snd.frameCount == 0) return;

    beep_stop();      /* вытесняем предыдущий */

    g_beep_sound = snd;
    g_beep_used  = 1;
    PlaySound(snd);
}