#include "gfx/blackbody.hpp"

#include <cmath>

namespace gfx {

namespace {
double lobe(double x, double mu, double s1, double s2) {
    double t = (x - mu) / (x < mu ? s1 : s2);
    return std::exp(-0.5 * t * t);
}
// CIE 1931 colour-matching functions, Wyman et al. 2013 (wavelength in nm).
double xBar(double l) { return 1.056 * lobe(l, 599.8, 37.9, 31.0) + 0.362 * lobe(l, 442.0, 16.0, 26.7) - 0.065 * lobe(l, 501.1, 20.4, 26.2); }
double yBar(double l) { return 0.821 * lobe(l, 568.8, 46.9, 40.5) + 0.286 * lobe(l, 530.9, 16.3, 31.1); }
double zBar(double l) { return 1.217 * lobe(l, 437.0, 11.8, 36.0) + 0.681 * lobe(l, 459.0, 26.0, 13.8); }

// Planck spectral radiance, arbitrary constant factor (it cancels on normalising).
double planck(double nm, double T) {
    const double c2 = 1.438776877e-2;  // second radiation constant hc/k, m·K
    double l = nm * 1e-9;
    return 1.0 / (std::pow(l, 5) * (std::exp(c2 / (l * T)) - 1.0));
}
}  // namespace

vec3 blackbodyRGB(float kelvin) {
    double T = kelvin < 100 ? 100 : kelvin, X = 0, Y = 0, Z = 0;
    for (double l = 380; l <= 780; l += 2) {
        double b = planck(l, T);
        X += b * xBar(l); Y += b * yBar(l); Z += b * zBar(l);
    }
    X /= Y; Z /= Y; Y = 1;
    double r = 3.2406 * X - 1.5372 * Y - 0.4986 * Z;
    double g = -0.9689 * X + 1.8758 * Y + 0.0415 * Z;
    double b = 0.0557 * X - 0.2040 * Y + 1.0570 * Z;
    return {float(r > 0 ? r : 0), float(g > 0 ? g : 0), float(b > 0 ? b : 0)};
}

std::vector<float> blackbodyLUT(int n, float tMin, float tMax) {
    std::vector<float> out;
    out.reserve(n * 3);
    for (int i = 0; i < n; ++i) {
        float t = tMin * std::pow(tMax / tMin, float(i) / float(n - 1));
        vec3 c = blackbodyRGB(t);
        out.push_back(c.x); out.push_back(c.y); out.push_back(c.z);
    }
    return out;
}

}  // namespace gfx
