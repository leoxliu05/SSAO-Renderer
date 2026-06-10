#pragma once

#include "Scene.hpp"
#include "ShaderProgram.hpp"
#include "ShadowMap.hpp"

#include <vector>

class ShadowPass {
public:
    explicit ShadowPass(int shadowMapSize);

    std::vector<ShadowMap> render(const Scene& scene) const;

private:
    int shadowMapSize_ = 0;
    ShaderProgram shader_;
};
