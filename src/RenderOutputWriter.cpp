#include "RenderOutputWriter.hpp"

#include <iostream>

void RenderOutputWriter::write(const AppConfig& config,
    const GeometryBuffer& geometryBuffer,
    const AOBuffer& aoBuffer,
    const LightingBuffer& lightingBuffer) const
{
    lightingBuffer.writeColor(config.colorOutput);
    geometryBuffer.writeNormalDebug(config.normalOutput);
    geometryBuffer.writeDepthDebug(config.depthOutput);
    aoBuffer.writeDebug(config.aoOutput);

    std::cout << "Wrote " << config.colorOutput << "\n";
    std::cout << "Wrote " << config.normalOutput << "\n";
    std::cout << "Wrote " << config.depthOutput << "\n";
    std::cout << "Wrote " << config.aoOutput << "\n";
}
