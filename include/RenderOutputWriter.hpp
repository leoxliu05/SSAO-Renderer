#pragma once

#include "SSAOBuffer.hpp"
#include "AppConfig.hpp"
#include "GeometryBuffer.hpp"
#include "LightingBuffer.hpp"

class RenderOutputWriter {
public:
    void write(const AppConfig& config,
        const GeometryBuffer& geometryBuffer,
        const SSAOBuffer& SSAOBuffer,
        const LightingBuffer& lightingBuffer) const;
};
