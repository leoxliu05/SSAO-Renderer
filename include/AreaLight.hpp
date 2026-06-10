#pragma once

#include "Math.hpp"
#include "Scene.hpp"

#include <vector>

struct PointLight {
    Vec3 position{};
    Vec3 color{};
};

std::vector<PointLight> sampleAreaLight(const RectAreaLight& areaLight, int samplesPerSide);
