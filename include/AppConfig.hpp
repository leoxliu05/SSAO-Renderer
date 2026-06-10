#pragma once

#include <filesystem>

struct AppConfig {
    int width = 1024;
    int height = 1024;
    int areaLightSamplesPerSide = 8;
    int shadowMapSize = 512;
    float ambientStrength = 0.20f;
    float lightIntensity = 1.00f;
    std::filesystem::path modelDir = "models/cornellbox";
    std::filesystem::path colorOutput = "render.ppm";
    std::filesystem::path normalOutput = "normal_debug.ppm";
    std::filesystem::path depthOutput = "depth_debug.ppm";
    std::filesystem::path ambientOcclusionOutput = "ambient_occlusion_debug.ppm";
};

AppConfig parseAppConfig(int argc, char** argv);
