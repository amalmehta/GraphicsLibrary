#pragma once
#include "gfx/scene.hpp"

namespace bh {

// Schwarzschild black hole with a thin accretion disk, ray-traced per pixel.
class BlackHoleScene : public gfx::Scene {
public:
    BlackHoleScene();
    std::string id() const override { return "black-hole"; }
    std::string name() const override { return "Black Hole"; }
    std::string tag() const override { return "Astrophysics"; }
    bool init() override;
    void setupCamera(gfx::OrbitCamera& cam) override;
    void render(const gfx::Frame& f) override;

private:
    gfx::Shader trace_;
    gfx::Texture lut_;
    gfx::FullscreenPass quad_;
};

}  // namespace bh
