#include "gfx/engine.hpp"

#include <algorithm>

namespace gfx {

bool Engine::init() { return post_.init(); }

bool Engine::select(int index) {
    if (index < 0 || index >= int(scenes_.size())) return false;
    inited_.resize(scenes_.size(), false);
    if (!inited_[index]) {
        if (!scenes_[index]->init()) return false;
        inited_[index] = true;
    }
    current_ = index;
    resetView();
    camera_.snap();
    return true;
}

void Engine::resetView() {
    if (Scene* s = scene()) {
        OrbitCamera fresh;
        s->setupCamera(fresh);
        camera_ = fresh;
    }
}

void Engine::resetSim() {
    if (Scene* s = scene()) s->reset();
}

void Engine::ensureTarget(int w, int h) {
    if (hdr_.fbo() && hdr_.width() == w && hdr_.height() == h) return;
    TextureDesc d;
    d.width = w; d.height = h;
    d.internalFormat = GL_RGBA16F; d.format = GL_RGBA; d.type = GL_HALF_FLOAT;
    hdr_.create({d}, true);
    post_.resize(w, h);
}

void Engine::frame(float dt, int canvasW, int canvasH) {
    Scene* s = scene();
    if (!s || canvasW <= 0 || canvasH <= 0) return;

    // Frame-time smoothing; auto-quality nudges resolution in small steps so
    // the image never jumps visibly.
    frameMs_ = frameMs_ * 0.95f + dt * 1000 * 0.05f;
    fps_ = 1000.f / std::max(frameMs_, 1.f);
    if (autoQuality_) {
        if (frameMs_ > 21) autoScale_ = std::max(0.5f, autoScale_ - 0.01f);
        else if (frameMs_ < 15) autoScale_ = std::min(1.f, autoScale_ + 0.005f);
    } else {
        autoScale_ = 1;
    }

    float scale = effectiveScale();
    int w = std::max(16, int(canvasW * scale)), h = std::max(16, int(canvasH * scale));
    // Quantise so small scale changes don't reallocate every frame.
    w = (w + 7) / 8 * 8;
    h = (h + 7) / 8 * 8;
    ensureTarget(w, h);

    float step = std::min(dt, 1.f / 20);
    camera_.update(step);
    if (!paused_) time_ += step;

    Frame f;
    f.time = time_;
    f.dt = paused_ ? 0 : step;
    f.width = w; f.height = h;
    f.camera = &camera_;
    f.quality = quality_;

    if (!paused_) s->update(f);

    hdr_.bind();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    s->render(f);

    PostSettings ps;
    ps.exposureEV = s->params().get("exposure");
    ps.bloom = s->params().get("bloom");
    ps.vignette = s->params().get("vignette");
    post_.apply(hdr_.color(), canvasW, canvasH, ps, time_);
}

}  // namespace gfx
