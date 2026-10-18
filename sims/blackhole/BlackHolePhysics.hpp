// Schwarzschild black-hole physics, CPU reference. The GLSL in
// BlackHoleShaders.hpp implements the same equations per pixel; the tests in
// tests/ check this version against known results.
//
// Units: geometric, G = c = M = 1. Event horizon r = 2, photon sphere r = 3,
// innermost stable circular orbit (ISCO) r = 6.
//
// Null geodesics: the orbit equation u'' + u = 3 M u^2 (u = 1/r, ' = d/dphi;
// Chandrasekhar 1983, §20) is reproduced exactly by the Cartesian "force"
//   a = -3 M h^2 x / r^5,  h = |x × v| (conserved),
// so photon paths can be integrated as ordinary 3D trajectories. The time
// parameter is not proper or coordinate time, only the path shape is physical.
#pragma once
#include <cmath>

#include "gfx/math.hpp"

namespace bh {

using gfx::vec3;

constexpr float kHorizon = 2.f, kPhotonSphere = 3.f, kIsco = 6.f;
// Critical impact parameter: rays with b < 3√3 M fall in (photon capture).
constexpr float kCriticalImpact = 5.196152422706632f;

inline vec3 accel(vec3 x, float h2) {
    float r2 = gfx::dot(x, x);
    float r5 = r2 * r2 * std::sqrt(r2);
    return x * (-3.f * h2 / r5);
}

// One RK4 step of the geodesic (x, v) with step dt.
inline void rk4(vec3& x, vec3& v, float h2, float dt) {
    vec3 k1v = accel(x, h2), k1x = v;
    vec3 k2v = accel(x + k1x * (dt * .5f), h2), k2x = v + k1v * (dt * .5f);
    vec3 k3v = accel(x + k2x * (dt * .5f), h2), k3x = v + k2v * (dt * .5f);
    vec3 k4v = accel(x + k3x * dt, h2), k4x = v + k3v * dt;
    x += (k1x + k2x * 2.f + k3x * 2.f + k4x) * (dt / 6.f);
    v += (k1v + k2v * 2.f + k3v * 2.f + k4v) * (dt / 6.f);
}

struct TraceResult {
    bool captured = false;
    vec3 position, direction;  // where the ray ended up and which way it was going
    int steps = 0;
};

// Integrates a ray from x0 along unit direction d0. Step size scales with r so
// far-field steps are cheap and near-horizon steps fine.
inline TraceResult trace(vec3 x0, vec3 d0, float stepScale = 0.02f, int maxSteps = 20000,
                         float escapeRadius = 0) {
    vec3 x = x0, v = gfx::normalize(d0);
    float h2 = gfx::dot(gfx::cross(x, v), gfx::cross(x, v));
    if (escapeRadius <= 0) escapeRadius = gfx::length(x0) * 2.f;
    TraceResult res;
    for (int i = 0; i < maxSteps; ++i) {
        float r = gfx::length(x);
        if (r < kHorizon) { res.captured = true; break; }
        if (r > escapeRadius && gfx::dot(x, v) > 0) break;
        float dt = gfx::clampf(stepScale * r, 1e-3f, 4.f);
        rk4(x, v, h2, dt);
        res.steps = i + 1;
    }
    res.position = x;
    res.direction = gfx::normalize(v);
    return res;
}

// ---- Thin accretion disk ------------------------------------------------------

// Radiated flux of a thin disk with a zero-torque inner edge, Newtonian form
// (Shakura & Sunyaev 1973): F(r) ∝ r^-3 (1 - sqrt(r_in / r)). The full
// relativistic Page–Thorne (1974) profile differs by tens of percent near the
// ISCO; we use this simpler shape and say so in the docs.
inline float diskFlux(float r, float rIn) {
    if (r <= rIn) return 0.f;
    return (1.f - std::sqrt(rIn / r)) / (r * r * r);
}

// The flux peaks at r = (49/36) r_in.
inline float diskFluxPeakRadius(float rIn) { return rIn * 49.f / 36.f; }

// Local effective temperature relative to the peak, T ∝ F^(1/4).
inline float diskTemperature(float r, float rIn, float peakKelvin) {
    float f = diskFlux(r, rIn) / diskFlux(diskFluxPeakRadius(rIn), rIn);
    return peakKelvin * std::pow(f > 0 ? f : 0, 0.25f);
}

// Keplerian angular velocity of a circular orbit (exact in Schwarzschild).
inline float keplerOmega(float r) { return std::pow(r, -1.5f); }

// Redshift factor g = E_obs / E_emit for a photon from disk gas on a circular
// orbit at radius r (gas rotates about +y), reaching an observer at infinity.
// lambda is the photon's angular momentum about +y per unit energy.
// g = sqrt(1 - 3M/r) / (1 - Ω λ)  (e.g. Luminet 1979, Cunningham 1975).
inline float redshiftFactor(float r, float lambda) {
    float ut = 1.f / std::sqrt(1.f - 3.f / r);
    return 1.f / (ut * (1.f - keplerOmega(r) * lambda));
}

}  // namespace bh
