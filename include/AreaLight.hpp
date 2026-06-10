#pragma once

#include "Math.hpp"
#include "Scene.hpp"

#include <vector>

std::vector<PointLight> sampleAreaLight(const RectAreaLight& areaLight, int samplesPerSide);
