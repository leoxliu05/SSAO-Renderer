#pragma once

#include "AmbientOcclusionBuffer.hpp"
#include "AppConfig.hpp"
#include "GeometryBuffer.hpp"
#include "LightingBuffer.hpp"

class RenderOutputWriter {
public:
    void write(const AppConfig& config,
        const GeometryBuffer& geometryBuffer,
        const AmbientOcclusionBuffer& ambientOcclusionBuffer,
        const LightingBuffer& lightingBuffer) const;
};
