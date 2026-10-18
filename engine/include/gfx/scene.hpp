// The one interface a simulation implements. The engine owns the frame loop,
// camera, HDR target and post-processing; a scene only simulates and draws.
#pragma once
#include <string>

#include "gfx/camera.hpp"
#include "gfx/gl.hpp"
#include "gfx/params.hpp"

namespace gfx {

enum class Quality { Low = 0, Medium = 1, High = 2 };

struct Frame {
    float time = 0, dt = 0;  // seconds; dt is clamped to avoid huge steps
    int width = 0, height = 0;  // HDR target size in pixels
    OrbitCamera* camera = nullptr;  // scenes may set fov / auto-orbit from params
    Quality quality = Quality::Medium;
    float aspect() const { return height ? float(width) / float(height) : 1.f; }
};

class Scene {
public:
    virtual ~Scene() = default;
    virtual std::string id() const = 0;     // "black-hole"
    virtual std::string name() const = 0;   // "Black Hole"
    virtual std::string tag() const = 0;    // category, e.g. "Astrophysics"

    virtual bool init() = 0;                // create GL resources; false on failure
    virtual void setupCamera(OrbitCamera& cam) = 0;
    virtual void update(const Frame&) {}    // simulation step
    // Draw into the currently bound HDR target (linear light, RGBA16F + depth).
    virtual void render(const Frame&) = 0;
    virtual void reset() {}                 // restart the simulation state

    ParamSet& params() { return params_; }

protected:
    // Image params every scene shares; read by the engine's post-processing.
    void addImageParams(float exposure, float bloom) {
        params_.group("Image");
        params_.slider("exposure", "Exposure", exposure, -4, 4, 0.05f, "EV");
        params_.slider("bloom", "Bloom", bloom, 0, 1, 0.01f);
        params_.slider("vignette", "Vignette", 0.25f, 0, 1, 0.01f);
    }
    ParamSet params_;
};

}  // namespace gfx
