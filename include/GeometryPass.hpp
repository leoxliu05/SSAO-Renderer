#pragma once

#include "AppConfig.hpp"
#include "GeometryBuffer.hpp"
#include "Scene.hpp"
#include "ShaderProgram.hpp"

class GeometryPass {
public:
    GeometryPass();

    void render(const AppConfig& config,
        const Scene& scene,
        const GeometryBuffer& output) const;

private:
    ShaderProgram shader_;
};
