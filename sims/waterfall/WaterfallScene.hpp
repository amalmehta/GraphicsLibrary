#pragma once
#include "gfx/scene.hpp"

namespace wf {

// A 12 m waterfall: GPU particles (water, spray, mist) over a ray-marched cliff,
// ledge, boulders and plunge pool.
class WaterfallScene : public gfx::Scene {
public:
    WaterfallScene();
    std::string id() const override { return "waterfall"; }
    std::string name() const override { return "Waterfall"; }
    std::string tag() const override { return "Fluids"; }
    bool init() override;
    void setupCamera(gfx::OrbitCamera& cam) override;
    void update(const gfx::Frame& f) override;
    void render(const gfx::Frame& f) override;
    void reset() override;

private:
    bool allocate(gfx::Quality q);
    gfx::vec3 sunDir() const;

    gfx::Shader update_, particles_, env_;
    gfx::FullscreenPass quad_;
    gfx::PingPong state_;
    int texW_ = 0, texH_ = 0;
    gfx::Quality allocated_ = gfx::Quality::Medium;
    bool hasState_ = false;
    float accumulator_ = 0;
    int warmup_ = 0;  // extra steps still to run so the fall starts already flowing
    unsigned frame_ = 0;
};

}  // namespace wf
