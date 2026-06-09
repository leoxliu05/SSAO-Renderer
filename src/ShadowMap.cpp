#include "ssao/ShadowMap.hpp"

#include <array>
#include <stdexcept>
#include <utility>

ShadowCubeMap::ShadowCubeMap(int size)
    : size_(size)
{
    glGenFramebuffers(1, &fbo_);
    glGenTextures(1, &depthCubemap_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap_);

    for (int face = 0; face < 6; ++face) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
            0,
            GL_DEPTH_COMPONENT24,
            size_,
            size_,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        depthCubemap_,
        0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("shadow framebuffer is incomplete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ShadowCubeMap::~ShadowCubeMap()
{
    release();
}

ShadowCubeMap::ShadowCubeMap(ShadowCubeMap&& other) noexcept
    : size_(std::exchange(other.size_, 0))
    , fbo_(std::exchange(other.fbo_, 0))
    , depthCubemap_(std::exchange(other.depthCubemap_, 0))
{
}

ShadowCubeMap& ShadowCubeMap::operator=(ShadowCubeMap&& other) noexcept
{
    if (this != &other) {
        release();
        size_ = std::exchange(other.size_, 0);
        fbo_ = std::exchange(other.fbo_, 0);
        depthCubemap_ = std::exchange(other.depthCubemap_, 0);
    }
    return *this;
}

void ShadowCubeMap::render(const std::vector<GpuMesh>& meshes,
    const ShaderProgram& shader,
    const Vec3& lightPosition,
    float farPlane) const
{
    const Mat4 projection = perspective(radians(90.0f), 1.0f, 1.0f, farPlane);
    const std::array<Mat4, 6> views = {
        lookAt(lightPosition, lightPosition + Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f)),
        lookAt(lightPosition, lightPosition + Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f)),
        lookAt(lightPosition, lightPosition + Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)),
        lookAt(lightPosition, lightPosition + Vec3(0.0f, -1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f)),
        lookAt(lightPosition, lightPosition + Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f)),
        lookAt(lightPosition, lightPosition + Vec3(0.0f, 0.0f, -1.0f), Vec3(0.0f, -1.0f, 0.0f)),
    };

    glViewport(0, 0, size_, size_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    shader.use();
    shader.setVec3("uLightPosition", lightPosition);
    shader.setFloat("uFarPlane", farPlane);

    for (int face = 0; face < 6; ++face) {
        glFramebufferTexture2D(GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
            depthCubemap_,
            0);
        glClear(GL_DEPTH_BUFFER_BIT);
        shader.setMat4("uLightViewProjection", projection * views[face]);

        for (const GpuMesh& mesh : meshes) {
            mesh.draw();
        }
    }

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowCubeMap::bind(GLenum textureUnit) const
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap_);
}

void ShadowCubeMap::release()
{
    if (depthCubemap_ != 0) {
        glDeleteTextures(1, &depthCubemap_);
        depthCubemap_ = 0;
    }
    if (fbo_ != 0) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
}
