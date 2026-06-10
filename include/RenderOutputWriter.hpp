#pragma once

#include "AOBuffer.hpp"
#include "AppConfig.hpp"
#include "GeometryBuffer.hpp"
#include "LightingBuffer.hpp"

class RenderOutputWriter {
public:
    void write(const AppConfig& config,
        const GeometryBuffer& geometryBuffer,
        const AOBuffer& aoBuffer,
        const LightingBuffer& lightingBuffer) const;
};
