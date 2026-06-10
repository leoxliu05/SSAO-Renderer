#include "ShadowMap.hpp"

#include <stdexcept>
#include <utility>

namespace {

Mat4 makeLightViewProjection(const Vec3& lightPosition, float farPlane)
{
    const Mat4 projection = perspective(radians(90.0f), 1.0f, 1.0f, farPlane);
    const Mat4 view = lookAt(
        lightPosition,
        Vec3(278.0f, 273.0f, 279.6f),
        Vec3(0.0f, 0.0f, -1.0f));
    return projection * view;
}

} // namespace

ShadowMap::ShadowMap(int size)
    : size_(size)
{
    glGenFramebuffers(1, &fbo_);
    glGenTextures(1, &depthTexture_);
    glBindTexture(GL_TEXTURE_2D, depthTexture_);
    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT24,
        size_,
        size_,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture_, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("shadow framebuffer is incomplete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ShadowMap::~ShadowMap()
{
    release();
}

ShadowMap::ShadowMap(ShadowMap&& other) noexcept
    : size_(std::exchange(other.size_, 0))
    , fbo_(std::exchange(other.fbo_, 0))
    , depthTexture_(std::exchange(other.depthTexture_, 0))
    , lightViewProjection_(other.lightViewProjection_)
{
}

ShadowMap& ShadowMap::operator=(ShadowMap&& other) noexcept
{
    if (this != &other) {
        release();
        size_ = std::exchange(other.size_, 0);
        fbo_ = std::exchange(other.fbo_, 0);
        depthTexture_ = std::exchange(other.depthTexture_, 0);
        lightViewProjection_ = other.lightViewProjection_;
    }
    return *this;
}

void ShadowMap::render(const std::vector<GpuMesh>& meshes,
    const ShaderProgram& shader,
    const Vec3& lightPosition,
    float farPlane)
{
    lightViewProjection_ = makeLightViewProjection(lightPosition, farPlane);

    glViewport(0, 0, size_, size_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glClear(GL_DEPTH_BUFFER_BIT);

    shader.use();
    shader.setMat4("uLightViewProjection", lightViewProjection_);

    for (const GpuMesh& mesh : meshes) {
        if (!mesh.emissive()) {
            mesh.draw();
        }
    }

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::bind(GLenum textureUnit) const
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, depthTexture_);
}

void ShadowMap::release()
{
    if (depthTexture_ != 0) {
        glDeleteTextures(1, &depthTexture_);
        depthTexture_ = 0;
    }
    if (fbo_ != 0) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
}
