#pragma once

#include <GL/glew.h>

#include <sstream>
#include <stdexcept>
#include <string>

inline void checkGl(const std::string& label)
{
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::ostringstream message;
        message << label << " failed with GL error 0x" << std::hex << error;
        throw std::runtime_error(message.str());
    }
}
