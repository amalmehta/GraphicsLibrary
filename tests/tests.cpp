// Native tests for the GL-free parts: math, parameters, blackbody colour and the
// physics of both simulations, checked against published values.
#include <cmath>
#include <cstdio>
#include <string>

#include "../sims/blackhole/BlackHolePhysics.hpp"
#include "../sims/waterfall/WaterfallPhysics.hpp"
#include "gfx/blackbody.hpp"
#include "gfx/math.hpp"
#include "gfx/params.hpp"

static int gFailed = 0, gPassed = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (cond) ++gPassed;                                                   \
        else { ++gFailed; std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, msg); } \
    } while (0)

static bool near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

static void testMath() {
    using namespace gfx;
    mat4 v = mat4::lookAt({0, 0, 5}, {0, 0, 0}, {0, 1, 0});
    vec4 p = v * vec4{0, 0, 0, 1};
    CHECK(near(p.z, -5, 1e-5), "lookAt puts the target 5 units down -z");
    mat4 pr = mat4::perspective(kPi / 2, 1, 1, 100);
    vec4 c = pr * vec4{0, 0, -1, 1};
    CHECK(near(c.z / c.w, -1, 1e-5), "near plane maps to NDC z = -1");
    CHECK(near(length(cross(vec3(1, 0, 0), vec3(0, 1, 0)) - vec3(0, 0, 1)), 0, 1e-6), "x × y = z");
}

static void testParams() {
    gfx::ParamSet ps;
    ps.group("Disk");
    ps.slider("temp", "Peak temperature", 9000, 2000, 40000, 100, "K");
    ps.toggle("beam", "Beaming", true);
    ps.choice("mode", "Physics", {"Full", "Lensing only", "Newtonian"}, 0);
    CHECK(ps.set("temp", 1e9f) && ps.get("temp") == 40000, "slider clamps to max");
    CHECK(ps.set("beam", 0.2f) && !ps.flag("beam"), "toggle snaps to off");
    CHECK(ps.set("mode", 1.6f) && ps.get("mode") == 2, "choice rounds to index");
    CHECK(!ps.set("nope", 1), "unknown id rejected");
    CHECK(!ps.set("temp", NAN), "NaN rejected");
    ps.resetAll();
    CHECK(ps.get("temp") == 9000 && ps.flag("beam"), "reset restores defaults");
    std::string j = ps.toJson();
    CHECK(j.find("\"group\":\"Disk\"") != std::string::npos, "json carries group");
    CHECK(j.find("\"options\":[\"Full\",\"Lensing only\",\"Newtonian\"]") != std::string::npos,
          "json carries options");
}

static void testBlackbody() {
    using gfx::blackbodyRGB;
    gfx::vec3 d65 = blackbodyRGB(6504);
    CHECK(near(d65.x, d65.z, 0.12) && near(d65.y, 1.0, 0.1), "~6500 K is close to white");
    gfx::vec3 warm = blackbodyRGB(3000), hot = blackbodyRGB(20000);
    CHECK(warm.x > warm.y && warm.y > warm.z, "3000 K is orange (r > g > b)");
    CHECK(hot.z > hot.x, "20000 K is blue-white (b > r)");
    CHECK(gfx::blackbodyLUT(64, 1000, 40000).size() == 64 * 3, "LUT has n RGB entries");
}

static void testBlackHole() {
    using namespace bh;
    using gfx::vec3;
    // Photon capture: impact parameter just below 3√3 M falls in, just above escapes.
    float d = 200;
    auto shoot = [&](float b) { return trace({b, 0, d}, {0, 0, -1}, 0.01f, 200000, d * 1.5f); };
    CHECK(shoot(kCriticalImpact - 0.05f).captured, "b = 3√3 M − 0.05 is captured");
    CHECK(!shoot(kCriticalImpact + 0.05f).captured, "b = 3√3 M + 0.05 escapes");

    // Weak-field deflection: α ≈ 4M/b + 15π M²/(4 b²) (Einstein 1915; second-order
    // term from e.g. Keeton & Petters 2005).
    float b = 100;
    TraceResult t = trace({b, 0, -4000}, {0, 0, 1}, 0.005f, 400000, 8000);
    float alpha = std::acos(gfx::clampf(gfx::dot(t.direction, vec3(0, 0, 1)), -1, 1));
    // Starting 4000 M away misses only ~(2M/b)(b/D)^2/2 of the bending: negligible.
    float expected = 4 / b + 15 * gfx::kPi / (4 * b * b);
    CHECK(!t.captured && near(alpha, expected, expected * 0.02),
          "weak-field deflection matches 4M/b to 2%");

    // A photon launched tangentially at r = 3M stays near the photon sphere for a while.
    vec3 x{3, 0, 0}, v{0, 0, 1};
    float h2 = gfx::dot(gfx::cross(x, v), gfx::cross(x, v));
    for (int i = 0; i < 2000; ++i) rk4(x, v, h2, 0.001f);
    CHECK(near(gfx::length(x), 3, 1e-3), "circular photon orbit at r = 3M");

    CHECK(near(diskFluxPeakRadius(6), 6 * 49.0 / 36.0, 1e-5), "flux peaks at 49/36 r_in");
    float rp = diskFluxPeakRadius(6);
    CHECK(diskFlux(rp, 6) > diskFlux(rp * 0.97f, 6) && diskFlux(rp, 6) > diskFlux(rp * 1.03f, 6),
          "flux maximum is at the peak radius");
    CHECK(near(diskTemperature(rp, 6, 9000), 9000, 1), "peak temperature at peak radius");
    CHECK(diskTemperature(5, 6, 9000) == 0, "no emission inside r_in");

    // Redshift: a photon emitted perpendicular to the gas motion (λ = 0) at the
    // ISCO has g = sqrt(1 − 3M/r) = 1/√2.
    CHECK(near(redshiftFactor(6, 0), std::sqrt(0.5), 1e-5), "transverse redshift at ISCO");
    // Approaching side is blueshifted, receding side redshifted.
    CHECK(redshiftFactor(6, 5) > 1.f, "approaching gas blueshifted");
    CHECK(redshiftFactor(6, -5) < redshiftFactor(6, 0), "receding gas redshifted");
}

static void testWaterfall() {
    using namespace wf;
    using gfx::vec3;
    // Free fall with no drag: t = sqrt(2h/g).
    Drop p{{0, 12, 0}, {0, 0, 0}};
    float t = 0, dt = 1.f / 240;
    while (p.pos.y > 0 && t < 10) { step(p, dt, 0, {0, 0, 0}); t += dt; }
    CHECK(near(t, std::sqrt(2 * 12 / kGravity), 0.01), "free-fall time matches sqrt(2h/g)");

    // With quadratic drag the speed approaches terminal velocity sqrt(g/k).
    float vt = 9.0f;  // ~2 mm raindrop (Gunn & Kinzer 1949)
    float k = dragForTerminal(vt);
    Drop q{{0, 1000, 0}, {0, 0, 0}};
    for (int i = 0; i < 240 * 20; ++i) step(q, dt, k, {0, 0, 0});
    CHECK(near(-q.vel.y, vt, 0.05), "terminal velocity approached");

    // Collision pushes particles out of the rock field and kills inward velocity.
    Drop c{{1.8f, 0.7f, 3}, {0, -5, 0}};  // inside the first boulder
    bool hit = collide(c, 0.2f, 0.5f);
    CHECK(hit && sceneSdf(c.pos) >= -1e-3f && c.vel.y > -0.5f, "collision resolves penetration");

    // Resting contact keeps sliding: a slow tangential flow along the cliff top isn't stopped.
    Drop s{{0, 11.99f, -2}, {0, -0.05f, 1.5f}};
    for (int i = 0; i < 12; ++i) { step(s, dt, 0, {0, 0, 0}); collide(s, 0.15f, 0.3f); }
    CHECK(s.vel.z > 1.4f, "sliding water keeps its speed along a surface");
}

int main() {
    testMath();
    testParams();
    testBlackbody();
    testBlackHole();
    testWaterfall();
    std::printf("%d passed, %d failed\n", gPassed, gFailed);
    return gFailed ? 1 : 0;
}
