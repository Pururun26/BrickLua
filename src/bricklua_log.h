#pragma once

/* Открыть лог рядом с exe. Вызывать один раз в начале main,
 * ДО InitWindow() — чтобы перехватить стартовые логи raylib. */
void bricklua_log_init(void);

/* Закрыть лог. Вызывать перед выходом (или через atexit). */
void bricklua_log_close(void);

/* Свои логи (аналог printf, но в файл). */
void bricklua_log(const char *format, ...);