#include "OpenGLHelpers.hpp"

#include <sstream>
#include <stdexcept>

namespace OpenGLHelpers {

Context::Context()
{
#ifdef __APPLE__
    CGLPixelFormatAttribute attributes[] = {
        kCGLPFAOpenGLProfile,
        static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
        kCGLPFAAccelerated,
        kCGLPFAColorSize,
        static_cast<CGLPixelFormatAttribute>(24),
        kCGLPFADepthSize,
        static_cast<CGLPixelFormatAttribute>(24),
        kCGLPFAAlphaSize,
        static_cast<CGLPixelFormatAttribute>(8),
        static_cast<CGLPixelFormatAttribute>(0),
    };

    CGLPixelFormatObj pixelFormat = nullptr;
    GLint pixelFormatCount = 0;
    CGLError error = CGLChoosePixelFormat(attributes, &pixelFormat, &pixelFormatCount);
    if (error != kCGLNoError || pixelFormat == nullptr) {
        throw std::runtime_error("failed to choose CGL pixel format");
    }

    error = CGLCreateContext(pixelFormat, nullptr, &context_);
    CGLDestroyPixelFormat(pixelFormat);
    if (error != kCGLNoError || context_ == nullptr) {
        throw std::runtime_error("failed to create CGL context");
    }

    error = CGLSetCurrentContext(context_);
    if (error != kCGLNoError) {
        throw std::runtime_error("failed to make CGL context current");
    }
#else
    throw std::runtime_error(
        "headless OpenGL context setup is implemented for macOS in this scaffold");
#endif

    glewExperimental = GL_TRUE;
    const GLenum glewError = glewInit();
    glGetError();
    if (glewError != GLEW_OK) {
        throw std::runtime_error(
            reinterpret_cast<const char*>(glewGetErrorString(glewError)));
    }
}

Context::~Context()
{
#ifdef __APPLE__
    CGLSetCurrentContext(nullptr);
    if (context_ != nullptr) {
        CGLDestroyContext(context_);
    }
#endif
}

void checkError(const std::string& label)
{
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::ostringstream message;
        message << label << " failed with GL error 0x" << std::hex << error;
        throw std::runtime_error(message.str());
    }
}

} // namespace OpenGLHelpers
