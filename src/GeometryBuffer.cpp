#include "GeometryBuffer.hpp"

#include "FramebufferSupport.hpp"

#include <algorithm>
#include <vector>

namespace {

std::vector<unsigned char> readNormalPixels(int width, int height)
{
    std::vector<float> normal(static_cast<size_t>(width) * height * 3);
    std::vector<unsigned char> rgb(normal.size());

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_FLOAT, normal.data());

    for (size_t i = 0; i < normal.size(); ++i) {
        const float encoded = normal[i] * 0.5f + 0.5f;
        rgb[i] = static_cast<unsigned char>(std::clamp(encoded, 0.0f, 1.0f) * 255.0f);
    }
    return rgb;
}

std::vector<unsigned char> readDepthPixels(int width, int height)
{
    std::vector<float> depth(static_cast<size_t>(width) * height);
    std::vector<unsigned char> rgb(depth.size() * 3);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data());

    float minDepth = 1.0f;
    float maxDepth = 0.0f;
    for (float value : depth) {
        if (value < 1.0f) {
            minDepth = std::min(minDepth, value);
            maxDepth = std::max(maxDepth, value);
        }
    }

    const float range = std::max(maxDepth - minDepth, 1e-6f);
    for (size_t source = 0, target = 0; source < depth.size(); ++source, target += 3) {
        const float value = depth[source] >= 1.0f
            ? 0.0f
            : 1.0f - (depth[source] - minDepth) / range;
        const unsigned char gray = static_cast<unsigned char>(
            std::clamp(value, 0.0f, 1.0f) * 255.0f);
        rgb[target] = gray;
        rgb[target + 1] = gray;
        rgb[target + 2] = gray;
    }
    return rgb;
}

} // namespace

GeometryBuffer::GeometryBuffer(int width, int height)
    : width_(width)
    , height_(height)
    , positionTexture_(FramebufferSupport::createTexture(width, height, GL_RGB32F, GL_RGB))
    , normalTexture_(FramebufferSupport::createTexture(width, height, GL_RGB16F, GL_RGB))
    , materialTexture_(FramebufferSupport::createTexture(width, height, GL_RGBA16F, GL_RGBA))
    , depthTexture_(FramebufferSupport::createTexture(
          width, height, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT))
{
    glGenFramebuffers(1, &fbo_);
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, positionTexture_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
        GL_TEXTURE_2D, normalTexture_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
        GL_TEXTURE_2D, materialTexture_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, depthTexture_, 0);

    const GLenum attachments[] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
    };
    glDrawBuffers(3, attachments);
    FramebufferSupport::requireComplete("geometry framebuffer");
}

GeometryBuffer::~GeometryBuffer()
{
    glDeleteTextures(1, &positionTexture_);
    glDeleteTextures(1, &normalTexture_);
    glDeleteTextures(1, &materialTexture_);
    glDeleteTextures(1, &depthTexture_);
    glDeleteFramebuffers(1, &fbo_);
}

void GeometryBuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
}

void GeometryBuffer::bindPosition(GLenum textureUnit) const
{
    FramebufferSupport::bindTexture(positionTexture_, textureUnit);
}

void GeometryBuffer::bindNormal(GLenum textureUnit) const
{
    FramebufferSupport::bindTexture(normalTexture_, textureUnit);
}

void GeometryBuffer::bindMaterial(GLenum textureUnit) const
{
    FramebufferSupport::bindTexture(materialTexture_, textureUnit);
}

void GeometryBuffer::bindDepth(GLenum textureUnit) const
{
    FramebufferSupport::bindTexture(depthTexture_, textureUnit);
}

void GeometryBuffer::writeNormalDebug(const std::filesystem::path& path) const
{
    bind();
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    FramebufferSupport::writePpm(path, width_, height_, readNormalPixels(width_, height_));
}

void GeometryBuffer::writeDepthDebug(const std::filesystem::path& path) const
{
    bind();
    FramebufferSupport::writePpm(path, width_, height_, readDepthPixels(width_, height_));
}
