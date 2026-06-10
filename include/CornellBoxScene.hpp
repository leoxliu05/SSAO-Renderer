#pragma once

#include "Math.hpp"

#include <filesystem>
#include <string>
#include <vector>

struct SceneObject {
    std::filesystem::path objPath;
    Vec3 color{};
    Vec3 positionOffset{};
    bool emissive = false;
};

std::vector<SceneObject> loadCornellBoxScene(const std::filesystem::path& modelDir);
