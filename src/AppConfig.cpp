#include "ssao/AppConfig.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void printUsage()
{
    std::cout
        << "Usage: SSAO_Renderer [--width N] [--height N]\n"
        << "                     [--area-light-samples N]\n"
        << "                     [--shadow-map-size N]\n"
        << "                     [--ambient-strength value]\n"
        << "                     [--light-intensity value]\n"
        << "                     [--shadow-min-light value]\n"
        << "                     [--model-dir path]\n"
        << "                     [--output render.ppm]\n"
        << "                     [--normal-output normal_debug.ppm]\n"
        << "                     [--depth-output depth_debug.ppm]\n";
}

} // namespace

AppConfig parseAppConfig(int argc, char** argv)
{
    AppConfig config;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto requireValue = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error(std::string("missing value for ") + name);
            }
            return argv[++i];
        };

        if (arg == "--width") {
            config.width = std::stoi(requireValue("--width"));
        } else if (arg == "--height") {
            config.height = std::stoi(requireValue("--height"));
        } else if (arg == "--area-light-samples") {
            config.areaLightSamplesPerSide = std::stoi(requireValue("--area-light-samples"));
        } else if (arg == "--shadow-map-size") {
            config.shadowMapSize = std::stoi(requireValue("--shadow-map-size"));
        } else if (arg == "--ambient-strength") {
            config.ambientStrength = std::stof(requireValue("--ambient-strength"));
        } else if (arg == "--light-intensity") {
            config.lightIntensity = std::stof(requireValue("--light-intensity"));
        } else if (arg == "--shadow-min-light") {
            config.shadowMinLight = std::stof(requireValue("--shadow-min-light"));
        } else if (arg == "--model-dir") {
            config.modelDir = requireValue("--model-dir");
        } else if (arg == "--output") {
            config.colorOutput = requireValue("--output");
        } else if (arg == "--normal-output") {
            config.normalOutput = requireValue("--normal-output");
        } else if (arg == "--depth-output") {
            config.depthOutput = requireValue("--depth-output");
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            std::exit(0);
        } else {
            throw std::runtime_error("unknown argument: " + arg);
        }
    }

    if (config.width <= 0 || config.height <= 0) {
        throw std::runtime_error("width and height must be positive");
    }
    if (config.areaLightSamplesPerSide <= 0) {
        throw std::runtime_error("area light samples must be positive");
    }
    if (config.areaLightSamplesPerSide > 16) {
        throw std::runtime_error("area light samples are limited to 16 per side to keep shadow memory bounded");
    }
    if (config.shadowMapSize <= 0) {
        throw std::runtime_error("shadow map size must be positive");
    }
    if (config.ambientStrength < 0.0f) {
        throw std::runtime_error("ambient strength must be non-negative");
    }
    if (config.lightIntensity < 0.0f) {
        throw std::runtime_error("light intensity must be non-negative");
    }
    if (config.shadowMinLight < 0.0f || config.shadowMinLight > 1.0f) {
        throw std::runtime_error("shadow min light must be in [0, 1]");
    }
    return config;
}
