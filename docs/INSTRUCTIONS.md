# Instructions

## Use it

Open the site: <https://amalmehta.github.io/GraphicsLibrary/>. You need a browser with WebGL2 (current Chrome, Edge, Firefox or Safari 15+).

| Do | How |
|---|---|
| Orbit | Drag the view (one finger on touch screens) |
| Zoom | Scroll, or pinch |
| Switch simulation | The tabs at the top, or keys **1** and **2** |
| Pause / play | **Space** or the Pause button |
| Reset the camera | **R** or Reset view |
| Restart the simulation | The Restart button |
| Fullscreen | **F** or ⤢ |
| Settings | The gear, or **⌘,** (Ctrl+, on Windows and Linux) |
| Tweak a value | Drag a slider, or click its number and type one |
| Undo your tweaks | **Reset** at the top of the controls panel |
| Read the physics and sources | Expand **The physics** at the bottom of the panel |
| Send feedback | The **Feedback** tab on the right edge opens a GitHub issue |

**Settings** (saved in this browser only):

- **Quality.** Low, Medium or High. Sets the black hole's ray steps, the waterfall's particle count (65k, 262k or 524k) and its ray-march steps.
- **Resolution.** Render scale from 40% to 100%.
- **Lower resolution automatically.** Drops resolution in small steps while frames take over 21 ms, and recovers below 15 ms.
- **Pixel density.** Caps the device pixel ratio (1×, 1.5× or 2×).
- **Theme.** Match system, Light or Dark.
- **Show frame rate.**

Your slider values are remembered per simulation in `localStorage`. The page still works if storage is blocked; values just don't persist.

## Build it

You need CMake 3.16+, a C++17 compiler, and [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) 3.1+ (tested with 6.0.12).

```bash
git clone https://github.com/amalmehta/GraphicsLibrary.git
```

```bash
source ~/emsdk/emsdk_env.sh
```

```bash
./scripts/build.sh
```

This runs the native tests, then writes `web/engine.js` and `web/engine.wasm`.

On macOS, emsdk needs Python 3.10+. If your default `python3` is older, use `EMSDK_PYTHON=/path/to/python3.12 ./emsdk install latest`.

## Run it locally

```bash
python3 -m http.server 8137 --directory web
```

Then open <http://localhost:8137>. You need an HTTP server, because browsers won't load `.wasm` from `file://`.

Append `?theme=light` or `?theme=dark` to the URL to force a theme for one visit, which is handy for screenshots.

## Test it

```bash
cmake -S . -B build/native && cmake --build build/native && ./build/native/tests
```

The tests are native C++ and need no GPU. They cover the math, the parameter registry, blackbody colour and both physics models. See [SYSTEM-DESIGN.md](SYSTEM-DESIGN.md#testing) for what is checked and the manual review list.

## Deploy

Every push to `main` runs `.github/workflows/pages.yml`. It runs the tests, builds the WebAssembly, and publishes `web/` to GitHub Pages. In the repo, Pages must be set to **Source: GitHub Actions**.

## Add a simulation

1. Create `sims/<name>/` with a class deriving from `gfx::Scene` (see `engine/include/gfx/scene.hpp`):
   - `id()`, `name()` and `tag()`. The tag is the category shown as a coloured chip; reuse one or add a hue in `web/styles.css`.
   - `init()` builds shaders and GPU resources. Return `false` on failure; the error reaches the page.
   - `setupCamera()` sets the default view.
   - `update(frame)` steps the simulation. It isn't called while paused.
   - `render(frame)` draws in linear light into the bound HDR target.
   - Declare parameters in the constructor with `params_.slider/toggle/choice`, grouped with `params_.group("…")`. Call `addImageParams()` for exposure, bloom and vignette.
2. Add the `.cpp` to the `engine` target in `CMakeLists.txt`.
3. Register it in `platform/web/main.cpp` with `gEngine.add(std::make_unique<…>())`.
4. Optionally add a caption and a physics note with sources to `INFO` in `web/app.js`.

The controls panel is generated from the parameter list, so you don't write any UI code.
