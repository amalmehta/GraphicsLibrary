// Small header-only vector and matrix math. Column-major mat4, like GLSL.
#pragma once
#include <cmath>

namespace gfx {

constexpr float kPi = 3.14159265358979323846f;

struct vec2 { float x = 0, y = 0; };

struct vec3 {
    float x = 0, y = 0, z = 0;
    constexpr vec3() = default;
    constexpr vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    vec3 operator+(vec3 b) const { return {x + b.x, y + b.y, z + b.z}; }
    vec3 operator-(vec3 b) const { return {x - b.x, y - b.y, z - b.z}; }
    vec3 operator-() const { return {-x, -y, -z}; }
    vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
    vec3& operator+=(vec3 b) { x += b.x; y += b.y; z += b.z; return *this; }
    vec3& operator-=(vec3 b) { x -= b.x; y -= b.y; z -= b.z; return *this; }
    vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
};
inline vec3 operator*(float s, vec3 v) { return v * s; }

inline float dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline vec3 cross(vec3 a, vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline float length(vec3 v) { return std::sqrt(dot(v, v)); }
inline vec3 normalize(vec3 v) { float l = length(v); return l > 0 ? v / l : v; }

struct vec4 { float x = 0, y = 0, z = 0, w = 0; };

struct mat4 {
    float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    float& at(int col, int row) { return m[col * 4 + row]; }
    float at(int col, int row) const { return m[col * 4 + row]; }

    mat4 operator*(const mat4& b) const {
        mat4 r;
        for (int c = 0; c < 4; ++c)
            for (int rI = 0; rI < 4; ++rI) {
                float s = 0;
                for (int k = 0; k < 4; ++k) s += at(k, rI) * b.at(c, k);
                r.at(c, rI) = s;
            }
        return r;
    }
    vec4 operator*(vec4 v) const {
        return {at(0, 0) * v.x + at(1, 0) * v.y + at(2, 0) * v.z + at(3, 0) * v.w,
                at(0, 1) * v.x + at(1, 1) * v.y + at(2, 1) * v.z + at(3, 1) * v.w,
                at(0, 2) * v.x + at(1, 2) * v.y + at(2, 2) * v.z + at(3, 2) * v.w,
                at(0, 3) * v.x + at(1, 3) * v.y + at(2, 3) * v.z + at(3, 3) * v.w};
    }

    static mat4 perspective(float fovyRad, float aspect, float zNear, float zFar) {
        mat4 r;
        float f = 1.0f / std::tan(fovyRad * 0.5f);
        r.m[0] = f / aspect; r.m[5] = f;
        r.m[10] = (zFar + zNear) / (zNear - zFar);
        r.m[11] = -1;
        r.m[14] = 2 * zFar * zNear / (zNear - zFar);
        r.m[15] = 0;
        return r;
    }
    static mat4 lookAt(vec3 eye, vec3 target, vec3 up) {
        vec3 f = normalize(target - eye), s = normalize(cross(f, up)), u = cross(s, f);
        mat4 r;
        r.at(0, 0) = s.x; r.at(1, 0) = s.y; r.at(2, 0) = s.z;
        r.at(0, 1) = u.x; r.at(1, 1) = u.y; r.at(2, 1) = u.z;
        r.at(0, 2) = -f.x; r.at(1, 2) = -f.y; r.at(2, 2) = -f.z;
        r.at(3, 0) = -dot(s, eye); r.at(3, 1) = -dot(u, eye); r.at(3, 2) = dot(f, eye);
        return r;
    }
};

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float mixf(float a, float b, float t) { return a + (b - a) * t; }

}  // namespace gfx
