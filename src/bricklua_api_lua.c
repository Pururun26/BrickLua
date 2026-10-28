#include "bricklua_api_lua.h"
#include "bricklua_api.h"
#include "bricklua_conf.h"
#include "bricklua_save.h"

static int l_cls(lua_State *L) {
    // Всегда очищаем в 0 (фон)
    bricklua_cls(0);
    return 0;
}

static int l_bset(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    // Всегда ставит кирпич (1). Никаких флагов.
    bricklua_brick(x, y, 1); 
    return 0;
}

static int l_bget(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    // Возвращает true, если там есть кирпич, иначе false
    lua_pushboolean(L, bricklua_pget(x, y) != 0); 
    return 1;
}

// Текст ТОЛЬКО в правой панели
// Текст ТОЛЬКО в правой панели
static int l_print(lua_State *L) {
    const char *s = luaL_checkstring(L, 1);
    int x = (int)luaL_optinteger(L, 2, 0);
    int y = (int)luaL_optinteger(L, 3, 0);

    // Ограничения для текста (чтобы не вылезал за рамки)
    if (x < 0) x = 0; 
    if (y < 0) y = 0;

    // Снизу: рамка на 159. Высота шрифта 4.
    // Максимальный y = 154 (154 + 4 = 158, оставляем 1 пиксель до рамки)
    if (y > 154) y = 154;
    
    // Сдвигаем на 160 пикселей вправо (начало панели UI)
    // Текст всегда рисуется цветом 1
    bricklua_print(s, 160 + x, y, 1);
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

static int l_line(lua_State *L) {
    bricklua_line(
        (int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2),
        (int)luaL_checkinteger(L, 3), (int)luaL_checkinteger(L, 4)
    );
    return 0;
}

static int l_rect(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int w = (int)luaL_checkinteger(L, 3);
    int h = (int)luaL_checkinteger(L, 4);
    // Если 5-й аргумент не передан (nil) или false — вернет 0, если true — вернет 1
    int filled = lua_toboolean(L, 5); 
    
    bricklua_rect(x, y, w, h, filled);
    return 0;
}

static int l_oval(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int w = (int)luaL_checkinteger(L, 3);
    int h = (int)luaL_checkinteger(L, 4);
    int filled = lua_toboolean(L, 5);
    
    bricklua_oval(x, y, w, h, filled);
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
    {"bset",    l_bset},
    {"bget",    l_bget},
    {"print",   l_print},
    {"sprdata", l_sprdata},
    {"spr",     l_spr},
    {"line",    l_line},
    {"rect",    l_rect},
    {"oval",    l_oval},
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
    luaL_setfuncs(L, api_funcs, 0);  /* adds cls/bset/bget into _G */
    lua_pop(L, 1);
}