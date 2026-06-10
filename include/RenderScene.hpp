#pragma once

#include "AppConfig.hpp"
#include "AreaLight.hpp"
#include "GpuMesh.hpp"
#include "Scene.hpp"

#include <vector>

class RenderScene {
public:
    explicit RenderScene(const AppConfig& config);

    const Scene& description() const { return description_; }
    const std::vector<GpuMesh>& meshes() const { return meshes_; }
    const std::vector<PointLight>& lights() const { return lights_; }

private:
    Scene description_;
    std::vector<GpuMesh> meshes_;
    std::vector<PointLight> lights_;
};
