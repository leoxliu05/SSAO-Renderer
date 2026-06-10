#include "AmbientOcclusionPass.hpp"

void AmbientOcclusionPass::render(const AmbientOcclusionBuffer& output) const
{
    // The SSAO algorithm will replace this neutral visibility value.
    output.clearNeutral();
}
