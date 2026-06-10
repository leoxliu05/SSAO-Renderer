#include "RenderOutputWriter.hpp"

#include <iostream>

void RenderOutputWriter::write(const AppConfig& config,
    const GeometryBuffer& geometryBuffer,
    const SSAOBuffer& ssaoBuffer,
    const LightingBuffer& lightingBuffer) const
{
    lightingBuffer.writeColor(config.colorOutput);
    geometryBuffer.writeNormalDebug(config.normalOutput);
    geometryBuffer.writeDepthDebug(config.depthOutput);
    ssaoBuffer.writeDebug(config.ssaoOutput);

    std::cout << "Wrote " << config.colorOutput << "\n";
    std::cout << "Wrote " << config.normalOutput << "\n";
    std::cout << "Wrote " << config.depthOutput << "\n";
    std::cout << "Wrote " << config.ssaoOutput << "\n";
}
