#include "AmbientOcclusionBuffer.hpp"

#include "FramebufferSupport.hpp"

#include <algorithm>
#include <vector>

namespace {

std::vector<unsigned char> readScalarPixels(int width, int height)
{
    std::vector<float> values(static_cast<size_t>(width) * height);
    std::vector<unsigned char> rgb(values.size() * 3);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RED, GL_FLOAT, values.data());

    for (size_t source = 0, target = 0; source < values.size(); ++source, target += 3) {
        const unsigned char gray = static_cast<unsigned char>(
            std::clamp(values[source], 0.0f, 1.0f) * 255.0f);
        rgb[target] = gray;
        rgb[target + 1] = gray;
        rgb[target + 2] = gray;
    }
    return rgb;
}

} // namespace

AmbientOcclusionBuffer::AmbientOcclusionBuffer(int width, int height)
    : width_(width)
    , height_(height)
    , texture_(FramebufferSupport::createTexture(width, height, GL_R16F, GL_RED))
{
    glGenFramebuffers(1, &fbo_);
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, texture_, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    FramebufferSupport::requireComplete("ambient occlusion framebuffer");
}

AmbientOcclusionBuffer::~AmbientOcclusionBuffer()
{
    glDeleteTextures(1, &texture_);
    glDeleteFramebuffers(1, &fbo_);
}

void AmbientOcclusionBuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
}

void AmbientOcclusionBuffer::clearNeutral() const
{
    bind();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void AmbientOcclusionBuffer::bindTexture(GLenum textureUnit) const
{
    FramebufferSupport::bindTexture(texture_, textureUnit);
}

void AmbientOcclusionBuffer::writeDebug(const std::filesystem::path& path) const
{
    bind();
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    FramebufferSupport::writePpm(path, width_, height_, readScalarPixels(width_, height_));
}
