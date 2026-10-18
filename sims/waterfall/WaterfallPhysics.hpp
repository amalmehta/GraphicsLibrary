// Waterfall physics, CPU reference. The GPU update shader in
// WaterfallShaders.hpp runs the same integrator and the same rock field.
//
// Units: metres, seconds. Each particle is a parcel of water, not a single drop:
// it falls under gravity with quadratic air drag (a = g − k|v|v), collides with
// a signed-distance rock field, and can break into spray or mist. This is a
// ballistic particle model, not a Navier–Stokes solver: no pressure, surface
// tension or particle–particle interaction.
#pragma once
#include <cmath>

#include "gfx/math.hpp"

namespace wf {

using gfx::vec3;

constexpr float kGravity = 9.81f;     // m/s²
constexpr float kLipHeight = 12.f;    // m, top of the cliff
constexpr float kLipHalfWidth = 2.4f; // m

struct Drop {
    vec3 pos, vel;
};

// Drag constant k for a given terminal speed: at terminal speed g = k v².
inline float dragForTerminal(float terminalSpeed) {
    return kGravity / (terminalSpeed * terminalSpeed);
}

// Semi-implicit Euler step. `wind` is air velocity; drag acts on relative velocity.
inline void step(Drop& d, float dt, float k, vec3 wind, float gravityScale = 1.f) {
    vec3 rel = d.vel - wind;
    vec3 a = vec3(0, -kGravity * gravityScale, 0) - rel * (k * gfx::length(rel));
    d.vel += a * dt;
    d.pos += d.vel * dt;
}

// ---- Rock field (signed distance, metres; negative inside rock) --------------

inline float sdBox(vec3 p, vec3 b) {
    vec3 q{std::fabs(p.x) - b.x, std::fabs(p.y) - b.y, std::fabs(p.z) - b.z};
    vec3 m{std::fmax(q.x, 0.f), std::fmax(q.y, 0.f), std::fmax(q.z, 0.f)};
    return gfx::length(m) + std::fmin(std::fmax(q.x, std::fmax(q.y, q.z)), 0.f);
}
inline float sdSphere(vec3 p, vec3 c, float r) { return gfx::length(p - c) - r; }

// Cliff face at z = 0 (rock behind, z < 0), lip at y = 12, a ledge that splits
// the fall at y ≈ 5, and three boulders in the plunge pool (surface at y = 0).
inline float sceneSdf(vec3 p) {
    float cliff = sdBox(p - vec3(0, 5, -10), {40, 7, 10});
    float ledge = sdBox(p - vec3(-0.6f, 5, 1.0f), {1.4f, 0.3f, 1.0f}) - 0.2f;
    float b1 = sdSphere(p, {1.8f, -0.2f, 3.0f}, 1.0f);
    float b2 = sdSphere(p, {-1.5f, -0.4f, 4.2f}, 1.2f);
    float b3 = sdSphere(p, {0.2f, -0.6f, 5.5f}, 0.9f);
    return std::fmin(std::fmin(cliff, ledge), std::fmin(b1, std::fmin(b2, b3)));
}

inline vec3 sceneNormal(vec3 p) {
    const float e = 1e-3f;
    return gfx::normalize({sceneSdf(p + vec3(e, 0, 0)) - sceneSdf(p - vec3(e, 0, 0)),
                           sceneSdf(p + vec3(0, e, 0)) - sceneSdf(p - vec3(0, e, 0)),
                           sceneSdf(p + vec3(0, 0, e)) - sceneSdf(p - vec3(0, 0, e))});
}

// Pushes a penetrating particle back to the surface and reflects its normal
// velocity with the given restitution. Tangential velocity loses up to
// `friction`, scaled by impact speed (none below 0.5 m/s, full at 2 m/s) so
// resting contact slides.
inline bool collide(Drop& d, float restitution, float friction) {
    float s = sceneSdf(d.pos);
    if (s >= 0) return false;
    vec3 n = sceneNormal(d.pos);
    d.pos -= n * s;
    float vn = gfx::dot(d.vel, n);
    if (vn < 0) {
        vec3 vt = d.vel - n * vn;
        float impact = gfx::clampf((-vn - 0.5f) / 1.5f, 0, 1);
        d.vel = vt * (1 - friction * impact) - n * (vn * restitution);
    }
    return true;
}

}  // namespace wf
