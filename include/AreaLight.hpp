#pragma once

#include "Math.hpp"
#include <vector>

struct PointLight {
    Vec3 position{};
    Vec3 color{};
};

std::vector<PointLight> sampleCornellAreaLight(int samplesPerSide);
