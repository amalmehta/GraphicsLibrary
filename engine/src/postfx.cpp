#include "gfx/postfx.hpp"

#include <cmath>

namespace gfx {

namespace {
const char* kDownFS = R"(
in vec2 vUv;
out vec4 o;
uniform sampler2D uSrc;
uniform vec2 uTexel;   // 1 / source size
uniform int uFirst;    // first pass: Karis average to tame fireflies
vec3 s(vec2 off) { return texture(uSrc, vUv + off * uTexel).rgb; }
float luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }
void main() {
    vec3 a = s(vec2(-2, 2)), b = s(vec2(0, 2)), c = s(vec2(2, 2));
    vec3 d = s(vec2(-2, 0)), e = s(vec2(0, 0)), f = s(vec2(2, 0));
    vec3 g = s(vec2(-2, -2)), h = s(vec2(0, -2)), i = s(vec2(2, -2));
    vec3 j = s(vec2(-1, 1)), k = s(vec2(1, 1)), l = s(vec2(-1, -1)), m = s(vec2(1, -1));
    vec3 r;
    if (uFirst == 1) {
        // Weighted by 1/(1+luma) per 2x2 group (Karis 2014).
        vec3 g0 = (a + b + d + e) * 0.25, g1 = (b + c + e + f) * 0.25;
        vec3 g2 = (d + e + g + h) * 0.25, g3 = (e + f + h + i) * 0.25;
        vec3 g4 = (j + k + l + m) * 0.25;
        float w0 = 1.0 / (1.0 + luma(g0)), w1 = 1.0 / (1.0 + luma(g1));
        float w2 = 1.0 / (1.0 + luma(g2)), w3 = 1.0 / (1.0 + luma(g3));
        float w4 = 1.0 / (1.0 + luma(g4));
        r = (g0 * w0 * 0.125 + g1 * w1 * 0.125 + g2 * w2 * 0.125 + g3 * w3 * 0.125 + g4 * w4 * 0.5) /
            (w0 * 0.125 + w1 * 0.125 + w2 * 0.125 + w3 * 0.125 + w4 * 0.5);
    } else {
        r = e * 0.125 + (a + c + g + i) * 0.03125 + (b + d + f + h) * 0.0625 + (j + k + l + m) * 0.125;
    }
    o = vec4(max(r, 0.0), 1.0);
}
)";

const char* kUpFS = R"(
in vec2 vUv;
out vec4 o;
uniform sampler2D uSrc;
uniform vec2 uTexel;
void main() {
    vec3 r = texture(uSrc, vUv).rgb * 4.0;
    r += (texture(uSrc, vUv + vec2(-1, 0) * uTexel).rgb + texture(uSrc, vUv + vec2(1, 0) * uTexel).rgb +
          texture(uSrc, vUv + vec2(0, -1) * uTexel).rgb + texture(uSrc, vUv + vec2(0, 1) * uTexel).rgb) * 2.0;
    r += texture(uSrc, vUv + vec2(-1, -1) * uTexel).rgb + texture(uSrc, vUv + vec2(1, -1) * uTexel).rgb +
         texture(uSrc, vUv + vec2(-1, 1) * uTexel).rgb + texture(uSrc, vUv + vec2(1, 1) * uTexel).rgb;
    o = vec4(r / 16.0, 1.0);
}
)";

const char* kCompositeFS = R"(
in vec2 vUv;
out vec4 o;
uniform sampler2D uHdr, uBloom;
uniform float uExposure, uBloomAmt, uBloomNorm, uVignette, uTime;
// ACES filmic curve fit, Krzysztof Narkowicz 2015.
vec3 aces(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}
vec3 toSrgb(vec3 c) {
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));
}
float hash(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233)) + uTime) * 43758.5453); }
void main() {
    vec3 hdr = texture(uHdr, vUv).rgb;
    vec3 bloom = texture(uBloom, vUv).rgb * uBloomNorm;
    vec3 c = mix(hdr, bloom, uBloomAmt) * uExposure;
    vec2 q = vUv - 0.5;
    c *= 1.0 - uVignette * dot(q, q) * 1.6;
    c = toSrgb(aces(c));
    c += (hash(gl_FragCoord.xy) - 0.5) / 255.0;  // dither out banding
    o = vec4(c, 1.0);
}
)";
}  // namespace

bool PostFX::init() {
    quad_.init();
    return down_.build(kFullscreenVS, kDownFS, "bloom-down") &&
           up_.build(kFullscreenVS, kUpFS, "bloom-up") &&
           composite_.build(kFullscreenVS, kCompositeFS, "composite");
}

void PostFX::resize(int w, int h) {
    if (w == w_ && h == h_) return;
    w_ = w; h_ = h;
    mips_.clear();
    int mw = w / 2, mh = h / 2;
    for (int i = 0; i < 7 && mw >= 4 && mh >= 4; ++i, mw /= 2, mh /= 2) {
        TextureDesc d;
        d.width = mw; d.height = mh;
        d.internalFormat = GL_RGBA16F; d.format = GL_RGBA; d.type = GL_HALF_FLOAT;
        RenderTarget rt;
        rt.create({d}, false);
        mips_.push_back(std::move(rt));
    }
}

void PostFX::apply(GLuint hdr, int outW, int outH, const PostSettings& s, float time) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    if (!mips_.empty()) {
        down_.use();
        GLuint src = hdr;
        int sw = w_, sh = h_;
        for (size_t i = 0; i < mips_.size(); ++i) {
            mips_[i].bind();
            down_.tex("uSrc", src, 0);
            down_.set("uTexel", vec2{1.f / sw, 1.f / sh});
            down_.set("uFirst", i == 0 ? 1 : 0);
            quad_.draw();
            src = mips_[i].color();
            sw = mips_[i].width(); sh = mips_[i].height();
        }
        up_.use();
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        for (int i = int(mips_.size()) - 1; i > 0; --i) {
            mips_[i - 1].bind();
            up_.tex("uSrc", mips_[i].color(), 0);
            up_.set("uTexel", vec2{1.f / mips_[i].width(), 1.f / mips_[i].height()});
            quad_.draw();
        }
        glDisable(GL_BLEND);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, outW, outH);
    composite_.use();
    composite_.tex("uHdr", hdr, 0);
    composite_.tex("uBloom", mips_.empty() ? hdr : mips_[0].color(), 1);
    composite_.set("uExposure", std::pow(2.f, s.exposureEV));
    composite_.set("uBloomAmt", s.bloom);
    // Each upsample adds one level into mip 0, so normalise by the chain length.
    composite_.set("uBloomNorm", mips_.empty() ? 1.f : 1.f / float(mips_.size()));
    composite_.set("uVignette", s.vignette);
    composite_.set("uTime", std::fmod(time, 100.f));
    quad_.draw();
}

}  // namespace gfx
