// Orbit camera: drag to rotate around a target, scroll to dolly. Motion is
// eased so input feels smooth rather than jumpy.
#pragma once
#include <cmath>

#include "gfx/math.hpp"

namespace gfx {

struct OrbitCamera {
    vec3 target{0, 0, 0};
    float yaw = 0, pitch = 0.2f, distance = 10;  // goals
    float minDistance = 1, maxDistance = 100, minPitch = -1.5f, maxPitch = 1.5f;
    float fovY = 50 * kPi / 180;
    float autoOrbit = 0;  // radians per second

    // Eased state that actually renders.
    float curYaw = 0, curPitch = 0.2f, curDistance = 10;

    void snap() { curYaw = yaw; curPitch = pitch; curDistance = distance; }

    void drag(float dxPixels, float dyPixels) {
        yaw -= dxPixels * 0.005f;
        pitch = clampf(pitch + dyPixels * 0.005f, minPitch, maxPitch);
    }
    void zoom(float wheelDelta) {
        distance = clampf(distance * std::exp(wheelDelta * 0.001f), minDistance, maxDistance);
    }
    void update(float dt) {
        yaw += autoOrbit * dt;
        float k = 1 - std::exp(-dt * 10);
        curYaw += (yaw - curYaw) * k;
        curPitch += (pitch - curPitch) * k;
        curDistance += (distance - curDistance) * k;
    }

    vec3 eye() const {
        float cp = std::cos(curPitch);
        return target + vec3(cp * std::sin(curYaw), std::sin(curPitch), cp * std::cos(curYaw)) * curDistance;
    }
    vec3 forward() const { return normalize(target - eye()); }
    vec3 right() const { return normalize(cross(forward(), {0, 1, 0})); }
    vec3 up() const { return cross(right(), forward()); }
    mat4 view() const { return mat4::lookAt(eye(), target, {0, 1, 0}); }
    mat4 proj(float aspect, float zNear = 0.05f, float zFar = 500) const {
        return mat4::perspective(fovY, aspect, zNear, zFar);
    }
};

}  // namespace gfx
