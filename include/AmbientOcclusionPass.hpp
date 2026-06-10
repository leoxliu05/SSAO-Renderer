#pragma once

#include "AmbientOcclusionBuffer.hpp"

class AmbientOcclusionPass {
public:
    void render(const AmbientOcclusionBuffer& output) const;
};
