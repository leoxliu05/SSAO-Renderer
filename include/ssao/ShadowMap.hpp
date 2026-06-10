#pragma once

#include "ssao/GpuMesh.hpp"
#include "ssao/Math.hpp"
#include "ssao/ShaderProgram.hpp"

#include <GL/glew.h>
#include <vector>

class ShadowMap {
public:
    explicit ShadowMap(int size);
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;
    ShadowMap(ShadowMap&& other) noexcept;
    ShadowMap& operator=(ShadowMap&& other) noexcept;

    void render(const std::vector<GpuMesh>& meshes,
        const ShaderProgram& shader,
        const Vec3& lightPosition,
        float farPlane);
    void bind(GLenum textureUnit) const;
    const Mat4& lightViewProjection() const { return lightViewProjection_; }

private:
    void release();

    int size_ = 0;
    GLuint fbo_ = 0;
    GLuint depthTexture_ = 0;
    Mat4 lightViewProjection_{};
};
