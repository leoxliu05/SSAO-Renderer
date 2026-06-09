#pragma once

#include <filesystem>

struct AppConfig {
    int width = 1024;
    int height = 1024;
    int areaLightSamplesPerSide = 4;
    int shadowMapSize = 512;
    std::filesystem::path modelDir = "models/cornellbox";
    std::filesystem::path colorOutput = "render.ppm";
    std::filesystem::path normalOutput = "normal_debug.ppm";
    std::filesystem::path depthOutput = "depth_debug.ppm";
};

AppConfig parseAppConfig(int argc, char** argv);
