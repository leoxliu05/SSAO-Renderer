#include "RenderOutputWriter.hpp"

#include <iostream>

void RenderOutputWriter::write(const AppConfig& config,
    const GeometryBuffer& geometryBuffer,
    const AmbientOcclusionBuffer& ambientOcclusionBuffer,
    const LightingBuffer& lightingBuffer) const
{
    lightingBuffer.writeColor(config.colorOutput);
    geometryBuffer.writeNormalDebug(config.normalOutput);
    geometryBuffer.writeDepthDebug(config.depthOutput);
    ambientOcclusionBuffer.writeDebug(config.ambientOcclusionOutput);

    std::cout << "Wrote " << config.colorOutput << "\n";
    std::cout << "Wrote " << config.normalOutput << "\n";
    std::cout << "Wrote " << config.depthOutput << "\n";
    std::cout << "Wrote " << config.ambientOcclusionOutput << "\n";
}
