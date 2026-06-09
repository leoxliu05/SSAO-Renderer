#pragma once

#include "ssao/Math.hpp"

#include <filesystem>
#include <string>
#include <vector>

struct SceneObject {
    std::filesystem::path objPath;
    Vec3 color{};
    bool emissive = false;
};

std::vector<SceneObject> loadCornellBoxScene(const std::filesystem::path& modelDir);
