// Thin RAII wrappers over OpenGL ES 3.0 / WebGL2: shaders, textures, render
// targets and a fullscreen pass. Move-only; each owns its GL object.
#pragma once
#include <GLES3/gl3.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "gfx/math.hpp"

namespace gfx {

// Last shader compile/link or framebuffer error, for the UI to show.
const std::string& lastGlError();
void setGlError(const std::string& msg);

// Every fragment/vertex source gets this prepended (version, precision).
extern const char* kGlslHeader;

// A vertex shader that draws one oversized triangle covering the screen and
// passes uv in [0,1]. Pair with FullscreenPass::draw().
extern const char* kFullscreenVS;

class Shader {
public:
    Shader() = default;
    // Sources are given without the #version line; kGlslHeader is prepended.
    bool build(const std::string& vs, const std::string& fs, const std::string& name);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& o) noexcept { *this = std::move(o); }
    Shader& operator=(Shader&& o) noexcept;

    void use() const { glUseProgram(prog_); }
    bool ok() const { return prog_ != 0; }
    GLint loc(const char* name);

    void set(const char* n, float v) { glUniform1f(loc(n), v); }
    void set(const char* n, int v) { glUniform1i(loc(n), v); }
    void set(const char* n, vec2 v) { glUniform2f(loc(n), v.x, v.y); }
    void set(const char* n, vec3 v) { glUniform3f(loc(n), v.x, v.y, v.z); }
    void set(const char* n, const mat4& m) { glUniformMatrix4fv(loc(n), 1, GL_FALSE, m.m); }
    // Binds a texture to `unit` and points sampler `n` at it.
    void tex(const char* n, GLuint texture, int unit);

private:
    GLuint prog_ = 0;
    std::unordered_map<std::string, GLint> locs_;
};

struct TextureDesc {
    int width = 1, height = 1;
    GLenum internalFormat = GL_RGBA16F, format = GL_RGBA, type = GL_HALF_FLOAT;
    GLenum filter = GL_LINEAR, wrap = GL_CLAMP_TO_EDGE;
};

class Texture {
public:
    Texture() = default;
    explicit Texture(const TextureDesc& d, const void* data = nullptr) { create(d, data); }
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& o) noexcept { *this = std::move(o); }
    Texture& operator=(Texture&& o) noexcept;

    void create(const TextureDesc& d, const void* data = nullptr);
    GLuint id() const { return id_; }
    const TextureDesc& desc() const { return desc_; }

private:
    GLuint id_ = 0;
    TextureDesc desc_;
};

// A framebuffer with one or more colour attachments and an optional depth buffer.
class RenderTarget {
public:
    RenderTarget() = default;
    ~RenderTarget();
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    RenderTarget(RenderTarget&& o) noexcept { *this = std::move(o); }
    RenderTarget& operator=(RenderTarget&& o) noexcept;

    bool create(const std::vector<TextureDesc>& colors, bool depth);
    void bind() const;  // binds and sets the viewport
    GLuint color(int i = 0) const { return colors_[i].id(); }
    GLuint depthTexture() const { return depth_; }
    int width() const { return w_; }
    int height() const { return h_; }
    GLuint fbo() const { return fbo_; }

private:
    void release();
    GLuint fbo_ = 0, depth_ = 0;
    std::vector<Texture> colors_;
    int w_ = 0, h_ = 0;
};

// Two render targets swapped each step: read from one, write to the other.
struct PingPong {
    RenderTarget a, b;
    bool flip = false;
    RenderTarget& read() { return flip ? b : a; }
    RenderTarget& write() { return flip ? a : b; }
    void swap() { flip = !flip; }
};

class FullscreenPass {
public:
    void init();
    void draw() const;
    ~FullscreenPass();

private:
    GLuint vao_ = 0;
};

}  // namespace gfx
