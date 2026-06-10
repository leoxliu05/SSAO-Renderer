#include "AOPass.hpp"

void AOPass::render(const AOBuffer& output) const
{
    // The SSAO algorithm will replace this neutral visibility value.
    output.clearNeutral();
}
