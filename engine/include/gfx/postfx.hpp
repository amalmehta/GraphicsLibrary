// HDR post-processing: physically-based bloom (progressive downsample and
// tent-filtered upsample, after Jimenez, "Next Generation Post Processing in
// Call of Duty: Advanced Warfare", SIGGRAPH 2014) and ACES filmic tone mapping.
#pragma once
#include <vector>

#include "gfx/gl.hpp"

namespace gfx {

struct PostSettings {
    float exposureEV = 0, bloom = 0.1f, vignette = 0.25f;
};

class PostFX {
public:
    bool init();
    void resize(int w, int h);
    // Reads `hdr`, writes to the default framebuffer at outW x outH.
    void apply(GLuint hdr, int outW, int outH, const PostSettings& s, float time);

private:
    Shader down_, up_, composite_;
    FullscreenPass quad_;
    std::vector<RenderTarget> mips_;
    int w_ = 0, h_ = 0;
};

}  // namespace gfx
