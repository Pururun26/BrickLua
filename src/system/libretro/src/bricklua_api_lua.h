#pragma once
#include "lua.h"

/* Открыть безопасный subset (base/math/string/table/utf8), без io/os/package/debug. */
void bricklua_openlibs(lua_State *L);

/* Зарегистрировать API (cls/bset/bget) в _G. */
void bricklua_register_api(lua_State *L);