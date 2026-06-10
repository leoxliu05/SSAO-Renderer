#pragma once

#include "AppConfig.hpp"
#include "GpuMesh.hpp"
#include "Math.hpp"

#include <filesystem>
#include <vector>

struct SceneObject {
    std::filesystem::path objPath;
    Vec3 color{};
    Vec3 positionOffset{};
    bool emissive = false;
};

struct SceneCamera {
    Vec3 position{};
    Vec3 target{};
    Vec3 up{};
    float fovYDegrees = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
};

struct RectAreaLight {
    Vec3 origin{};
    Vec3 edgeU{};
    Vec3 edgeV{};
    Vec3 color{1.0f};
};

struct ShadowSettings {
    Vec3 target{};
    Vec3 up{};
    float fovYDegrees = 90.0f;
    float nearPlane = 1.0f;
    float farPlane = 1000.0f;
};

struct PointLight {
    Vec3 position{};
    Vec3 color{};
};

class Scene {
public:
    explicit Scene(const AppConfig& config);

    std::vector<SceneObject> objects;
    SceneCamera camera;
    RectAreaLight areaLight;
    ShadowSettings shadow;
    std::vector<GpuMesh> meshes;
    std::vector<PointLight> lights;

    // render settings read from scene.json
    int areaLightSamplesPerSide = 8;
    int shadowMapSize = 512;
    float ambientStrength = 0.2f;
    float lightIntensity = 1.0f;
};
