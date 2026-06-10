#include "Framebuffer.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace {

GLuint createColorTexture(int width, int height, GLenum internalFormat, GLenum format, GLenum type)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat),
        width, height, 0, format, type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

GLuint createDepthTexture(int width, int height)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
        width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

void writePpm(const std::filesystem::path& path, int width, int height, const std::vector<unsigned char>& rgb)
{
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("failed to open output: " + path.string());
    }

    out << "P6\n" << width << " " << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y) {
        const unsigned char* row = rgb.data() + static_cast<size_t>(y) * width * 3;
        out.write(reinterpret_cast<const char*>(row), width * 3);
    }
}

std::vector<unsigned char> readColorPixels(int width, int height)
{
    std::vector<float> rgba(static_cast<size_t>(width) * height * 4);
    std::vector<unsigned char> rgb(static_cast<size_t>(width) * height * 3);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, rgba.data());

    for (size_t i = 0, j = 0; i < rgba.size(); i += 4, j += 3) {
        rgb[j + 0] = static_cast<unsigned char>(std::clamp(rgba[i + 0], 0.0f, 1.0f) * 255.0f);
        rgb[j + 1] = static_cast<unsigned char>(std::clamp(rgba[i + 1], 0.0f, 1.0f) * 255.0f);
        rgb[j + 2] = static_cast<unsigned char>(std::clamp(rgba[i + 2], 0.0f, 1.0f) * 255.0f);
    }
    return rgb;
}

std::vector<unsigned char> readNormalPixels(int width, int height)
{
    std::vector<float> rgba(static_cast<size_t>(width) * height * 4);
    std::vector<unsigned char> rgb(static_cast<size_t>(width) * height * 3);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, rgba.data());

    for (size_t i = 0, j = 0; i < rgba.size(); i += 4, j += 3) {
        rgb[j + 0] = static_cast<unsigned char>(std::clamp(rgba[i + 0], 0.0f, 1.0f) * 255.0f);
        rgb[j + 1] = static_cast<unsigned char>(std::clamp(rgba[i + 1], 0.0f, 1.0f) * 255.0f);
        rgb[j + 2] = static_cast<unsigned char>(std::clamp(rgba[i + 2], 0.0f, 1.0f) * 255.0f);
    }
    return rgb;
}

std::vector<unsigned char> readDepthPixels(int width, int height)
{
    std::vector<float> depth(static_cast<size_t>(width) * height);
    std::vector<unsigned char> rgb(static_cast<size_t>(width) * height * 3);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data());

    float minDepth = 1.0f;
    float maxDepth = 0.0f;
    for (float d : depth) {
        if (d < 1.0f) {
            minDepth = std::min(minDepth, d);
            maxDepth = std::max(maxDepth, d);
        }
    }

    float range = std::max(maxDepth - minDepth, 1e-6f);
    for (size_t i = 0, j = 0; i < depth.size(); ++i, j += 3) {
        float value = depth[i] >= 1.0f ? 0.0f : 1.0f - (depth[i] - minDepth) / range;
        unsigned char c = static_cast<unsigned char>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
        rgb[j + 0] = c;
        rgb[j + 1] = c;
        rgb[j + 2] = c;
    }
    return rgb;
}

} // namespace

Framebuffer::Framebuffer(int width, int height)
    : width_(width)
    , height_(height)
    , colorTexture_(createColorTexture(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT))
    , normalTexture_(createColorTexture(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT))
    , depthTexture_(createDepthTexture(width, height))
{
    glGenFramebuffers(1, &fbo_);
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalTexture_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture_, 0);

    GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, drawBuffers);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("framebuffer is incomplete");
    }
}

Framebuffer::~Framebuffer()
{
    if (colorTexture_ != 0) {
        glDeleteTextures(1, &colorTexture_);
    }
    if (normalTexture_ != 0) {
        glDeleteTextures(1, &normalTexture_);
    }
    if (depthTexture_ != 0) {
        glDeleteTextures(1, &depthTexture_);
    }
    if (fbo_ != 0) {
        glDeleteFramebuffers(1, &fbo_);
    }
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : width_(other.width_)
    , height_(other.height_)
    , fbo_(other.fbo_)
    , colorTexture_(other.colorTexture_)
    , normalTexture_(other.normalTexture_)
    , depthTexture_(other.depthTexture_)
{
    other.fbo_ = 0;
    other.colorTexture_ = 0;
    other.normalTexture_ = 0;
    other.depthTexture_ = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
{
    if (this != &other) {
        this->~Framebuffer();
        width_ = other.width_;
        height_ = other.height_;
        fbo_ = other.fbo_;
        colorTexture_ = other.colorTexture_;
        normalTexture_ = other.normalTexture_;
        depthTexture_ = other.depthTexture_;
        other.fbo_ = 0;
        other.colorTexture_ = 0;
        other.normalTexture_ = 0;
        other.depthTexture_ = 0;
    }
    return *this;
}

void Framebuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
}

void Framebuffer::writeColor(const std::filesystem::path& path) const
{
    bind();
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    writePpm(path, width_, height_, readColorPixels(width_, height_));
}

void Framebuffer::writeNormalDebug(const std::filesystem::path& path) const
{
    bind();
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    writePpm(path, width_, height_, readNormalPixels(width_, height_));
}

void Framebuffer::writeDepthDebug(const std::filesystem::path& path) const
{
    bind();
    writePpm(path, width_, height_, readDepthPixels(width_, height_));
}
