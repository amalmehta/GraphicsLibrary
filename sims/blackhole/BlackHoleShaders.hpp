// Per-pixel geodesic ray tracer for a Schwarzschild black hole with a thin
// accretion disk. Same equations as BlackHolePhysics.hpp (G = c = M = 1).
#pragma once

namespace bh {

inline const char* kTraceFS = R"(
in vec2 vUv;
out vec4 o;

uniform vec3 uEye, uFwd, uRight, uUp;
uniform float uTanHalfFov, uAspect, uTime;
uniform int uMaxSteps;
uniform float uStepScale;
uniform int uMode;            // 0 full GR, 1 lensing only (no shifts), 2 Newtonian (straight rays)
uniform float uDiskIn, uDiskOut, uPeakK, uDiskOpacity, uTurbulence, uSpin, uDiskGain;
uniform float uStarGain, uGalaxyGain;
uniform sampler2D uBlackbody; // 1-row LUT, log-spaced 800 K .. 60000 K, Y = 1
uniform vec2 uLutRange;       // (ln Tmin, ln Tmax)

const float PI = 3.14159265;

vec3 blackbody(float T) {
    float u = clamp((log(max(T, 1.0)) - uLutRange.x) / (uLutRange.y - uLutRange.x), 0.0, 1.0);
    return texture(uBlackbody, vec2(u, 0.5)).rgb;
}

// ---- noise ----
float hash13(vec3 p) {
    p = fract(p * 0.1031);
    p += dot(p, p.zyx + 31.32);
    return fract((p.x + p.y) * p.z);
}
vec3 hash33(vec3 p) {
    p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return fract((p.xxy + p.yxx) * p.zyx);
}
float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash13(vec3(i, 0.0)), b = hash13(vec3(i + vec2(1, 0), 0.0));
    float c = hash13(vec3(i + vec2(0, 1), 0.0)), d = hash13(vec3(i + vec2(1, 1), 0.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}
float fbm(vec2 p) {
    float s = 0.0, a = 0.5;
    for (int i = 0; i < 5; ++i) { s += a * vnoise(p); p = p * 2.03 + 17.1; a *= 0.5; }
    return s;
}
float vnoise3(vec3 p) {
    vec3 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash13(i), hash13(i + vec3(1,0,0)), f.x),
                   mix(hash13(i + vec3(0,1,0)), hash13(i + vec3(1,1,0)), f.x), f.y),
               mix(mix(hash13(i + vec3(0,0,1)), hash13(i + vec3(1,0,1)), f.x),
                   mix(hash13(i + vec3(0,1,1)), hash13(i + vec3(1,1,1)), f.x), f.y), f.z);
}

// ---- background sky: point stars plus a faint galactic band ----
vec3 stars(vec3 d) {
    vec3 col = vec3(0);
    for (int layer = 0; layer < 3; ++layer) {
        float scale = 60.0 + float(layer) * 70.0;
        vec3 p = d * scale;
        vec3 cell = floor(p);
        vec3 h = hash33(cell + float(layer) * 91.7);
        if (h.x > 0.1) continue;
        vec3 starPos = cell + 0.25 + 0.5 * h;
        float dist = length(p - starPos);
        float mag = pow(hash13(cell * 1.7 + 3.1), 6.0);
        float temp = mix(3200.0, 11000.0, hash13(cell + 8.3));
        col += blackbody(temp) * mag * exp(-dist * dist * 60.0) * 3.0;
    }
    return col;
}
vec3 galaxy(vec3 d) {
    // Band around a great circle tilted against the disk plane.
    vec3 n = normalize(vec3(0.3, 1.0, 0.45));
    float lat = dot(d, n);
    float band = exp(-lat * lat * 18.0);
    float dust = vnoise3(d * 6.0) * 0.6 + vnoise3(d * 14.0) * 0.4;
    vec3 c = mix(vec3(0.55, 0.5, 0.45), vec3(0.75, 0.65, 0.55), dust);
    return c * band * (0.25 + 0.75 * dust) * 0.05;
}
vec3 sky(vec3 d) { return stars(d) * uStarGain + galaxy(d) * uGalaxyGain; }

// ---- accretion disk ----
float diskFlux(float r) {
    if (r <= uDiskIn) return 0.0;
    return (1.0 - sqrt(uDiskIn / r)) / (r * r * r);
}

// Gas density pattern, sheared by differential (Keplerian) rotation. Two
// phases cross-fade so the pattern never winds up infinitely (flow-map trick).
float diskDensity(vec2 xz, float r) {
    float omega = pow(r, -1.5);
    float period = 12.0;
    float t = uTime * uSpin;
    float ph1 = fract(t / period), ph2 = fract(t / period + 0.5);
    float w = abs(ph1 - 0.5) * 2.0;
    float n = 0.0;
    for (int k = 0; k < 2; ++k) {
        float ph = k == 0 ? ph1 : ph2;
        float ang = -omega * (ph - 0.5) * period * 6.0;
        float c = cos(ang), s = sin(ang);
        vec2 q = mat2(c, -s, s, c) * xz;
        float v = fbm(q * 0.9 + float(k) * 13.0);
        n += (k == 0 ? (1.0 - w) : w) * v;
    }
    float rings = 0.5 + 0.5 * sin(r * 3.1 + n * 4.0);
    float d = mix(1.0, n * 1.6 * (0.6 + 0.4 * rings), uTurbulence);
    float edge = smoothstep(uDiskIn, uDiskIn + 0.6, r) * (1.0 - smoothstep(uDiskOut * 0.75, uDiskOut, r));
    return clamp(d, 0.0, 1.5) * edge;
}

// Emission from a disk crossing at position p. `towardCam` is the photon's
// pseudo-velocity toward the camera, normalised to |v| = 1 at infinity.
vec4 diskSample(vec3 p, vec3 towardCam) {
    float r = length(p.xz);
    if (r < uDiskIn || r > uDiskOut) return vec4(0);
    float fluxPeak = diskFlux(uDiskIn * 49.0 / 36.0);
    float T = uPeakK * pow(max(diskFlux(r) / fluxPeak, 0.0), 0.25);
    float g = 1.0;
    if (uMode == 0) {
        float lambda = cross(p, towardCam).y;   // L_y / E of the photon
        float omega = pow(r, -1.5);
        g = sqrt(max(1.0 - 3.0 / r, 1e-4)) / (1.0 - omega * lambda);
    }
    float Tobs = T * g;
    // A blackbody stays a blackbody under shifts (I_nu / nu^3 invariant), so the
    // observed light is a blackbody at g*T with bolometric intensity ∝ (gT)^4.
    float lum = pow(Tobs / uPeakK, 4.0) * uDiskGain;
    float dens = diskDensity(p.xz, r);
    float alpha = clamp(dens * uDiskOpacity, 0.0, 1.0);
    return vec4(blackbody(Tobs) * lum * dens, alpha);
}

void main() {
    vec2 ndc = vUv * 2.0 - 1.0;
    vec3 dir = normalize(uFwd + uRight * ndc.x * uTanHalfFov * uAspect + uUp * ndc.y * uTanHalfFov);

    vec3 x = uEye;
    vec3 v = dir;

    vec3 hv = cross(x, v);
    float h2 = dot(hv, hv);
    vec3 col = vec3(0);
    float trans = 1.0;
    float escapeR = max(length(x) * 1.2, uDiskOut * 1.5);
    bool captured = false;

    for (int i = 0; i < 1200; ++i) {
        if (i >= uMaxSteps) break;
        float r = length(x);
        if (r < 2.0) { captured = true; break; }
        if (r > escapeR && dot(x, v) > 0.0) break;
        float dt = clamp(uStepScale * r, 0.01, 3.0);
        vec3 xp = x;
        if (uMode == 2) {
            x += v * dt;
        } else {
            // RK4 on a = -3 h^2 x / r^5
            vec3 k1v = -3.0 * h2 * x / pow(dot(x, x), 2.5), k1x = v;
            vec3 x2 = x + k1x * dt * 0.5; vec3 k2v = -3.0 * h2 * x2 / pow(dot(x2, x2), 2.5), k2x = v + k1v * dt * 0.5;
            vec3 x3 = x + k2x * dt * 0.5; vec3 k3v = -3.0 * h2 * x3 / pow(dot(x3, x3), 2.5), k3x = v + k2v * dt * 0.5;
            vec3 x4 = x + k3x * dt;       vec3 k4v = -3.0 * h2 * x4 / pow(dot(x4, x4), 2.5), k4x = v + k3v * dt;
            x += (k1x + 2.0 * k2x + 2.0 * k3x + k4x) * dt / 6.0;
            v += (k1v + 2.0 * k2v + 2.0 * k3v + k4v) * dt / 6.0;
        }
        // Disk plane crossing (y = 0) between xp and x.
        if (xp.y * x.y < 0.0) {
            float f = xp.y / (xp.y - x.y);
            vec3 hit = mix(xp, x, f);
            vec4 e = diskSample(hit, -v);  // not normalised: |x × v| = b, so x × v is L/E
            col += trans * e.rgb * e.a;
            trans *= 1.0 - e.a;
            if (trans < 0.01) break;
        }
        if (uMode == 2 && length(x) < 2.0) { captured = true; break; }
    }
    if (!captured) col += trans * sky(normalize(v));
    o = vec4(col, 1.0);
}
)";

}  // namespace bh
