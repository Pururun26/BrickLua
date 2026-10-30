#pragma once
#include <stddef.h>
#include <stdint.h>

/* 32 слота по 8 байт (int64_t) = 256 байт.
 * Файл: <application_dir>/save/<cartname>.bin */

void bricklua_save_open(const char *cart_path);   /* создать save/, загрузить данные */
void bricklua_save_close(void);                   /* финальный flush */

void    bricklua_dset(int slot, int64_t value);
int64_t bricklua_dget(int slot);