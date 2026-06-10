#include "AreaLight.hpp"

#include <algorithm>

std::vector<PointLight> sampleAreaLight(const RectAreaLight& areaLight, int samplesPerSide)
{
    samplesPerSide = std::max(samplesPerSide, 1);

    std::vector<PointLight> lights;
    lights.reserve(static_cast<size_t>(samplesPerSide * samplesPerSide));

    for (int row = 0; row < samplesPerSide; ++row) {
        for (int col = 0; col < samplesPerSide; ++col) {
            float u = (static_cast<float>(col) + 0.5f) / static_cast<float>(samplesPerSide);
            float v = (static_cast<float>(row) + 0.5f) / static_cast<float>(samplesPerSide);
            const Vec3 position = areaLight.origin + areaLight.edgeU * u + areaLight.edgeV * v;

            lights.push_back(PointLight{position, areaLight.color});
        }
    }

    return lights;
}
