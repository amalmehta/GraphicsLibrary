# Graphics Library

A C++ graphics library compiled to WebAssembly. It runs physically grounded simulations live in the browser, starting with a ray-traced black hole and a particle waterfall.

**[Open the live site →](https://amalmehta.github.io/GraphicsLibrary/)**

![Black hole: light bent around a Schwarzschild black hole, with a Doppler-brightened accretion disk](docs/images/blackhole-dark.png)

![Waterfall: a 12 m fall of water parcels breaking into spray and mist over a ray-marched cliff](docs/images/waterfall-light.png)

```mermaid
flowchart LR
  JS["Web page<br/>app.js"] -->|C API| E["Engine<br/>frame loop · camera · params"]
  E --> S["Scene<br/>Black Hole · Waterfall"]
  S --> H["HDR target"]
  H --> P["Post FX<br/>bloom · ACES"]
  P --> C["Canvas (WebGL2)"]
```

| | |
|---|---|
| [Instructions](docs/INSTRUCTIONS.md) | Build, run, use, and add a simulation |
| [System design](docs/SYSTEM-DESIGN.md) | Architecture, flows, physics, trade-offs, limits |
| [File structure](docs/FILE-STRUCTURE.md) | What's where |
| [Light mode](docs/images/blackhole-light.png) · [Dark waterfall](docs/images/waterfall-dark.png) | More screenshots |
