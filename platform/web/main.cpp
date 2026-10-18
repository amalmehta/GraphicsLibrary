// Web entry point: creates the WebGL2 context on a canvas and exposes a small C
// API to JavaScript (web/app.js). JS owns the requestAnimationFrame loop.
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <memory>
#include <string>

#include "../../sims/blackhole/BlackHoleScene.hpp"
#include "../../sims/waterfall/WaterfallScene.hpp"
#include "gfx/engine.hpp"

namespace {
gfx::Engine gEngine;
std::string gBuf;  // backing store for strings returned to JS
}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE int gl_init(const char* canvasSelector) {
    EmscriptenWebGLContextAttributes a;
    emscripten_webgl_init_context_attributes(&a);
    a.majorVersion = 2;
    a.minorVersion = 0;
    a.alpha = false;
    a.depth = false;
    a.stencil = false;
    a.antialias = false;
    a.premultipliedAlpha = false;
    a.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context(canvasSelector, &a);
    if (ctx <= 0) {
        gfx::setGlError("WebGL2 is not available in this browser.");
        return 0;
    }
    emscripten_webgl_make_context_current(ctx);
    if (!emscripten_webgl_enable_extension(ctx, "EXT_color_buffer_float") &&
        !emscripten_webgl_enable_extension(ctx, "EXT_color_buffer_half_float")) {
        gfx::setGlError("This GPU can't render to floating-point textures (EXT_color_buffer_float).");
        return 0;
    }
    emscripten_webgl_enable_extension(ctx, "OES_texture_float_linear");

    gEngine.add(std::make_unique<bh::BlackHoleScene>());
    gEngine.add(std::make_unique<wf::WaterfallScene>());
    return gEngine.init() ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE const char* gl_scenes() {
    gBuf = "[";
    const auto& s = gEngine.scenes();
    for (size_t i = 0; i < s.size(); ++i)
        gBuf += std::string(i ? "," : "") + "{\"id\":\"" + s[i]->id() + "\",\"name\":\"" +
                s[i]->name() + "\",\"tag\":\"" + s[i]->tag() + "\"}";
    gBuf += "]";
    return gBuf.c_str();
}

EMSCRIPTEN_KEEPALIVE int gl_select(int i) { return gEngine.select(i) ? 1 : 0; }

EMSCRIPTEN_KEEPALIVE const char* gl_params() {
    gfx::Scene* s = gEngine.scene();
    gBuf = s ? s->params().toJson() : "[]";
    return gBuf.c_str();
}

EMSCRIPTEN_KEEPALIVE int gl_set_param(const char* id, float v) {
    gfx::Scene* s = gEngine.scene();
    return s && s->params().set(id, v) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void gl_reset_params() {
    if (gfx::Scene* s = gEngine.scene()) s->params().resetAll();
}

EMSCRIPTEN_KEEPALIVE void gl_frame(float dt, int w, int h) { gEngine.frame(dt, w, h); }
EMSCRIPTEN_KEEPALIVE void gl_drag(float dx, float dy) { gEngine.drag(dx, dy); }
EMSCRIPTEN_KEEPALIVE void gl_zoom(float d) { gEngine.zoom(d); }
EMSCRIPTEN_KEEPALIVE void gl_reset_view() { gEngine.resetView(); }
EMSCRIPTEN_KEEPALIVE void gl_reset_sim() { gEngine.resetSim(); }
EMSCRIPTEN_KEEPALIVE void gl_set_quality(int q) { gEngine.setQuality(gfx::Quality(q < 0 ? 0 : q > 2 ? 2 : q)); }
EMSCRIPTEN_KEEPALIVE void gl_set_render_scale(float s) { gEngine.setRenderScale(s); }
EMSCRIPTEN_KEEPALIVE void gl_set_auto_quality(int on) { gEngine.setAutoQuality(on != 0); }
EMSCRIPTEN_KEEPALIVE void gl_set_paused(int p) { gEngine.setPaused(p != 0); }
EMSCRIPTEN_KEEPALIVE float gl_fps() { return gEngine.fps(); }
EMSCRIPTEN_KEEPALIVE float gl_scale() { return gEngine.effectiveScale(); }
EMSCRIPTEN_KEEPALIVE const char* gl_error() { return gfx::lastGlError().c_str(); }

}  // extern "C"
