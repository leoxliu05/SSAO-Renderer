#include "AreaLight.hpp"

#include <algorithm>

std::vector<PointLight> sampleCornellAreaLight(int samplesPerSide)
{
    samplesPerSide = std::max(samplesPerSide, 1);

    constexpr float minX = 213.0f;
    constexpr float maxX = 343.0f;
    constexpr float y = 545.0f;
    constexpr float minZ = 227.0f;
    constexpr float maxZ = 332.0f;

    std::vector<PointLight> lights;
    lights.reserve(static_cast<size_t>(samplesPerSide * samplesPerSide));

    for (int row = 0; row < samplesPerSide; ++row) {
        for (int col = 0; col < samplesPerSide; ++col) {
            float u = (static_cast<float>(col) + 0.5f) / static_cast<float>(samplesPerSide);
            float v = (static_cast<float>(row) + 0.5f) / static_cast<float>(samplesPerSide);
            Vec3 position(
                minX + (maxX - minX) * u,
                y,
                minZ + (maxZ - minZ) * v);

            lights.push_back(PointLight{position, Vec3(1.0f)});
        }
    }

    return lights;
}
