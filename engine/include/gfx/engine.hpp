// The engine: a registry of scenes, the frame loop, input, the HDR target and
// post-processing. Platform code (platform/web) owns the GL context and calls in.
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "gfx/postfx.hpp"
#include "gfx/scene.hpp"

namespace gfx {

class Engine {
public:
    bool init();  // needs a current GL context
    void add(std::unique_ptr<Scene> s) { scenes_.push_back(std::move(s)); }

    bool select(int index);  // lazily inits the scene; false if it fails
    int current() const { return current_; }
    Scene* scene() { return current_ >= 0 ? scenes_[current_].get() : nullptr; }
    const std::vector<std::unique_ptr<Scene>>& scenes() const { return scenes_; }

    // Advance by dt seconds and draw at the given canvas size.
    void frame(float dt, int canvasW, int canvasH);

    void drag(float dx, float dy) { camera_.drag(dx, dy); }
    void zoom(float delta) { camera_.zoom(delta); }
    void resetView();
    void resetSim();

    void setQuality(Quality q) { quality_ = q; }
    void setRenderScale(float s) { renderScale_ = clampf(s, 0.25f, 1.f); }
    void setAutoQuality(bool on) { autoQuality_ = on; }
    void setPaused(bool p) { paused_ = p; }
    float effectiveScale() const { return renderScale_ * autoScale_; }
    float fps() const { return fps_; }
    OrbitCamera& camera() { return camera_; }

private:
    void ensureTarget(int w, int h);

    std::vector<std::unique_ptr<Scene>> scenes_;
    std::vector<bool> inited_;
    int current_ = -1;
    OrbitCamera camera_;
    RenderTarget hdr_;
    PostFX post_;
    Quality quality_ = Quality::Medium;
    float renderScale_ = 1, autoScale_ = 1, time_ = 0, fps_ = 60, frameMs_ = 16;
    bool autoQuality_ = true, paused_ = false;
};

}  // namespace gfx
