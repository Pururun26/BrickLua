#include "bricklua_api_lua.h"
#include "bricklua_api.h"
#include "lauxlib.h"
#include "lualib.h"

static int l_cls(lua_State *L) {
    bricklua_cls((int)luaL_optinteger(L, 1, 0));
    return 0;
}

static int l_pset(lua_State *L) {
    bricklua_brick((int)luaL_checkinteger(L, 1),
                 (int)luaL_checkinteger(L, 2),
                 (int)luaL_optinteger  (L, 3, 1));
    return 0;
}

static int l_pget(lua_State *L) {
    lua_pushinteger(L, bricklua_pget(
        (int)luaL_checkinteger(L, 1),
        (int)luaL_checkinteger(L, 2)));
    return 1;
}

static int l_line(lua_State *L) {
    bricklua_line(
        (int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2),
        (int)luaL_checkinteger(L, 3), (int)luaL_checkinteger(L, 4)
    );
    return 0;
}

static int l_rect(lua_State *L) {
    bricklua_rect(
        (int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2),
        (int)luaL_checkinteger(L, 3), (int)luaL_checkinteger(L, 4),
        lua_toboolean(L, 5) // true / false
    );
    return 0;
}

static int l_oval(lua_State *L) {
    bricklua_oval(
        (int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2),
        (int)luaL_checkinteger(L, 3), (int)luaL_checkinteger(L, 4),
        lua_toboolean(L, 5) // true / false
    );
    return 0;
}

static int l_sprdata(lua_State *L) {
    int id = (int)luaL_checkinteger(L, 1);
    const char *art = luaL_checkstring(L, 2);
    bricklua_sprdata(id, art);
    return 0;
}

static int l_spr(lua_State *L) {
    int id = (int)luaL_checkinteger(L, 1);
    int gx = (int)luaL_checkinteger(L, 2);
    int gy = (int)luaL_checkinteger(L, 3);
    bricklua_spr(id, gx, gy);
    return 0;
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


static int l_btn(lua_State *L) {
    int i = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, bricklua_btn(i));
    return 1;
}


static int l_time(lua_State *L) {
    lua_pushinteger(L, (lua_Integer)bricklua_time());
    return 1;
}

static int l_utime(lua_State *L) {
    lua_pushinteger(L, (lua_Integer)bricklua_utime());
    return 1;
}


static int l_dset(lua_State *L) {
    int slot = (int)luaL_checkinteger(L, 1);
    lua_Integer v = luaL_checkinteger(L, 2);
    bricklua_dset(slot, (int64_t)v);
    return 0;
}

static int l_dget(lua_State *L) {
    int slot = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, (lua_Integer)bricklua_dget(slot));
    return 1;
}


static int l_beep(lua_State *L) {
    int freq = (int)luaL_checkinteger(L, 1);
    int dur  = (int)luaL_checkinteger(L, 2);
    int vol  = (int)luaL_optinteger(L, 3, 100);
    bricklua_beep(freq, dur, vol);
    return 0;
}

static const luaL_Reg api_funcs[] = {
    {"cls",     l_cls},
    {"pset",    l_pset},
    {"pget",    l_pget},
    {"line",    l_line},
    {"rect",    l_rect},
    {"oval",    l_oval},
    {"sprdata", l_sprdata},
    {"spr",     l_spr},
    {"print",   l_print},
    {"btn",     l_btn},
    {"time",    l_time},
    {"utime",   l_utime},
    {"dset",    l_dset},
    {"dget",    l_dget},
    {"beep",    l_beep},
    {NULL, NULL}
};

void bricklua_openlibs(lua_State *L) {
    luaL_requiref(L, LUA_GNAME,        luaopen_base,   1); lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME,  luaopen_math,   1); lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME,   luaopen_string, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME,   luaopen_table,  1); lua_pop(L, 1);

    lua_pushnil(L); lua_setglobal(L, "loadfile");
    lua_pushnil(L); lua_setglobal(L, "dofile");
}

void bricklua_register_api(lua_State *L) {
    lua_pushglobaltable(L);
    luaL_setfuncs(L, api_funcs, 0);
    lua_pop(L, 1);
}