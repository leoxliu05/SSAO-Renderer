#pragma once

#include <GL/glew.h>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#endif

class OpenGlContext {
public:
    OpenGlContext();
    ~OpenGlContext();

    OpenGlContext(const OpenGlContext&) = delete;
    OpenGlContext& operator=(const OpenGlContext&) = delete;

private:
#ifdef __APPLE__
    CGLContextObj context_ = nullptr;
#endif
};
