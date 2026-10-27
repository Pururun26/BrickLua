# BrickLua

A tiny fantasy console for Lua in the style of the classic Brick Game. 
240×160 resolution (20×20 grid of 8×8 bricks + text panel), 2 colors, 64 KB per cartridge.

- One `.brick` file — the entire game (code + data)
- Lua 5.4.9, without `io`/`os`/`package`/`debug`
- 2-color palette (bg `#9ead86`, fg `#1a1a1a`)
- **20×20 grid of 8×8 bricks** (left side) + dedicated text panel (right side)
- Single-channel sound (square wave)

## Build

```bash
cmake -S . -B build-native
cmake --build build-native
```

Dependencies: CMake, MSYS2 UCRT64 gcc, `raylib-6.0_win64_mingw-w64`, `lua-5.4.9`.

## Run

```bash
./run.sh examples/snake.brick
```

`run.sh` packs the `.brick` via `pack.py` and launches `build-native/bricklua.exe`.

## Web

```bash
source /c/emsdk/emsdk_env.sh
emcmake cmake -S . -B build-web
cmake --build build-web
```

Deploy 4 files together: `index.html`, `index.js`, `index.wasm`, `cart.brick`. Test locally over HTTP (not `file://`):

```bash
cd build-web && python -m http.server 8000
```

## Cartridge

A `.brick` file is a regular Lua script with a `.brick` extension. Limit — **65536 characters** (code combined).

```lua
--! dfc 1

function update()
    cls(0)
    -- your game
end
```

Top-level code runs once at load. `update()` is called 30 times per second.

## Modules

Multi-file project: `main.brick` + `.lua` modules alongside.

```lua
-- main.brick
--[[utils.lua]]
--[[player.lua]]

function update()
    player_update()
end
```

`pack.py` bundles everything into a single file:
```bash
python tools/pack.py examples/my_game/main.brick -o build-native/run.brick
```

## Controls

| i | Button | Keys |
|---|---|---|
| 0 | LEFT | ← / A |
| 1 | RIGHT | → / D |
| 2 | UP | ↑ / W |
| 3 | DOWN | ↓ / S |
| 4 | A | Z / J |
| 5 | B | X / K |

F11 — fullscreen. Ctrl+P — FPS overlay.

## API

### Constants

| | |
|---|---|
| `20` | grid size (20×20 bricks) |
| `30` | FPS |
| color `0` | background `#9ead86` |
| color `1` | foreground `#1a1a1a` |

### Graphics

The screen is divided into two zones: the **Game Grid** (left, 20×20 cells) and the **Text Panel** (right, 80 pixels wide). 

```lua
cls(color)                -- fill the ENTIRE screen with color (0 or 1)
pset(x, y, color)         -- place an 8×8 brick at grid coordinates (0..19)
pget(x, y)                -- read brick at grid coordinates, returns 0 or 1
print(s, x, y, color)     -- text, 3×4 font (x offset starts at 160px)
```

- **`pset` / `pget`**: Works strictly on the 20×20 grid. `(0,0)` is top-left, `(19,19)` is bottom-right. Out-of-bounds coordinates are safely ignored.
- **`print`**: Text is automatically shifted to the right panel. `x=0` means the left edge of the text panel. `y` is in pixels. Uppercase only.
- **`cls`**: Clears everything (both the grid and the text panel).

### Input

```lua
btn(i)   -- true on press, and auto-repeats if held (0..5)
```

- **DAS (Delayed Auto Shift)**: Returns `true` immediately on the first frame of press. If held, it repeats every 5 frames after a 10-frame delay. Perfect for both menu navigation and grid movement. There is no `btnp`.

### Sound

```lua
beep(freq, dur, vol)      -- tone at freq Hz, dur frames, vol 0..100
```

- Single channel (monophonic).
- `dur` in frames: `15` = 0.5 s, `30` = 1 s (at 30 FPS).
- `beep(0, 0, 0)` — silence.
- A new `beep` interrupts the current one.

### Time

```lua
time()    -- milliseconds since game start
utime()   -- Unix timestamp in seconds
```

`time()` for animation, timers, cooldowns. `utime()` for `math.randomseed(utime())` and persistent games.

### Save

```lua
dset(slot, value)     -- write int64 to slot (0..63)
dget(slot)            -- read int64 from slot (0..63)
```

- **64 slots × 8 bytes = 512 bytes** per cartridge.
- `dset` writes immediately (crash-safe).
- Desktop: `save/<cartname>.bin` next to the exe.
- Web: `localStorage['storage']` (persists across reloads).

### Standard Lua

`math`, `string`, `table`, `coroutine` are available. **No** `io`, `os`, `package`, `debug`, `loadfile`, `dofile`. `load` (for strings) is available.

## Example: mini-game

```lua
math.randomseed(utime())

px, py = 10, 10
food_x, food_y = math.random(0, 19), math.random(0, 19)
score = 0

function update()
    -- Move player (using auto-repeat)
    if btn(0) then px = px - 1 end
    if btn(1) then px = px + 1 end
    if btn(2) then py = py - 1 end
    if btn(3) then py = py + 1 end

    -- Boundaries
    if px < 0 then px = 0 end
    if px > 19 then px = 19 end
    if py < 0 then py = 0 end
    if py > 19 then py = 19 end

    -- Collision
    if px == food_x and py == food_y then
        food_x = math.random(0, 19)
        food_y = math.random(0, 19)
        score = score + 1
        beep(880, 5)
    end

    -- Render
    cls(0)
    pset(px, py, 1)           -- Player
    pset(food_x, food_y, 1)   -- Food
    
    -- UI (Right Panel)
    print("SCORE", 2, 10, 1)
    print(tostring(score), 2, 16, 1)
end
```

## Limitations

- 240×160 resolution (20×20 grid + 80px text panel)
- 2 colors strictly
- 65536 characters per cartridge
- No sprites, no lines, no rectangles — only the 8×8 brick
- 64 save slots × 8 bytes
- No `require` — modules via `--[[file.lua]]`
- No `io`/`os`/`package`/`debug` — sandboxed
- No network, no file I/O (except cartridge loading)

## Project layout

```
src/
  bricklua.c             — main, window, loop, log
  bricklua_conf.h        — constants (WIDTH, FPS, colors)
  bricklua_api.c/.h      — C implementation of the API
  bricklua_api_lua.c/.h  — Lua bindings
  bricklua_log.c/.h      — logging (file on desktop, console.log on web)
  bricklua_save.c/.h     — dset/dget (file on desktop, localStorage on web)
  bricklua_font3x4.h     — font
web/
  shell.html           — web shell (fetch cart.brick, canvas CSS)
tools/
  pack.py              — builds .brick from main + modules
examples/
  snake.brick           — snake
  ...                  — other examples
```

## Build outputs

- `build-native/bricklua.exe` — desktop binary
- `build-native/bricklua_log.txt` — log of the last run
- `build-native/save/<name>.bin` — save data
- `build-web/index.html` + `.js` + `.wasm` — web build

## License

MIT — see [LICENSE](LICENSE).
Bundled: Lua 5.4.9 (MIT), raylib 6.0 (zlib) — full texts in `--license`.