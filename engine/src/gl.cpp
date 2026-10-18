#include "gfx/gl.hpp"

#include <cstdio>
#include <utility>

namespace gfx {

namespace {
std::string gError;

GLuint compile(GLenum type, const std::string& src, const std::string& name) {
    GLuint s = glCreateShader(type);
    std::string full = std::string(kGlslHeader) + src;
    const char* p = full.c_str();
    glShaderSource(s, 1, &p, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(s, sizeof log, nullptr, log);
        setGlError(name + (type == GL_VERTEX_SHADER ? " (vertex): " : " (fragment): ") + log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}
}  // namespace

const std::string& lastGlError() { return gError; }
void setGlError(const std::string& msg) {
    gError = msg;
    std::fprintf(stderr, "[gfx] %s\n", msg.c_str());
}

const char* kGlslHeader =
    "#version 300 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "precision highp sampler2D;\n";

const char* kFullscreenVS = R"(
out vec2 vUv;
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vUv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";

// ---- Shader ----------------------------------------------------------------

bool Shader::build(const std::string& vs, const std::string& fs, const std::string& name) {
    GLuint v = compile(GL_VERTEX_SHADER, vs, name);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs, name);
    if (!v || !f) {
        if (v) glDeleteShader(v);
        if (f) glDeleteShader(f);
        return false;
    }
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(p, sizeof log, nullptr, log);
        setGlError(name + " (link): " + log);
        glDeleteProgram(p);
        return false;
    }
    if (prog_) glDeleteProgram(prog_);
    prog_ = p;
    locs_.clear();
    return true;
}

Shader::~Shader() {
    if (prog_) glDeleteProgram(prog_);
}

Shader& Shader::operator=(Shader&& o) noexcept {
    std::swap(prog_, o.prog_);
    std::swap(locs_, o.locs_);
    return *this;
}

GLint Shader::loc(const char* name) {
    auto it = locs_.find(name);
    if (it != locs_.end()) return it->second;
    GLint l = glGetUniformLocation(prog_, name);
    locs_.emplace(name, l);
    return l;
}

void Shader::tex(const char* n, GLuint texture, int unit) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(loc(n), unit);
}

// ---- Texture ---------------------------------------------------------------

void Texture::create(const TextureDesc& d, const void* data) {
    if (id_) glDeleteTextures(1, &id_);
    desc_ = d;
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
    glTexImage2D(GL_TEXTURE_2D, 0, d.internalFormat, d.width, d.height, 0, d.format, d.type, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, d.filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, d.filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, d.wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, d.wrap);
}

Texture::~Texture() {
    if (id_) glDeleteTextures(1, &id_);
}

Texture& Texture::operator=(Texture&& o) noexcept {
    std::swap(id_, o.id_);
    std::swap(desc_, o.desc_);
    return *this;
}

// ---- RenderTarget ----------------------------------------------------------

void RenderTarget::release() {
    if (fbo_) glDeleteFramebuffers(1, &fbo_);
    if (depth_) glDeleteTextures(1, &depth_);
    fbo_ = depth_ = 0;
    colors_.clear();
}

RenderTarget::~RenderTarget() { release(); }

RenderTarget& RenderTarget::operator=(RenderTarget&& o) noexcept {
    std::swap(fbo_, o.fbo_);
    std::swap(depth_, o.depth_);
    std::swap(colors_, o.colors_);
    std::swap(w_, o.w_);
    std::swap(h_, o.h_);
    return *this;
}

bool RenderTarget::create(const std::vector<TextureDesc>& colors, bool depth) {
    release();
    w_ = colors.at(0).width;
    h_ = colors.at(0).height;
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    std::vector<GLenum> bufs;
    for (size_t i = 0; i < colors.size(); ++i) {
        colors_.emplace_back(colors[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GLenum(GL_COLOR_ATTACHMENT0 + i), GL_TEXTURE_2D,
                               colors_.back().id(), 0);
        bufs.push_back(GLenum(GL_COLOR_ATTACHMENT0 + i));
    }
    glDrawBuffers(GLsizei(bufs.size()), bufs.data());
    if (depth) {
        glGenTextures(1, &depth_);
        glBindTexture(GL_TEXTURE_2D, depth_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w_, h_, 0, GL_DEPTH_COMPONENT,
                     GL_UNSIGNED_INT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_, 0);
    }
    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (st != GL_FRAMEBUFFER_COMPLETE) {
        char buf[96];
        std::snprintf(buf, sizeof buf, "Framebuffer incomplete (0x%x). Float render targets may be unsupported.", st);
        setGlError(buf);
        return false;
    }
    return true;
}

void RenderTarget::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, w_, h_);
}

// ---- FullscreenPass --------------------------------------------------------

void FullscreenPass::init() {
    if (!vao_) glGenVertexArrays(1, &vao_);
}

void FullscreenPass::draw() const {
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

FullscreenPass::~FullscreenPass() {
    if (vao_) glDeleteVertexArrays(1, &vao_);
}

}  // namespace gfx
