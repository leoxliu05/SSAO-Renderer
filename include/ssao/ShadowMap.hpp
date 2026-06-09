#pragma once

#include "ssao/GpuMesh.hpp"
#include "ssao/Math.hpp"
#include "ssao/ShaderProgram.hpp"

#include <GL/glew.h>
#include <vector>

class ShadowCubeMap {
public:
    explicit ShadowCubeMap(int size);
    ~ShadowCubeMap();

    ShadowCubeMap(const ShadowCubeMap&) = delete;
    ShadowCubeMap& operator=(const ShadowCubeMap&) = delete;
    ShadowCubeMap(ShadowCubeMap&& other) noexcept;
    ShadowCubeMap& operator=(ShadowCubeMap&& other) noexcept;

    void render(const std::vector<GpuMesh>& meshes,
        const ShaderProgram& shader,
        const Vec3& lightPosition,
        float farPlane) const;
    void bind(GLenum textureUnit) const;

private:
    void release();

    int size_ = 0;
    GLuint fbo_ = 0;
    GLuint depthCubemap_ = 0;
};
