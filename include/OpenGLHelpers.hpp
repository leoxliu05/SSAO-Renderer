#pragma once

#include <GL/glew.h>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#endif

#include <string>

namespace OpenGLHelpers {

class Context {
public:
    Context();
    ~Context();

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

private:
#ifdef __APPLE__
    CGLContextObj context_ = nullptr;
#endif
};

void checkError(const std::string& label);

} // namespace OpenGLHelpers
