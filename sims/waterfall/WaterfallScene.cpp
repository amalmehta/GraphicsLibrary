#include "WaterfallScene.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include "WaterfallPhysics.hpp"
#include "WaterfallShaders.hpp"

namespace wf {

namespace {
constexpr float kStep = 1.f / 120;  // fixed simulation step, seconds
struct Size { int w, h; };
constexpr Size kSizes[] = {{256, 256}, {512, 512}, {1024, 512}};  // 65k, 262k, 524k
}  // namespace

WaterfallScene::WaterfallScene() {
    auto& p = params_;
    p.group("Water");
    p.slider("flow", "Flow", 0.8f, 0.05f, 1, 0.01f).help = "Share of particles in use.";
    p.slider("lipSpeed", "Speed over the lip", 1.5f, 0.3f, 4, 0.05f, "m/s");
    p.slider("terminal", "Terminal speed", 9, 3, 15, 0.1f, "m/s").help =
        "Air drag limit. About 9 m/s for 2 mm drops (Gunn & Kinzer 1949); larger slugs of water fall faster.";
    p.slider("splash", "Splash", 0.45f, 0, 1, 0.01f).help = "Chance that water breaks into spray on impact.";
    p.slider("mist", "Mist", 0.2f, 0, 0.6f, 0.01f).help = "Chance that water hitting the pool becomes mist.";
    p.slider("wind", "Wind", 0.4f, -4, 4, 0.05f, "m/s");
    p.group("Look");
    p.slider("dropSize", "Drop size", 0.05f, 0.01f, 0.15f, 0.005f, "m");
    p.slider("mistSize", "Mist size", 0.7f, 0.2f, 2, 0.05f, "m");
    p.slider("mistAmount", "Mist density", 1, 0, 3, 0.05f);
    p.slider("sun", "Sun height", 38, 5, 85, 1, "°");
    p.slider("sunAz", "Sun direction", 35, -180, 180, 1, "°");
    p.group("Camera");
    p.slider("orbit", "Auto orbit", 0, 0, 0.3f, 0.005f, "rad/s");
    p.slider("fov", "Field of view", 50, 20, 90, 1, "°");
    addImageParams(0.f, 0.06f);
}

bool WaterfallScene::init() {
    quad_.init();
    std::string common = kCommonGLSL;
    if (!update_.build(gfx::kFullscreenVS, common + kUpdateFS, "waterfall-update")) return false;
    if (!particles_.build(kParticleVS, kParticleFS, "waterfall-particles")) return false;
    if (!env_.build(gfx::kFullscreenVS, common + kEnvFS, "waterfall-env")) return false;
    return allocate(gfx::Quality::Medium);
}

bool WaterfallScene::allocate(gfx::Quality q) {
    Size s = kSizes[int(q)];
    texW_ = s.w; texH_ = s.h;
    allocated_ = q;
    int n = texW_ * texH_;
    // Everything starts parked with staggered emit times so the fall fills in
    // smoothly rather than as one sheet.
    std::mt19937 rng(1234);
    std::uniform_real_distribution<float> u(0, 1);
    std::vector<float> pos(n * 4), vel(n * 4, 0.f);
    for (int i = 0; i < n; ++i) {
        pos[i * 4 + 1] = -100;
        pos[i * 4 + 3] = -u(rng) * 2.5f;
    }
    gfx::TextureDesc d;
    d.width = texW_; d.height = texH_;
    d.internalFormat = GL_RGBA32F; d.format = GL_RGBA; d.type = GL_FLOAT; d.filter = GL_NEAREST;
    for (gfx::RenderTarget* rt : {&state_.a, &state_.b})
        if (!rt->create({d, d}, false)) return false;
    // Upload initial state into the read target.
    state_.flip = false;
    glBindTexture(GL_TEXTURE_2D, state_.read().color(0));
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW_, texH_, GL_RGBA, GL_FLOAT, pos.data());
    glBindTexture(GL_TEXTURE_2D, state_.read().color(1));
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW_, texH_, GL_RGBA, GL_FLOAT, vel.data());
    hasState_ = true;
    warmup_ = int(4.f / kStep);  // 4 s of simulated flow
    return true;
}

void WaterfallScene::reset() { allocate(allocated_); }

void WaterfallScene::setupCamera(gfx::OrbitCamera& cam) {
    cam.target = {0, 5.2f, 2.5f};
    cam.distance = 23;
    cam.yaw = 0.55f;
    cam.pitch = 0.1f;
    cam.minDistance = 4;
    cam.maxDistance = 70;
    cam.minPitch = -0.15f;
    cam.maxPitch = 1.3f;
    cam.snap();
}

gfx::vec3 WaterfallScene::sunDir() const {
    float el = params_.get("sun") * gfx::kPi / 180, az = params_.get("sunAz") * gfx::kPi / 180;
    return gfx::normalize({std::cos(el) * std::sin(az), std::sin(el), std::cos(el) * std::cos(az)});
}

void WaterfallScene::update(const gfx::Frame& f) {
    if (!hasState_) return;
    if (f.quality != allocated_) allocate(f.quality);

    accumulator_ = std::fmin(accumulator_ + f.dt, kStep * 4);
    // Spread the warm-up over a few frames so switching scenes doesn't hitch.
    int extra = std::min(warmup_, 120);
    warmup_ -= extra;
    accumulator_ += extra * kStep;
    update_.use();
    glUniform2i(update_.loc("uSize"), texW_, texH_);
    update_.set("uDt", kStep);
    update_.set("uFlow", params_.get("flow"));
    update_.set("uLipSpeed", params_.get("lipSpeed"));
    update_.set("uDragWater", dragForTerminal(params_.get("terminal")));
    update_.set("uSplash", params_.get("splash"));
    update_.set("uMist", params_.get("mist"));
    update_.set("uWind", params_.get("wind"));
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    while (accumulator_ >= kStep) {
        accumulator_ -= kStep;
        glUniform1ui(update_.loc("uFrame"), ++frame_);
        state_.write().bind();
        update_.tex("uPos", state_.read().color(0), 0);
        update_.tex("uVel", state_.read().color(1), 1);
        quad_.draw();
        state_.swap();
    }
}

void WaterfallScene::render(const gfx::Frame& f) {
    gfx::OrbitCamera& cam = *f.camera;
    cam.autoOrbit = params_.get("orbit");
    cam.fovY = params_.get("fov") * gfx::kPi / 180;

    gfx::mat4 view = cam.view(), proj = cam.proj(f.aspect(), 0.1f, 300);
    gfx::vec3 sun = sunDir();
    // Warmer, dimmer sun near the horizon (art-directed, not a sky model).
    float lowSun = 1 - gfx::clampf(sun.y * 2.2f, 0, 1);
    gfx::vec3 sunColor = gfx::vec3(1.6f, 1.48f, 1.3f) * (1 - 0.4f * lowSun) +
                         gfx::vec3(0.25f, -0.15f, -0.45f) * lowSun;
    gfx::vec3 skyColor{0.2f, 0.26f, 0.34f};
    static const int steps[] = {70, 110, 160};

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    env_.use();
    env_.set("uEye", cam.eye());
    env_.set("uFwd", cam.forward());
    env_.set("uRight", cam.right());
    env_.set("uUp", cam.up());
    env_.set("uTanHalfFov", std::tan(cam.fovY * 0.5f));
    env_.set("uAspect", f.aspect());
    env_.set("uTime", f.time);
    env_.set("uViewProj", proj * view);
    env_.set("uSteps", steps[int(f.quality)]);
    env_.set("uSunDir", sun);
    env_.set("uSunColor", sunColor);
    env_.set("uSkyColor", skyColor);
    quad_.draw();

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // premultiplied alpha
    particles_.use();
    glUniform2i(particles_.loc("uSize"), texW_, texH_);
    particles_.tex("uPos", state_.read().color(0), 0);
    particles_.tex("uVel", state_.read().color(1), 1);
    particles_.set("uView", view);
    particles_.set("uProj", proj);
    particles_.set("uViewportH", float(f.height));
    particles_.set("uDropSize", params_.get("dropSize"));
    particles_.set("uMistSize", params_.get("mistSize"));
    particles_.set("uMistAmount", params_.get("mistAmount"));
    particles_.set("uSunDir", sun);
    particles_.set("uSunColor", sunColor);
    particles_.set("uSkyColor", skyColor);
    glBindVertexArray(0);
    glDrawArrays(GL_POINTS, 0, texW_ * texH_);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

}  // namespace wf
