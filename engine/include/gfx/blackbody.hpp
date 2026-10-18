// Blackbody colour: Planck's law integrated against the CIE 1931 2° observer
// (analytic multi-lobe fit of Wyman, Sloan & Shirley, JCGT 2013), converted to
// linear sRGB (D65). Output is normalised to luminance Y = 1 so brightness can
// be applied separately (e.g. Stefan–Boltzmann T^4).
#pragma once
#include <vector>

#include "gfx/math.hpp"

namespace gfx {

// Linear-sRGB chromaticity of a blackbody at temperature T (kelvin), Y = 1.
// Negative components (out of gamut) are clamped to 0.
vec3 blackbodyRGB(float kelvin);

// A log-spaced lookup table from tMin..tMax, packed RGB floats, for upload as a
// 1-row texture. Index i maps to T = tMin * (tMax/tMin)^(i/(n-1)).
std::vector<float> blackbodyLUT(int n, float tMin, float tMax);

}  // namespace gfx
