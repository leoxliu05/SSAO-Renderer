#pragma once

#include "AOBuffer.hpp"

class AOPass {
public:
    void render(const AOBuffer& output) const;
};
