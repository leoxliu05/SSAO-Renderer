#pragma once

#include <GL/glew.h>

class FullscreenTriangle {
public:
    FullscreenTriangle();
    ~FullscreenTriangle();

    FullscreenTriangle(const FullscreenTriangle&) = delete;
    FullscreenTriangle& operator=(const FullscreenTriangle&) = delete;

    void draw() const;

private:
    GLuint vao_ = 0;
};
