// GPU shaders for the waterfall: particle update (state lives in float
// textures), particle drawing, and the ray-marched cliff, rocks and pool.
// The rock field and integrator mirror WaterfallPhysics.hpp.
#pragma once

namespace wf {

// Shared GLSL: rock field and hashing.
inline const char* kCommonGLSL = R"(
float sdBox(vec3 p, vec3 b) {
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}
float rockSdf(vec3 p) {
    float cliff = sdBox(p - vec3(0, 5, -10), vec3(40, 7, 10));
    float ledge = sdBox(p - vec3(-0.6, 5, 1.0), vec3(1.4, 0.3, 1.0)) - 0.2;
    float b1 = length(p - vec3(1.8, -0.2, 3.0)) - 1.0;
    float b2 = length(p - vec3(-1.5, -0.4, 4.2)) - 1.2;
    float b3 = length(p - vec3(0.2, -0.6, 5.5)) - 0.9;
    return min(min(cliff, ledge), min(b1, min(b2, b3)));
}
vec3 rockNormal(vec3 p) {
    const vec2 e = vec2(1e-3, 0);
    return normalize(vec3(rockSdf(p + e.xyy) - rockSdf(p - e.xyy),
                          rockSdf(p + e.yxy) - rockSdf(p - e.yxy),
                          rockSdf(p + e.yyx) - rockSdf(p - e.yyx)));
}
uint pcg(uint v) {
    uint s = v * 747796405u + 2891336453u;
    uint w = ((s >> ((s >> 28u) + 4u)) ^ s) * 277803737u;
    return (w >> 22u) ^ w;
}
float rnd(inout uint s) { s = pcg(s); return float(s) * (1.0 / 4294967295.0); }
)";

// Particle state: pos.xyz + life (s), vel.xyz + type (0 water, 1 spray, 2 mist).
// Parked (not yet emitted) particles sit at y = -100 with life counting up to 0.
inline const char* kUpdateFS = R"(
uniform sampler2D uPos, uVel;
uniform ivec2 uSize;
uniform uint uFrame;
uniform float uDt, uFlow, uLipSpeed, uDragWater, uSplash, uMist, uWind;
layout(location = 0) out vec4 oPos;
layout(location = 1) out vec4 oVel;

const float G = 9.81;
const float LIP_Y = 12.0, HALF_W = 2.4;

void spawnWater(inout vec3 p, inout vec3 v, inout float life, inout float type, inout uint s) {
    float u = rnd(s) * 2.0 - 1.0;
    // Slightly thinner flow toward the edges of the channel.
    float x = sign(u) * pow(abs(u), 0.8) * HALF_W;
    p = vec3(x, LIP_Y + 0.02 + rnd(s) * 0.06, -0.35 + rnd(s) * 0.3);
    v = vec3((rnd(s) - 0.5) * 0.12, (rnd(s) - 0.5) * 0.1, uLipSpeed * (0.82 + 0.36 * rnd(s)));
    life = 9.0;
    type = 0.0;
}

void main() {
    ivec2 c = ivec2(gl_FragCoord.xy);
    vec4 P = texelFetch(uPos, c, 0), V = texelFetch(uVel, c, 0);
    vec3 p = P.xyz, v = V.xyz;
    float life = P.w, type = V.w;
    int idx = c.x + c.y * uSize.x;
    uint s = pcg(uint(idx) ^ pcg(uFrame * 9781u + 1u));
    bool inUse = float(idx) / float(uSize.x * uSize.y) < uFlow;

    if (!inUse) {
        oPos = vec4(0, -100, 0, -rnd(s) * 1.5);
        oVel = vec4(0);
        return;
    }
    if (p.y < -50.0) {  // parked: wait, then emit
        life += uDt;
        if (life >= 0.0) spawnWater(p, v, life, type, s);
        oPos = vec4(p, life); oVel = vec4(v, type);
        return;
    }

    // Drag constant per type: k = g / v_terminal^2.
    float k = type < 0.5 ? uDragWater : (type < 1.5 ? uDragWater * 2.5 : G / (0.6 * 0.6));
    float grav = type < 1.5 ? 1.0 : 0.012;
    vec3 wind = vec3(uWind * (type < 0.5 ? 0.25 : 1.0), 0, 0);
    if (type > 1.5) {
        // Mist: gentle buoyant swirl (artistic turbulence).
        wind += vec3(sin(p.y * 1.7 + p.z * 0.9 + float(uFrame) * 0.01),
                     0.25 + 0.2 * sin(p.x * 1.3 + float(uFrame) * 0.013),
                     cos(p.x * 1.1 + p.y * 0.7)) * 0.5;
    }
    vec3 rel = v - wind;
    v += (vec3(0, -G * grav, 0) - rel * (k * length(rel))) * uDt;
    p += v * uDt;
    life -= uDt;

    // Rock collision.
    float d = rockSdf(p);
    if (d < 0.0) {
        vec3 n = rockNormal(p);
        p -= n * d;
        float vn = dot(v, n);
        if (vn < 0.0) {
            vec3 vt = v - n * vn;
            float speed = length(v);
            // Friction scales with impact speed so sliding water keeps flowing.
            v = vt * (1.0 - 0.3 * clamp((-vn - 0.5) / 1.5, 0.0, 1.0)) - n * vn * 0.15;
            if (type < 0.5 && speed > 2.5 && rnd(s) < uSplash) {
                // Break into spray: scatter into the hemisphere around the normal.
                vec3 r = normalize(vec3(rnd(s), rnd(s), rnd(s)) * 2.0 - 1.0 + n * 1.2);
                v = vt * 0.4 + r * speed * (0.15 + 0.3 * rnd(s));
                type = 1.0;
                life = 0.6 + rnd(s) * 1.2;
            }
        }
    }

    // Plunge pool surface.
    if (p.y < 0.0 && type < 1.5) {
        float speed = length(v);
        float r = rnd(s);
        if (r < uMist) {
            type = 2.0;
            p.y = 0.05 + rnd(s) * 0.3;
            v = vec3((rnd(s) - 0.5) * 2.5, 0.4 + rnd(s) * 1.4, 0.3 + (rnd(s) - 0.3) * 2.0);
            life = 2.5 + rnd(s) * 3.5;
        } else if (r < uMist + uSplash * 0.35 && type < 0.5) {
            type = 1.0;
            p.y = 0.02;
            v = vec3((rnd(s) - 0.5) * 2.0, (0.2 + rnd(s) * 0.5) * speed * 0.35, (rnd(s) - 0.3) * 2.0);
            life = 0.5 + rnd(s) * 0.8;
        } else {
            life = 0.0;
        }
    }

    if (life <= 0.0 || p.y < -1.0 || abs(p.x) > 30.0 || p.z > 30.0) spawnWater(p, v, life, type, s);
    oPos = vec4(p, life);
    oVel = vec4(v, type);
}
)";

inline const char* kParticleVS = R"(
uniform sampler2D uPos, uVel;
uniform ivec2 uSize;
uniform mat4 uView, uProj;
uniform float uViewportH, uDropSize, uMistSize, uMistAmount;
uniform vec3 uSunColor, uSkyColor, uSunDir;
out vec4 vColor;
flat out float vType;

void main() {
    ivec2 c = ivec2(gl_VertexID % uSize.x, gl_VertexID / uSize.x);
    vec4 P = texelFetch(uPos, c, 0), V = texelFetch(uVel, c, 0);
    if (P.y < -50.0) { gl_Position = vec4(2, 2, 2, 1); gl_PointSize = 0.0; return; }
    float type = V.w;
    vec4 vp = uView * vec4(P.xyz, 1);
    gl_Position = uProj * vp;
    float speed = length(V.xyz);

    float size, alpha;
    vec3 col;
    if (type < 0.5) {
        size = uDropSize;
        alpha = 0.22;
        // Faster water is more aerated, so it scatters more and looks whiter.
        float foam = smoothstep(3.0, 12.0, speed);
        col = mix(vec3(0.42, 0.56, 0.62), vec3(0.92, 0.95, 0.97), foam);
    } else if (type < 1.5) {
        size = uDropSize * 0.7;
        alpha = 0.28 * smoothstep(0.0, 0.3, P.w);
        col = vec3(0.9, 0.94, 0.97);
    } else {
        float age = 1.0 - clamp(P.w / 6.0, 0.0, 1.0);
        size = uMistSize * (0.6 + 2.2 * age);
        alpha = 0.006 * uMistAmount * smoothstep(0.0, 1.2, P.w) * smoothstep(0.0, 0.15, age);
        col = vec3(0.92, 0.95, 0.98);
    }
    // Light: sun with a forward-scattering lobe plus sky fill.
    vec3 view = normalize(-vp.xyz);
    vec3 sunV = normalize((uView * vec4(uSunDir, 0)).xyz);
    float fwd = pow(max(dot(-view, sunV), 0.0), 6.0);
    col *= uSunColor * (0.55 + 1.4 * fwd) + uSkyColor * 0.8;

    float px = size * uProj[1][1] * uViewportH * 0.5 / max(-vp.z, 0.05);
    // Sub-pixel points: keep coverage by trading size for alpha.
    if (px < 1.5) { alpha *= px * px / 2.25; px = 1.5; }
    gl_PointSize = min(px, 128.0);
    vColor = vec4(col * alpha, alpha);
    vType = type;
}
)";

inline const char* kParticleFS = R"(
in vec4 vColor;
flat in float vType;
out vec4 o;
void main() {
    vec2 d = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(d, d);
    if (r2 > 1.0) discard;
    float fall = vType > 1.5 ? (1.0 - r2) * (1.0 - r2) : exp(-r2 * 3.5);
    o = vColor * fall;
}
)";

// Ray-marched environment: cliff, ledge, boulders, plunge pool and sky.
// Writes gl_FragDepth so particles depth-test against it.
inline const char* kEnvFS = R"(
in vec2 vUv;
out vec4 o;
uniform vec3 uEye, uFwd, uRight, uUp, uSunDir, uSunColor, uSkyColor;
uniform float uTanHalfFov, uAspect, uTime;
uniform mat4 uViewProj;
uniform int uSteps;

float hash13(vec3 p) {
    p = fract(p * 0.1031);
    p += dot(p, p.zyx + 31.32);
    return fract((p.x + p.y) * p.z);
}
float vnoise(vec3 p) {
    vec3 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash13(i), hash13(i + vec3(1,0,0)), f.x),
                   mix(hash13(i + vec3(0,1,0)), hash13(i + vec3(1,1,0)), f.x), f.y),
               mix(mix(hash13(i + vec3(0,0,1)), hash13(i + vec3(1,0,1)), f.x),
                   mix(hash13(i + vec3(0,1,1)), hash13(i + vec3(1,1,1)), f.x), f.y), f.z);
}
float fbm(vec3 p) {
    float s = 0.0, a = 0.5;
    for (int i = 0; i < 4; ++i) { s += a * vnoise(p); p = p * 2.07 + 5.3; a *= 0.5; }
    return s;
}

// Rendered rock = collision rock + small surface relief (≤ ~0.15 m).
float map(vec3 p) {
    float d = rockSdf(p);
    if (d < 0.6) d += (vnoise(p * 1.3) - 0.5) * 0.22 + (vnoise(p * 4.1) - 0.5) * 0.06;
    return d;
}
vec3 mapNormal(vec3 p) {
    const vec2 e = vec2(0.01, 0);
    return normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy),
                          map(p + e.yyx) - map(p - e.yyx)));
}
float march(vec3 ro, vec3 rd, float tMax, int steps) {
    float t = 0.05;
    for (int i = 0; i < 200; ++i) {
        if (i >= steps) break;
        float d = map(ro + rd * t);
        if (d < 0.002 * t) return t;
        t += d * 0.85;
        if (t > tMax) break;
    }
    return -1.0;
}
float softShadow(vec3 ro, vec3 rd) {
    float res = 1.0, t = 0.08;
    for (int i = 0; i < 28; ++i) {
        float h = map(ro + rd * t);
        res = min(res, 10.0 * h / t);
        t += clamp(h, 0.05, 1.0);
        if (res < 0.01 || t > 25.0) break;
    }
    return clamp(res, 0.0, 1.0);
}
float ao(vec3 p, vec3 n) {
    float occ = 0.0, w = 1.0;
    for (int i = 1; i <= 4; ++i) {
        float h = 0.12 * float(i);
        occ += (h - map(p + n * h)) * w;
        w *= 0.6;
    }
    return clamp(1.0 - 2.0 * occ, 0.0, 1.0);
}
vec3 sky(vec3 rd) {
    float up = clamp(rd.y, 0.0, 1.0);
    vec3 horizon = vec3(0.86, 0.82, 0.76), zenith = vec3(0.38, 0.56, 0.82);
    vec3 c = mix(horizon, zenith, pow(up, 0.55)) * 0.75;
    float sd = max(dot(rd, uSunDir), 0.0);
    c += uSunColor * (pow(sd, 900.0) * 30.0 + pow(sd, 12.0) * 0.18);
    return c;
}
vec3 shadeRock(vec3 p, vec3 rd) {
    vec3 n = mapNormal(p);
    float grain = fbm(p * 2.3);
    vec3 alb = mix(vec3(0.07, 0.065, 0.06), vec3(0.2, 0.18, 0.155), grain * grain * 1.6);
    // Moss on upward-facing surfaces away from the falls: one sparing accent.
    float moss = smoothstep(0.55, 0.85, n.y) * smoothstep(0.45, 0.65, fbm(p * 0.9 + 3.0));
    alb = mix(alb, vec3(0.13, 0.19, 0.07), moss * 0.8);
    // Wet rock near the falls is darker and glossier.
    float wet = (1.0 - smoothstep(2.6, 4.2, abs(p.x))) * (1.0 - smoothstep(0.4, 3.5, p.z + max(p.y - 11.5, 0.0) * 3.0));
    wet = max(wet, 1.0 - smoothstep(0.0, 0.6, p.y));
    alb *= 1.0 - 0.45 * wet;
    float sh = softShadow(p + n * 0.02, uSunDir);
    float occ = ao(p, n);
    float dif = max(dot(n, uSunDir), 0.0);
    vec3 h = normalize(uSunDir - rd);
    float spec = pow(max(dot(n, h), 0.0), mix(16.0, 90.0, wet)) * mix(0.03, 0.5, wet);
    vec3 col = alb * (uSunColor * dif * sh + uSkyColor * (0.5 + 0.5 * n.y) * occ * 0.9);
    col += uSunColor * spec * sh;
    // River on the cliff top.
    if (p.y > 11.9 && abs(p.x) < 2.55 && p.z > -12.0) {
        vec3 w = mix(vec3(0.05, 0.1, 0.11), sky(reflect(rd, vec3(0, 1, 0))), 0.25);
        col = mix(col, w + uSunColor * spec, 0.85);
    }
    return col;
}

void main() {
    vec2 ndc = vUv * 2.0 - 1.0;
    vec3 rd = normalize(uFwd + uRight * ndc.x * uTanHalfFov * uAspect + uUp * ndc.y * uTanHalfFov);
    vec3 ro = uEye;

    float tRock = march(ro, rd, 150.0, uSteps);
    float tPool = rd.y < 0.0 ? -ro.y / rd.y : -1.0;
    vec3 col;
    float tHit = -1.0;

    if (tPool > 0.0 && (tRock < 0.0 || tPool < tRock)) {
        tHit = tPool;
        vec3 p = ro + rd * tPool;
        // Ripples, stronger in the plunge zone.
        float plunge = exp(-pow(length((p.xz - vec2(0.0, 3.0)) / vec2(3.2, 2.2)), 2.0));
        vec2 q = p.xz * 1.7 + vec2(0.0, uTime * 0.6);
        float amp = 0.04 + 0.25 * plunge;
        vec3 n = normalize(vec3((vnoise(vec3(q, uTime * 1.3)) - 0.5) * amp, 1.0,
                                (vnoise(vec3(q.yx + 7.0, uTime * 1.1)) - 0.5) * amp));
        vec3 rr = reflect(rd, n);
        float fres = 0.02 + 0.98 * pow(1.0 - max(dot(-rd, n), 0.0), 5.0);
        float tr = march(p + n * 0.05, rr, 60.0, uSteps / 2);
        vec3 refl = tr > 0.0 ? shadeRock(p + rr * tr, rr) * 0.9 : sky(rr);
        vec3 body = vec3(0.025, 0.07, 0.075) * (uSkyColor * 0.9 + uSunColor * 0.35 * max(uSunDir.y, 0.0));
        col = mix(body, refl, fres);
        float foam = plunge * smoothstep(0.35, 0.75, vnoise(vec3(p.xz * 2.5, uTime * 2.0)) * 0.6 + plunge * 0.5);
        col = mix(col, vec3(0.85, 0.88, 0.9) * (uSunColor * 0.6 + uSkyColor * 0.7), foam * 0.8);
    } else if (tRock > 0.0) {
        tHit = tRock;
        col = shadeRock(ro + rd * tRock, rd);
    } else {
        col = sky(rd);
    }

    if (tHit > 0.0) {
        vec3 p = ro + rd * tHit;
        float fog = 1.0 - exp(-tHit * 0.0025);
        col = mix(col, sky(normalize(vec3(rd.x, 0.05, rd.z))) * 0.95, fog);
        vec4 clip = uViewProj * vec4(p, 1.0);
        gl_FragDepth = clamp(clip.z / clip.w * 0.5 + 0.5, 0.0, 1.0);
    } else {
        gl_FragDepth = 1.0;
    }
    o = vec4(col, 1.0);
}
)";

}  // namespace wf
