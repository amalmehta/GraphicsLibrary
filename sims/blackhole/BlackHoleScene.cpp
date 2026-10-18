#include "BlackHoleScene.hpp"

#include <cmath>

#include "BlackHolePhysics.hpp"
#include "BlackHoleShaders.hpp"
#include "gfx/blackbody.hpp"

namespace bh {

namespace {
constexpr float kLutMin = 800, kLutMax = 60000;
constexpr int kLutSize = 256;
}  // namespace

BlackHoleScene::BlackHoleScene() {
    auto& p = params_;
    p.group("Physics");
    p.choice("mode", "Light paths", {"General relativity", "Lensing only", "Newtonian (straight)"}, 0).help =
        "Lensing only bends light but drops Doppler and gravitational shifts. Newtonian draws straight rays.";
    p.group("Disk");
    p.slider("diskIn", "Inner edge", kIsco, 3, 12, 0.1f, "M").help =
        "Thin disks end at the innermost stable circular orbit, 6 M for a non-spinning hole.";
    p.slider("diskOut", "Outer edge", 22, 10, 40, 0.5f, "M");
    p.slider("peakK", "Peak temperature", 5600, 2500, 30000, 100, "K").help =
        "Real stellar-mass disks peak near 10^7 K (X-rays). This is scaled into visible light.";
    p.slider("diskGain", "Brightness", 0.9f, 0.1f, 8, 0.05f);
    p.slider("opacity", "Opacity", 0.85f, 0.05f, 1, 0.01f);
    p.slider("turbulence", "Turbulence", 0.7f, 0, 1, 0.01f).help = "Artistic gas texture; not a physical model.";
    p.slider("spin", "Rotation speed", 1, 0, 4, 0.05f).help = "Animation speed of the gas pattern.";
    p.group("Sky");
    p.slider("stars", "Stars", 1, 0, 3, 0.05f);
    p.slider("galaxy", "Galactic band", 1, 0, 3, 0.05f);
    p.group("Camera");
    p.slider("orbit", "Auto orbit", 0.03f, 0, 0.3f, 0.005f, "rad/s");
    p.slider("fov", "Field of view", 45, 15, 90, 1, "°");
    addImageParams(0.f, 0.12f);
}

bool BlackHoleScene::init() {
    quad_.init();
    if (!trace_.build(gfx::kFullscreenVS, kTraceFS, "black-hole")) return false;
    std::vector<float> rgb = gfx::blackbodyLUT(kLutSize, kLutMin, kLutMax), rgba;
    for (int i = 0; i < kLutSize; ++i)
        rgba.insert(rgba.end(), {rgb[i * 3], rgb[i * 3 + 1], rgb[i * 3 + 2], 1.f});
    gfx::TextureDesc d;
    d.width = kLutSize; d.height = 1;
    d.internalFormat = GL_RGBA16F; d.format = GL_RGBA; d.type = GL_FLOAT;
    lut_.create(d, rgba.data());
    return true;
}

void BlackHoleScene::setupCamera(gfx::OrbitCamera& cam) {
    cam.target = {0, 0, 0};
    cam.distance = 34;
    cam.pitch = 0.11f;  // a few degrees above the disk plane: the classic view
    cam.yaw = 0.4f;
    cam.minDistance = 6;
    cam.maxDistance = 120;
    cam.minPitch = -1.45f;
    cam.maxPitch = 1.45f;
    cam.snap();
}

void BlackHoleScene::render(const gfx::Frame& f) {
    gfx::OrbitCamera& cam = *f.camera;
    cam.autoOrbit = params_.get("orbit");
    cam.fovY = params_.get("fov") * gfx::kPi / 180;

    static const int steps[] = {200, 360, 700};
    static const float scales[] = {0.11f, 0.065f, 0.035f};
    int q = int(f.quality);

    trace_.use();
    trace_.set("uEye", cam.eye());
    trace_.set("uFwd", cam.forward());
    trace_.set("uRight", cam.right());
    trace_.set("uUp", cam.up());
    trace_.set("uTanHalfFov", std::tan(cam.fovY * 0.5f));
    trace_.set("uAspect", f.aspect());
    trace_.set("uTime", f.time);
    trace_.set("uMaxSteps", steps[q]);
    trace_.set("uStepScale", scales[q]);
    trace_.set("uMode", int(params_.get("mode")));
    trace_.set("uDiskIn", params_.get("diskIn"));
    trace_.set("uDiskOut", std::fmax(params_.get("diskOut"), params_.get("diskIn") + 1));
    trace_.set("uPeakK", params_.get("peakK"));
    trace_.set("uDiskGain", params_.get("diskGain"));
    trace_.set("uDiskOpacity", params_.get("opacity"));
    trace_.set("uTurbulence", params_.get("turbulence"));
    trace_.set("uSpin", params_.get("spin"));
    trace_.set("uStarGain", params_.get("stars"));
    trace_.set("uGalaxyGain", params_.get("galaxy"));
    trace_.tex("uBlackbody", lut_.id(), 0);
    trace_.set("uLutRange", gfx::vec2{std::log(kLutMin), std::log(kLutMax)});
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    quad_.draw();
}

}  // namespace bh
