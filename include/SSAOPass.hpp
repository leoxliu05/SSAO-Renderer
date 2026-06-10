#pragma once

#include "SSAOBuffer.hpp"
#include "FullscreenTriangle.hpp"
#include "GeometryBuffer.hpp"
#include "Math.hpp"
#include "ShaderProgram.hpp"

#include <GL/glew.h>
#include "SSAOBuffer.hpp"

class SSAOPass {
public:
    SSAOPass();
    void render(const SSAOBuffer& output, const GeometryBuffer& gbuffer,
                const Mat4& view, const Mat4& proj,
                int width, int height) const;

private:
    ShaderProgram shader_;
    FullscreenTriangle fullscreenTriangle_;
    GLuint noiseTexture_ = 0;
    std::vector<Vec3> kernel_;
};
