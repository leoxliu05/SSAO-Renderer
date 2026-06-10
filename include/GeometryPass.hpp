#pragma once

#include "AppConfig.hpp"
#include "GeometryBuffer.hpp"
#include "RenderScene.hpp"
#include "ShaderProgram.hpp"

class GeometryPass {
public:
    GeometryPass();

    void render(const AppConfig& config,
        const RenderScene& scene,
        const GeometryBuffer& output) const;

private:
    ShaderProgram shader_;
};
