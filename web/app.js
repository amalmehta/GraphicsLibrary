// UI shell around the WebAssembly engine: sim tabs, controls built from each
// scene's parameter list, settings, input and the frame loop.

const REPO = "https://github.com/amalmehta/GraphicsLibrary";
const $ = (id) => document.getElementById(id);

// ---- per-viewer storage (may be unavailable; everything works without it) ----
const store = {
  get(key, fallback) {
    try { const v = localStorage.getItem(key); return v == null ? fallback : JSON.parse(v); }
    catch { return fallback; }
  },
  set(key, value) { try { localStorage.setItem(key, JSON.stringify(value)); } catch {} },
};

const settings = Object.assign(
  { quality: 1, scale: 1, auto: true, dpr: Math.min(2, window.devicePixelRatio || 1) > 1.4 ? 1.5 : 1, theme: "system", fps: false },
  store.get("gl.settings", {}),
);

// ---- what each simulation shows, and where the numbers come from ----
const INFO = {
  "black-hole": {
    caption: "A non-spinning black hole seen from 34 M away (M is its mass in geometric units). Each pixel follows a light ray backward through curved spacetime, so the disk behind the hole appears above and below it.",
    about: `
      <p><b>Light paths.</b> Rays obey the Schwarzschild orbit equation u″ + u = 3Mu², integrated with RK4. Rays with impact parameter under 3√3 M ≈ 5.20 M fall in. That threshold is the edge of the shadow. <cite>Chandrasekhar, <i>The Mathematical Theory of Black Holes</i>, 1983</cite></p>
      <ul>
        <li>Event horizon at 2 M, photon sphere at 3 M, innermost stable orbit at 6 M (the disk's inner edge).</li>
        <li>Disk temperature T ∝ F<sup>¼</sup>, with flux F ∝ r⁻³(1 − √(r<sub>in</sub>/r)). It peaks at 49/36 r<sub>in</sub>. <cite>Shakura &amp; Sunyaev 1973</cite></li>
        <li>Doppler and gravitational shift: g = √(1 − 3M/r) / (1 − Ωλ). The light stays a blackbody at gT, and its brightness scales as (gT)⁴. That is why the approaching side is brighter and bluer. <cite>Cunningham 1975; Luminet 1979</cite></li>
      </ul>
      <p class="est"><b>Estimates and artistic choices.</b> Real disks peak near 10⁷ K in X-rays, so here the temperature is scaled into visible light. The gas texture is decorative. The flux uses the Newtonian profile, not the full Page–Thorne one. The camera is treated as an observer at infinity, which is about 3% off at 34 M. The stars are procedural.</p>
      <p class="est"><b>Checked in tests.</b> Capture at b = 3√3 M ± 0.05, weak-field bending 4M/b within 2%, a circular photon orbit at 3 M, and the redshift at the ISCO.</p>`,
  },
  waterfall: {
    caption: "A 12 m waterfall made of a few hundred thousand water parcels. Each one falls under gravity and air drag, hits the rocks and the pool, and breaks into spray and mist.",
    about: `
      <p><b>Motion.</b> Each parcel follows a = g − k|v|v with g = 9.81 m/s². The drag constant k = g / v<sub>t</sub>² is set by the terminal speed. A 2 mm raindrop reaches about 9 m/s. <cite>Gunn &amp; Kinzer 1949</cite> With no drag the 12 m drop takes √(2h/g) ≈ 1.56 s.</p>
      <ul>
        <li>Rocks are a signed-distance field shared by the physics and the renderer, and are drawn by sphere tracing. <cite>Hart 1996</cite></li>
        <li>On impact, the normal velocity reverses with restitution 0.15 and the tangential velocity loses 30%.</li>
        <li>Bloom follows Jimenez 2014. Tone mapping is the ACES fit by Narkowicz 2015.</li>
      </ul>
      <p class="est"><b>Estimates and artistic choices.</b> Restitution, friction, the splash and mist chances, and the mist's swirl were tuned by eye, not measured. This is a ballistic particle model, not a Navier–Stokes solver: there is no pressure, surface tension or parcel-to-parcel interaction.</p>
      <p class="est"><b>Checked in tests.</b> Free-fall time, the approach to terminal speed, and collision resolution against the rock field.</p>`,
  },
};

// ---- theme (?theme=light|dark overrides for this visit, e.g. for screenshots) ----
const themeOverride = new URLSearchParams(location.search).get("theme");
function applyTheme() {
  const root = document.documentElement;
  const theme = themeOverride || settings.theme;
  if (theme === "system") root.removeAttribute("data-theme");
  else root.setAttribute("data-theme", theme);
}
applyTheme();

// ---- engine boot ----
let M, api, scenes = [], current = -1, paused = false;

function showMessage(text) {
  const m = $("message");
  m.textContent = text;
  m.hidden = false;
}

async function boot() {
  try {
    M = await createEngine();
  } catch (e) {
    showMessage("The engine failed to load. Try reloading the page.");
    console.error(e);
    return;
  }
  api = {
    init: M.cwrap("gl_init", "number", ["string"]),
    scenes: M.cwrap("gl_scenes", "string", []),
    select: M.cwrap("gl_select", "number", ["number"]),
    params: M.cwrap("gl_params", "string", []),
    setParam: M.cwrap("gl_set_param", "number", ["string", "number"]),
    resetParams: M.cwrap("gl_reset_params", null, []),
    frame: M._gl_frame, drag: M._gl_drag, zoom: M._gl_zoom,
    resetView: M._gl_reset_view, resetSim: M._gl_reset_sim,
    quality: M._gl_set_quality, scale: M._gl_set_render_scale, auto: M._gl_set_auto_quality,
    pause: M._gl_set_paused, fps: M._gl_fps, effScale: M._gl_scale,
    error: M.cwrap("gl_error", "string", []),
  };
  if (!api.init("#canvas")) {
    showMessage(api.error() || "This browser can't run WebGL2.");
    return;
  }
  applySettings();
  scenes = JSON.parse(api.scenes());
  buildTabs();
  const want = scenes.findIndex((s) => s.id === (location.hash.slice(1) || store.get("gl.scene", "")));
  select(want >= 0 ? want : 0);
  requestAnimationFrame(loop);
}

// ---- tabs ----
function buildTabs() {
  const nav = $("sims");
  nav.setAttribute("role", "tablist");
  scenes.forEach((s, i) => {
    const b = document.createElement("button");
    b.className = "sim-tab";
    b.setAttribute("role", "tab");
    b.innerHTML = `<span>${s.name}</span><span class="tag" data-tag="${s.tag}">${s.tag}</span>`;
    b.onclick = () => select(i);
    nav.appendChild(b);
  });
}

function select(i) {
  if (i === current || i < 0 || i >= scenes.length) return;
  if (!api.select(i)) {
    showMessage(`Couldn't start ${scenes[i].name}: ${api.error()}`);
    return;
  }
  $("message").hidden = true;
  current = i;
  const s = scenes[i];
  [...$("sims").children].forEach((b, j) => b.setAttribute("aria-selected", String(j === i)));
  history.replaceState(null, "", location.search + "#" + s.id);
  store.set("gl.scene", s.id);
  document.title = `${s.name} · Graphics Library`;
  // Restore this viewer's tweaks for the scene.
  const saved = store.get("gl.params." + s.id, {});
  for (const [k, v] of Object.entries(saved)) api.setParam(k, v);
  buildControls();
  $("caption").textContent = INFO[s.id]?.caption ?? "";
  $("aboutBody").innerHTML = INFO[s.id]?.about ?? "";
  $("panelTitle").textContent = s.name;
  $("feedback").href = `${REPO}/issues/new?title=${encodeURIComponent("Feedback: " + s.name)}&labels=feedback`;
}

// ---- controls, generated from the scene's parameter list ----
function fmt(v, step) {
  const d = step >= 1 ? 0 : Math.min(3, Math.max(0, -Math.floor(Math.log10(step))));
  return Number(v).toFixed(d);
}

function saveParam(id, v) {
  const key = "gl.params." + scenes[current].id;
  const saved = store.get(key, {});
  saved[id] = v;
  store.set(key, saved);
}

function setFill(range) {
  const p = ((range.value - range.min) / (range.max - range.min)) * 100;
  range.style.setProperty("--fill", p + "%");
}

function buildControls() {
  const root = $("controls");
  root.innerHTML = "";
  const params = JSON.parse(api.params());
  const collapsed = store.get("gl.collapsed", {});
  const groups = new Map();
  for (const p of params) {
    if (!groups.has(p.group)) groups.set(p.group, []);
    groups.get(p.group).push(p);
  }
  for (const [name, list] of groups) {
    const g = document.createElement("div");
    g.className = "group";
    const gKey = scenes[current].id + "/" + name;
    if (collapsed[gKey] ?? name === "Image") g.classList.add("collapsed");
    const head = document.createElement("button");
    head.textContent = name;
    head.setAttribute("aria-expanded", String(!g.classList.contains("collapsed")));
    head.onclick = () => {
      g.classList.toggle("collapsed");
      const c = store.get("gl.collapsed", {});
      c[gKey] = g.classList.contains("collapsed");
      store.set("gl.collapsed", c);
      head.setAttribute("aria-expanded", String(!c[gKey]));
    };
    g.appendChild(head);
    for (const p of list) g.appendChild(control(p));
    root.appendChild(g);
  }
}

function control(p) {
  const el = document.createElement("div");
  el.className = "ctrl";
  const uid = "p-" + p.id;
  const help = p.help ? `<span class="help">${p.help}</span>` : "";
  if (p.kind === "slider") {
    el.innerHTML = `
      <div class="ctrl-top">
        <label for="${uid}">${p.label}</label>
        <span><input class="val" type="number" aria-label="${p.label} value" step="${p.step}" min="${p.min}" max="${p.max}" value="${fmt(p.value, p.step)}"><span class="unit">${p.unit}</span></span>
      </div>
      <input type="range" id="${uid}" min="${p.min}" max="${p.max}" step="${p.step}" value="${p.value}">${help}`;
    const range = el.querySelector("input[type=range]"), num = el.querySelector(".val");
    setFill(range);
    const apply = (v) => {
      v = Math.min(p.max, Math.max(p.min, Number(v)));
      if (!Number.isFinite(v)) return;
      api.setParam(p.id, v);
      range.value = v; num.value = fmt(v, p.step); setFill(range);
      saveParam(p.id, v);
    };
    range.oninput = () => apply(range.value);
    num.onchange = () => apply(num.value);
    num.onkeydown = (e) => { if (e.key === "Enter") num.blur(); };
  } else if (p.kind === "toggle") {
    el.innerHTML = `<label class="switch"><span>${p.label}</span><input type="checkbox" ${p.value ? "checked" : ""}></label>${help}`;
    const cb = el.querySelector("input");
    cb.onchange = () => { api.setParam(p.id, cb.checked ? 1 : 0); saveParam(p.id, cb.checked ? 1 : 0); };
  } else {
    el.innerHTML = `<label>${p.label}</label><div class="seg" role="group" aria-label="${p.label}"></div>${help}`;
    const seg = el.querySelector(".seg");
    p.options.forEach((o, i) => {
      const b = document.createElement("button");
      b.textContent = o;
      b.setAttribute("aria-pressed", String(i === p.value));
      b.onclick = () => {
        api.setParam(p.id, i);
        saveParam(p.id, i);
        [...seg.children].forEach((x, j) => x.setAttribute("aria-pressed", String(j === i)));
      };
      seg.appendChild(b);
    });
  }
  return el;
}

$("resetParams").onclick = () => {
  api.resetParams();
  store.set("gl.params." + scenes[current].id, {});
  buildControls();
};

// ---- settings ----
function applySettings() {
  api.quality(settings.quality);
  api.scale(settings.scale);
  api.auto(settings.auto ? 1 : 0);
  $("fps").hidden = !settings.fps;
  applyTheme();
  store.set("gl.settings", settings);
}

function openSettings() {
  $("sQuality").value = settings.quality;
  $("sScale").value = settings.scale;
  $("sScaleOut").textContent = Math.round(settings.scale * 100) + "%";
  $("sAuto").checked = settings.auto;
  $("sDpr").value = String(settings.dpr);
  $("sTheme").value = settings.theme;
  $("sFps").checked = settings.fps;
  $("settings").showModal();
}
$("settingsBtn").onclick = openSettings;
$("sQuality").onchange = (e) => { settings.quality = +e.target.value; applySettings(); };
$("sScale").oninput = (e) => {
  settings.scale = +e.target.value;
  $("sScaleOut").textContent = Math.round(settings.scale * 100) + "%";
  applySettings();
};
$("sAuto").onchange = (e) => { settings.auto = e.target.checked; applySettings(); };
$("sDpr").onchange = (e) => { settings.dpr = +e.target.value; applySettings(); };
$("sTheme").onchange = (e) => { settings.theme = e.target.value; applySettings(); };
$("sFps").onchange = (e) => { settings.fps = e.target.checked; applySettings(); };

// ---- buttons and keys ----
function togglePause() {
  paused = !paused;
  api.pause(paused ? 1 : 0);
  $("pauseBtn").textContent = paused ? "Play" : "Pause";
}
function toggleFullscreen() {
  if (document.fullscreenElement) document.exitFullscreen();
  else $("stage").requestFullscreen?.();
}
$("pauseBtn").onclick = togglePause;
$("viewBtn").onclick = () => api.resetView();
$("simBtn").onclick = () => api.resetSim();
$("fullBtn").onclick = toggleFullscreen;

window.addEventListener("keydown", (e) => {
  if ((e.metaKey || e.ctrlKey) && e.key === ",") { e.preventDefault(); openSettings(); return; }
  const t = e.target;
  if (t.matches?.("input, select, textarea") || e.metaKey || e.ctrlKey || e.altKey || !api) return;
  if (e.key === " ") { e.preventDefault(); togglePause(); }
  else if (e.key === "r" || e.key === "R") api.resetView();
  else if (e.key === "f" || e.key === "F") toggleFullscreen();
  else if (/^[1-9]$/.test(e.key)) select(+e.key - 1);
});

// ---- pointer input: drag to orbit, wheel or pinch to zoom ----
const canvas = $("canvas");
const pointers = new Map();
let pinch = 0;
canvas.addEventListener("pointerdown", (e) => {
  canvas.setPointerCapture(e.pointerId);
  pointers.set(e.pointerId, { x: e.clientX, y: e.clientY });
  canvas.classList.add("dragging");
});
canvas.addEventListener("pointermove", (e) => {
  const prev = pointers.get(e.pointerId);
  if (!prev || !api) return;
  if (pointers.size === 1) api.drag(e.clientX - prev.x, e.clientY - prev.y);
  pointers.set(e.pointerId, { x: e.clientX, y: e.clientY });
  if (pointers.size === 2) {
    const [a, b] = [...pointers.values()];
    const d = Math.hypot(a.x - b.x, a.y - b.y);
    if (pinch) api.zoom((pinch - d) * 4);
    pinch = d;
  }
});
const endPointer = (e) => {
  pointers.delete(e.pointerId);
  if (pointers.size < 2) pinch = 0;
  if (!pointers.size) canvas.classList.remove("dragging");
};
canvas.addEventListener("pointerup", endPointer);
canvas.addEventListener("pointercancel", endPointer);
canvas.addEventListener("wheel", (e) => {
  e.preventDefault();
  api?.zoom(e.deltaMode === 1 ? e.deltaY * 30 : e.deltaY);
}, { passive: false });

// ---- frame loop ----
let last = 0, hudAt = 0;
function loop(t) {
  const dt = last ? (t - last) / 1000 : 1 / 60;
  last = t;
  const dpr = Math.min(window.devicePixelRatio || 1, settings.dpr);
  const w = Math.round(canvas.clientWidth * dpr), h = Math.round(canvas.clientHeight * dpr);
  if (canvas.width !== w || canvas.height !== h) { canvas.width = w; canvas.height = h; }
  api.frame(dt, w, h);
  if (settings.fps && t - hudAt > 500) {
    hudAt = t;
    $("fps").textContent = `${Math.round(api.fps())} fps · ${Math.round(api.effScale() * 100)}% resolution`;
  }
  requestAnimationFrame(loop);
}

boot();
