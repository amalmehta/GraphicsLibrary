# File structure

```
GraphicsLibrary/
├── README.md                    Front page: what it is, screenshots, links
├── GraphicsLibrary.md           Project spec, decisions and changelog
├── CMakeLists.txt               Native (tests) and Emscripten (engine) builds
├── scripts/build.sh             Run tests, then build the WebAssembly into web/
├── .github/workflows/pages.yml  CI: test, build, deploy to GitHub Pages
│
├── engine/                      The library: no simulation-specific code
│   ├── include/gfx/
│   │   ├── math.hpp             vec3, mat4, perspective/lookAt (header-only)
│   │   ├── params.hpp           Parameter registry → JSON for the UI
│   │   ├── blackbody.hpp        Planck + CIE 1931 → linear sRGB
│   │   ├── gl.hpp               Shader, Texture, RenderTarget, PingPong, FullscreenPass
│   │   ├── camera.hpp           Eased orbit camera
│   │   ├── scene.hpp            The Scene interface and Frame
│   │   ├── postfx.hpp           Bloom + ACES tone mapping
│   │   └── engine.hpp           Scene registry, frame loop, auto-quality
│   └── src/                     Implementations of the above
│
├── sims/
│   ├── blackhole/
│   │   ├── BlackHolePhysics.hpp  CPU reference: geodesics, disk flux, redshift
│   │   ├── BlackHoleShaders.hpp  Per-pixel geodesic ray tracer (GLSL)
│   │   └── BlackHoleScene.*      Parameters, camera, draw call
│   └── waterfall/
│       ├── WaterfallPhysics.hpp  CPU reference: drag integrator, rock SDF, collisions
│       ├── WaterfallShaders.hpp  Particle update/draw and the ray-marched scene (GLSL)
│       └── WaterfallScene.*      State textures, warm-up, passes
│
├── platform/web/main.cpp        WebGL2 context + C API exported to JavaScript
│
├── web/                         The site (what Pages serves)
│   ├── index.html               Layout, settings dialog
│   ├── styles.css               Warm light theme + dark theme, tag colours
│   ├── app.js                   UI: tabs, generated controls, input, frame loop
│   ├── icon.svg                 App icon (also the favicon)
│   └── engine.js / engine.wasm  Build output (not committed)
│
├── tests/tests.cpp              Native tests against published values
└── docs/
    ├── INSTRUCTIONS.md          Use, build, run, test, deploy, add a sim
    ├── SYSTEM-DESIGN.md         Architecture, flows, decisions, limits
    ├── FILE-STRUCTURE.md        This file
    └── images/                  README screenshots
```
