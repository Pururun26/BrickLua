#include "bricklua_api_lua.h"
#include "bricklua_api.h"
#include "bricklua_conf.h"
#include "bricklua_save.h"

// Объявляем новые функции из bricklua_api.c
static int l_cls(lua_State *L) {
    int c = (int)luaL_optinteger(L, 1, 0);
    // Вызываем очистку только игрового поля (левой части)
    bricklua_cls(c);
    return 0;
}

static int l_pset(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int c = (int)luaL_optinteger(L, 3, 1);
    // Вызываем отрисовку кубика вместо пикселя
    bricklua_brick(x, y, c);
    return 0;
}

static int l_pget(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    // Читаем состояние кубика по его центру (4,4 внутри блока 8x8)
    lua_pushinteger(L, bricklua_pget(x * 8 + 4, y * 8 + 4));
    return 1;
}

// Текст ТОЛЬКО в правой панели
static int l_print(lua_State *L) {
    const char *s = luaL_checkstring(L, 1);
    int x = (int)luaL_optinteger(L, 2, 0);
    int y = (int)luaL_optinteger(L, 3, 0);
    int c = (int)luaL_optinteger(L, 4, 1);

    // Ограничиваем координаты, чтобы текст не вылезал за рамки
    // Слева и сверху: не уходим в минус (на сетку или на верхнюю рамку)
    if (x < 0) x = 0; 
    if (y < 0) y = 0;

    // Снизу: рамка на 159. Высота шрифта 4.
    // Максимальный y = 154 (154 + 4 = 158, оставляем 1 пиксель до рамки)
    if (y > 154) y = 154;
    
    // Сдвигаем на 160 пикселей вправо (начало панели UI)
    // + 1 пиксель по Y, чтобы текст не прилипал к верхней рамке
    bricklua_print(s, 160 + x, 1 + y, c);
    return 0;
}

static int l_time(lua_State *L) {
    lua_pushinteger(L, (lua_Integer)bricklua_time());
    return 1;
}

static int l_utime(lua_State *L) {
    lua_pushinteger(L, (lua_Integer)bricklua_utime());
    return 1;
}

static int l_btn(lua_State *L) {
    int i = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, bricklua_btn(i));
    return 1;
}

static int l_beep(lua_State *L) {
    int freq = (int)luaL_checkinteger(L, 1);
    int dur  = (int)luaL_checkinteger(L, 2);
    int vol  = (int)luaL_optinteger(L, 3, 100);
    bricklua_beep(freq, dur, vol);
    return 0;
}

static int l_dset(lua_State *L) {
    int slot = (int)luaL_checkinteger(L, 1);
    lua_Integer value = luaL_checkinteger(L, 2);
    bricklua_dset(slot, (int64_t)value);
    return 0;
}

static int l_dget(lua_State *L) {
    int slot = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, (lua_Integer)bricklua_dget(slot));
    return 1;
}


static const luaL_Reg api_funcs[] = {
    {"cls",     l_cls},
    {"pset",    l_pset},
    {"pget",    l_pget},
    {"print",   l_print},
    {"time",    l_time},
    {"utime",   l_utime},
    {"btn",     l_btn},
    {"beep",    l_beep},
    {"dset",    l_dset},
    {"dget",    l_dget},
    {NULL, NULL}
};

void bricklua_register_api(lua_State *L) {
    lua_pushglobaltable(L);          /* _G */
    luaL_setfuncs(L, api_funcs, 0);  /* adds cls/pset/pget into _G */
    lua_pop(L, 1);
}