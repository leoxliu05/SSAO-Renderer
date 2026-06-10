#pragma once

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

struct Scene {
    std::vector<SceneObject> objects;
    SceneCamera camera;
    RectAreaLight areaLight;
    ShadowSettings shadow;
};

Scene loadScene(const std::filesystem::path& modelDir);
