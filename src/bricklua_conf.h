#pragma once

#define BRICKLUA_WINDOW_WIDTH  640
#define BRICKLUA_WINDOW_HEIGHT 640

// #define BRICKLUA_WINDOW_WIDTH 576
// #define BRICKLUA_WINDOW_HEIGHT 384

/* --- Виртуальный экран (framebuffer) --- */
// #define BRICKLUA_WIDTH   144
// #define BRICKLUA_HEIGHT  96
#define BRICKLUA_WIDTH 160
#define BRICKLUA_HEIGHT 160

#define BRICKLUA_TITLE   "BrickLua"
#define BRICKLUA_FPS     30

/* Палитра */
#define BRICKLUA_COLOR_COUNT       2
#define BRICKLUA_DEFAULT_COLOR_0   0x9ead86FF   /* фон #9ead86 */
#define BRICKLUA_DEFAULT_COLOR_1   0x1A1A1AFF   /* передний план #1a1a1a */

/* Третий цвет: только для ядра (блеклые кирпичики) */
#define BRICKLUA_COLOR_GHOST     0x879372FF // Темно-зеленоватый, чуть темнее фона

/* Максимальный размер картриджа в байтах. Больше — не запускаемся. */
#define BRICKLUA_MAX_SCRIPT_SIZE 32768

// Количество спрайтов
#define MAX_BRICK_SPRITES 32

#define BRICKLUA_VERSION "2.0.0"