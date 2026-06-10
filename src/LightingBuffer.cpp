#include "LightingBuffer.hpp"

#include "FramebufferSupport.hpp"

#include <algorithm>
#include <vector>

namespace {

std::vector<unsigned char> readRgbPixels(int width, int height)
{
    std::vector<float> rgba(static_cast<size_t>(width) * height * 4);
    std::vector<unsigned char> rgb(static_cast<size_t>(width) * height * 3);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, rgba.data());

    for (size_t source = 0, target = 0; source < rgba.size(); source += 4, target += 3) {
        rgb[target] = static_cast<unsigned char>(
            std::clamp(rgba[source], 0.0f, 1.0f) * 255.0f);
        rgb[target + 1] = static_cast<unsigned char>(
            std::clamp(rgba[source + 1], 0.0f, 1.0f) * 255.0f);
        rgb[target + 2] = static_cast<unsigned char>(
            std::clamp(rgba[source + 2], 0.0f, 1.0f) * 255.0f);
    }
    return rgb;
}

} // namespace

LightingBuffer::LightingBuffer(int width, int height)
    : width_(width)
    , height_(height)
    , texture_(FramebufferSupport::createTexture(width, height, GL_RGBA16F, GL_RGBA))
{
    glGenFramebuffers(1, &fbo_);
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, texture_, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    FramebufferSupport::requireComplete("lighting framebuffer");
}

LightingBuffer::~LightingBuffer()
{
    glDeleteTextures(1, &texture_);
    glDeleteFramebuffers(1, &fbo_);
}

void LightingBuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
}

void LightingBuffer::writeColor(const std::filesystem::path& path) const
{
    bind();
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    FramebufferSupport::writePpm(path, width_, height_, readRgbPixels(width_, height_));
}
