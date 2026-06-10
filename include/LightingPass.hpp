#pragma once

#include "SSAOBuffer.hpp"
#include "AppConfig.hpp"
#include "FullscreenTriangle.hpp"
#include "GeometryBuffer.hpp"
#include "LightingBuffer.hpp"
#include "Scene.hpp"
#include "ShaderProgram.hpp"
#include "ShadowMap.hpp"

#include <vector>

class LightingPass {
public:
    LightingPass();

    void render(const AppConfig& config,
        const Scene& scene,
        const std::vector<ShadowMap>& shadowMaps,
        const GeometryBuffer& geometryBuffer,
        const SSAOBuffer& ssaoBuffer,
        const LightingBuffer& output) const;

private:
    ShaderProgram shader_;
    FullscreenTriangle fullscreenTriangle_;
};
