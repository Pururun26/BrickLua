#include "bricklua_conf.h"
#include "bricklua_api.h"
#include "bricklua_api_lua.h"
#include "bricklua_log.h"
#include "bricklua_save.h"
#include "bricklua_console.h"
#include "bricklua_license.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "raylib.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* Дублирует сообщение в stderr и в bricklua_log.txt. */
static void bricklua_err(const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    fprintf(stderr, "%s", buf);
    bricklua_log("%s", buf);
}

static void bricklua_openlibs(lua_State *L) {
    luaL_requiref(L, LUA_GNAME,        luaopen_base,      1); lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME,  luaopen_math,      1); lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME,   luaopen_string,    1); lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME,   luaopen_table,     1); lua_pop(L, 1);

    lua_pushnil(L); lua_setglobal(L, "loadfile");
    lua_pushnil(L); lua_setglobal(L, "dofile");
}

static int call_update(lua_State *L) {
    lua_getglobal(L, "BRICK");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        bricklua_err("bricklua: update error: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return 1;
    }
    return 0;
}

static long file_size(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return sz;
}

static int has_brick_ext(const char *path) {
    size_t n = strlen(path);
    return n >= 6 && strcmp(path + n - 6, ".brick") == 0;
}

/* --- Состояние для главного цикла (нужно emscripten_set_main_loop) --- */
static lua_State *g_L        = NULL;
static int        g_show_fps = 0;
static int        g_running  = 1;

static void frame(void) {
    if (WindowShouldClose()) {
        g_running = 0;
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        return;
    }

#ifndef __EMSCRIPTEN__
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();
#endif

    if (IsKeyPressed(KEY_P) && IsKeyDown(KEY_LEFT_CONTROL))
        g_show_fps = !g_show_fps;

    bricklua_poll_buttons();

    if (call_update(g_L)) {
        g_running = 0;
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        return;
    }

    if (g_show_fps) {
        char buf[16];
        snprintf(buf, sizeof(buf), "FPS:%d", bricklua_fps_get());
        int len = (int)strlen(buf);
        int w   = len * 4;
        bricklua_print(buf, BRICKLUA_WIDTH - w + 1, 1);
    }

    BeginDrawing();
        ClearBackground(BLACK);
        bricklua_present();
    EndDrawing();

    bricklua_fps_frame();
}

int main(int argc, char **argv) {
    // CLI-флаги (до InitWindow, до Lua, до поиска картриджа)
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--license") == 0) {
            bricklua_console_attach();
            printf("\n%s", BRICKLUA_LICENSE_TEXT);
            fflush(stdout);
            bricklua_console_detach();
            return 0;
        }

        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            bricklua_console_attach();
            printf("\nBrickLua " BRICKLUA_VERSION " - a tiny fantasy console for Lua\n\n");
            printf("Usage:\n");
            printf("  bricklua.exe                  run cart.brick next to the exe\n");
            printf("  bricklua.exe mygame.brick     run a specific cartridge\n");
            printf("  bricklua.exe --license        show licenses\n");
            printf("  bricklua.exe --help           show this help\n");
            printf("  bricklua.exe --version        show version\n");
            fflush(stdout);
            bricklua_console_detach();
            return 0;
        }

        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            bricklua_console_attach();
            printf("BrickLua v%s\n", BRICKLUA_VERSION);
            fflush(stdout);
            bricklua_console_detach();
            return 0;
        }
    }

    const char *cart = NULL;
    char cart_buf[1024];

    if (argc >= 2) {
        cart = argv[1];
    } else {
        /* Нет аргумента — ищем cart.brick рядом с exe */
        snprintf(cart_buf, sizeof(cart_buf), "%scart.brick", GetApplicationDirectory());
        if (file_size(cart_buf) < 0) {
            fprintf(stderr, "usage: %s <cartridge.brick>\n", argv[0]);
            return 1;
        }
        cart = cart_buf;
    }

    if (!has_brick_ext(cart)) {
        fprintf(stderr, "bricklua: expected .brick cartridge, got: %s\n", cart);
        return 1;
    }

    bricklua_log_init();
    bricklua_log("starting: %s", cart);

    long sz = file_size(cart);
    if (sz < 0) {
        bricklua_err("bricklua: cannot read '%s'\n", cart);
        bricklua_log_close();
        return 1;
    }
    bricklua_log("[INFO] BrickLua: Script size %ld / %d chars", sz, BRICKLUA_MAX_SCRIPT_SIZE);
    if (sz > BRICKLUA_MAX_SCRIPT_SIZE) {
        bricklua_err("bricklua: script too large: %ld / %d\n", sz, BRICKLUA_MAX_SCRIPT_SIZE);
        bricklua_log_close();
        return 1;
    }

    bricklua_save_open(cart);

    g_L = luaL_newstate();
    if (!g_L) {
        bricklua_err("bricklua: failed to create lua state\n");
        bricklua_log_close();
        return 1;
    }

    bricklua_openlibs(g_L);
    bricklua_register_api(g_L);

    if (luaL_loadfile(g_L, cart) != LUA_OK) {
        bricklua_err("bricklua: load error: %s\n", lua_tostring(g_L, -1));
        lua_close(g_L);
        bricklua_log_close();
        return 1;
    }
    if (lua_pcall(g_L, 0, 0, 0) != LUA_OK) {
        bricklua_err("bricklua: runtime error: %s\n", lua_tostring(g_L, -1));
        lua_close(g_L);
        bricklua_log_close();
        return 1;
    }

    InitWindow(BRICKLUA_WINDOW_WIDTH, BRICKLUA_WINDOW_HEIGHT, BRICKLUA_TITLE);
    InitAudioDevice();
    HideCursor();

#ifndef __EMSCRIPTEN__
    SetWindowState(FLAG_WINDOW_RESIZABLE);
#endif
    SetTargetFPS(BRICKLUA_FPS);

    bricklua_api_init();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (g_running) frame();

    bricklua_api_shutdown();
    CloseAudioDevice();
    CloseWindow();
    bricklua_save_close();
    lua_close(g_L);
    bricklua_log_close();
#endif
    return 0;
}