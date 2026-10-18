PROJECT NAME: GraphicsLibrary

META-INSTRUCTIONS:

<Read it all before acting. Ask about anything unclear, contradictory or
 underspecified — before starting and mid-build. Ask in the question widget
 (AskUserQuestion): related questions batched, concrete options, your
 recommendation first. Plain text only if the widget isn't available.>

<Don't expand scope. Anything not listed here is a proposal, including changes
 to this file — propose it, don't do it.>

<Prefer doing over describing: run the code, write the files, test it.>

<As soon as a first working version (the MVP) exists, open it for me before
 carrying on: launch the app, open the page in the browser, or run it on a real
 sample, and say in a line or two what to try. Do the same after each big change.>

<Always in scope, no proposal needed: when it goes on GitHub, a README that is
 easy to read at a glance — a line on what it is, then clear visuals
 (screenshots, a diagram or a chart), then links. Everything else goes in
 linked files: docs/INSTRUCTIONS.md (setup, run, use),
 docs/SYSTEM-DESIGN.md (see below) and docs/FILE-STRUCTURE.md (what's where). If what you're
 building is an application rather than a script, also a small unobtrusive feedback tab, and a settings
 button or tab (⌘, on the Mac) that gathers its preferences in one place.>

<If what you're building is an application, build it as a Mac app first; the
 website comes after, as its own step.>

<Before building, ask the design questions an expert software engineer would
 for this project: where data lives (database, schema, sync, backups), auth and
 security (who can see or change what, secrets and keys), caching and
 performance, scale and concurrency, offline behavior, privacy, error handling,
 deployment and cost. Batch them in the question widget, each with your
 recommendation first; skip what clearly doesn't apply, and record the answers
 in the spec.>

<When an application takes files as input, offer a visible Upload button that
 opens the file picker, accept files dragged anywhere onto the window (not
 only onto a small drop zone), and accept pasting them with ⌘V.>

<Color-code tags, categories and types by similarity: related ones share a
 hue family, with shades far enough apart to tell them apart at a glance, in
 both light and dark mode.>

<Name things the way a person would say them — "Goal Tracker", not
 goal_tracker — for the app, its windows, titles, files people open, repo
 descriptions and README headings. When you create the GitHub repo, name it
 with no "_" or "-": one word or joined words, e.g. GoalTracker.>

<Always in scope: a system design doc in the codebase, docs/SYSTEM-DESIGN.md,
 kept current as the build changes. Cover the architecture (with a Mermaid
 diagram), each component's job, the main flows, where data lives, the key
 design decisions and their trade-offs, how it's tested, and known limits.>

<Finish by listing every deliverable: path, what it is, how to check it works.>

<Git rules (no Claude attribution, never commit .claude/) are in
 ~/.claude/CLAUDE.md and apply on their own — nothing to repeat here.>

<Keep the changelog at the bottom current.>

LEARNED ACROSS PROJECTS:

<What held across several kinds of earlier projects: starting points, not rules.>

<Settle the deliverable's form, target platforms, minimum versions and distribution up front, including packaging, signing, hosting and who runs it. (across 11 kinds)>

<Decide which data sources, external services and licenses are allowed, test versus live mode, key storage, who pays, privacy, and offline behavior. (across 10 kinds)>

<Define vague terms, scope boundaries and scoring rules with concrete thresholds, formulas and done criteria, recording what is deferred. (across 9 kinds)>

<Validate generated or computed content against real sources, cite them, and label estimates or invented parameters so nothing fabricated passes as fact. (across 8 kinds)>

<Settle version control policy early: who creates and hosts the repository, its visibility, and whether to push or only commit locally. (across 7 kinds)>

<Gather real sample inputs and working credentials early, then verify live APIs, model output and system tools, keeping a fallback ready. (across 6 kinds)>

<Define each output's specs, tone and visual style before building, offering a few concrete concepts or mockups to pick from when open-ended. (across 5 kinds)>

<Plan a manual or human review checklist for what automated tests can't verify, such as hardware, live services, feel or taste. (across 4 kinds)>

LEARNED UI/UX PREFERENCES:

<Starting points from 13 earlier projects, not rules: use what fits this one, and ask when it differs.>

<Use a warm, light, inviting palette with sparing color accents instead of dull or gloomy themes. (seen in 3)>

<Prefer instant, direct actions like in-place editing, drag-and-drop and auto-scrolling to the target over lagging or multi-step manual flows. (seen in 3)>

<Make key elements like markers clearly visible, and tune sizes, color and mood in small, subtle increments rather than big jumps. (seen in 2)>

<Favor clear visuals, compact elements, concise wording and short plain names, moving detailed instructions behind a link. (seen in 2)>

<Keep layouts open and airy with generous spacing and a friendly, welcoming, safe feel rather than cramped or stiff screens. (seen in 2)>

<Support a polished dark mode on every platform, with all components checked in both themes, and showcase both in screenshots. (seen in 2)>

<Carry the app's icon everywhere it appears, such as the website, Dock and notifications, so it is instantly recognizable. (seen in 2)>

<Write copy in the owner's authentic personal voice, grounded in true details and understated rather than embellished. (tentative)>

<Avoid cluttered, cheesy or overdone visuals and content; keep the interface clean, restrained, readable and easy on the eyes, revealing extras on demand. (seen in 4)>

<Avoid generic, template-looking designs that feel obviously AI-made; aim for distinctive, credible craft that impresses a discerning professional viewer. (tentative)>

LEARNED FOR VISUALIZATION PROJECTS:

<Starting points from 16 earlier visualization projects, not rules: use what fits this one, and ask when it differs.>

<Settle ambiguous terms, intended scale, the deliverable's form (library, content site, app, game, real-time scene or rendered video), stack, platform, distribution and hosting. (seen in 16)>

<Decide what '3D' means, what the scene shows, which views, camera modes and controls to build, and whether models are coded, generated or downloaded. (seen in 10)>

<Settle privacy, persistence and external services: who views it, what stays on-device, which AI or APIs are used, who pays, and offline fallback. (seen in 9)>

<Settle audio and other copyrighted assets: their source, licences and likenesses, whether users bring their own, and whether visuals react to them. (seen in 3)>

<Settle the visual tone early, restrained or showy, and apply it across the whole app; requests for eye candy can produce gimmicky results. (seen in 2)>

<Decide the map's base layer and source, geographic granularity, and how places are located, bounded and labelled on it. (tentative)>

<Include cited sources, label estimates and assumptions, and check computed outputs, prices or other simulated values against published or real-world figures. (seen in 9)>

CONTEXT:

Project type: Visualization (data or ideas made visible and explorable).

can a full high quality graphics library in C++ that can simulate any visualization? like a waterfall sim or a black hole simulation

DELIVERABLES:

website with the graphics sim

OPEN QUESTIONS / ASSUMPTIONS:

Answered by owner (2026-10-17):
- Stack: C++17 library compiled to WebAssembly with Emscripten, rendered with WebGL2. The same core also builds natively with clang for headless tests.
- Scope v1: the general engine (math, GL wrappers, render passes and bloom, camera, parameter system, scene registry) plus 2 flagship sims: Black Hole and Waterfall. More sims are proposals.
- Tone: physically grounded and restrained, with cited physics. The page is warm and light with a dark mode; the canvas is dark.
- Hosting: a public GitHub repo named GraphicsLibrary, deployed to GitHub Pages.

Decided without asking (design questions that barely apply to a static site):
- Data: none server-side. Preferences (quality, theme, last sim) go in localStorage, wrapped in try/catch.
- Auth/security: none needed. No secrets, keys or third-party scripts at runtime.
- Feedback tab: opens a prefilled GitHub issue on the repo, so no data is collected.
- Performance: Quality presets (Low/Medium/High) scale the render resolution, ray-march steps and particle count. An optional auto-quality mode drops resolution when frame time exceeds 20 ms.
- Offline: works offline once loaded, since all assets are static.
- Errors: if WebGL2 or float render targets are unsupported, show a plain message instead of a blank canvas.
- No file input, so no Upload button. Sim parameters are numbers in the settings/controls.
- Black Hole physics: Schwarzschild metric in geometric units (G=c=1, r_s=2M). Photon paths come from integrating the Binet equation u'' = -u + 3Mu² (Chandrasekhar, "Mathematical Theory of Black Holes", 1983). Photon sphere at 3M, ISCO at 6M. Disk temperature follows the Novikov–Thorne/Page–Thorne profile, with Doppler and gravitational shift g = 1/(γ(1 - v·n)) · sqrt(1-2M/r) (Luminet 1979). Disk color comes from a blackbody approximation. Rendering is art-directed where noted: brightness scaling and disk turbulence are artistic, not measured.
- Waterfall: a GPU particle sim (state in float textures, ping-pong) with gravity, quadratic air drag and collision against an analytic rock/ledge field. Spray/mist is a second particle class. Not a full Navier–Stokes solver; this is labeled in the docs.
- Black Hole flux uses the Newtonian Shakura–Sunyaev profile, not full Page–Thorne. It is labeled as such in the UI and docs.
- Added warm-up so the waterfall starts already flowing (4 s of simulated time spread over the first frames).
- Added a `?theme=light|dark` URL override, used to capture README screenshots in both themes.
- Feedback goes to GitHub issues on amalmehta/GraphicsLibrary. Build output (engine.js/.wasm) isn't committed; CI builds it.

CHANGELOG:

- 2026-10-17 — created
- 2026-09-15 — added meta-instruction: built-out applications include a small feedback tab
- 2026-09-15 — added meta-instruction: no "Claude" attribution in commits, PRs, or branches
- 2026-09-16 — added meta-instruction: always include a README when adding to GitHub
- 2026-09-16 — changed meta-instruction: ask clarifying questions in the question widget
- 2026-09-17 — added meta-instructions: Claude never a contributor; never commit .claude/
- 2026-09-26 — compressed the meta-instructions and every field prompt; git rules moved to the global instruction file
- 2026-09-27 — added meta-instruction: applications are built as a Mac app first, then a website
- 2026-09-28 — folded inputs, instructions, constraints, deliverables and done criteria into one free-form CONTEXT
- 2026-09-28 — changed meta-instruction: a README on GitHub always includes a visual
- 2026-09-28 — added meta-instruction: name things like a person would, never snake_case
- 2026-09-28 — changed meta-instruction: README leads with visuals; instructions live in a linked guide
- 2026-09-28 — changed meta-instruction: README is visuals and links; details in docs/INSTRUCTIONS.md and docs/FILE-STRUCTURE.md
- 2026-09-29 — changed meta-instruction: GitHub repo names have no "_" or "-"
- 2026-10-02 — added a DELIVERABLES field after CONTEXT
- 2026-10-02 — added meta-instruction: every project has a system design doc at docs/SYSTEM-DESIGN.md
- 2026-10-05 — added meta-instruction: applications include a settings button or tab
- 2026-10-08 — added meta-instruction: color-code tags by similarity (related share a hue family, each distinct)
- 2026-10-09 — added meta-instruction: apps that take files have an Upload button, drop anywhere, and paste
- 2026-10-17 — added meta-instruction: before building, ask the design questions an expert engineer would (data, security, caching, scale, deployment…)
- 2026-10-18 — added meta-instruction: open the MVP for me as soon as a first working version exists, and after each big change
- 2026-10-17 — built v1: C++17 engine (WebGL2 via Emscripten), Black Hole and Waterfall sims, website with settings (⌘,) and a feedback tab, native tests, docs, GitHub Pages deploy
