#include "FullscreenTriangle.hpp"

FullscreenTriangle::FullscreenTriangle()
{
    glGenVertexArrays(1, &vao_);
}

FullscreenTriangle::~FullscreenTriangle()
{
    glDeleteVertexArrays(1, &vao_);
}

void FullscreenTriangle::draw() const
{
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
