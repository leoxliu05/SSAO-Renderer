#pragma once

#include <GL/glew.h>

#include <filesystem>
#include <vector>

namespace FramebufferSupport {

GLuint createTexture(int width, int height, GLenum internalFormat, GLenum format);
void requireComplete(const char* name);
void bindTexture(GLuint texture, GLenum textureUnit);
void writePpm(const std::filesystem::path& path,
    int width,
    int height,
    const std::vector<unsigned char>& rgb);

} // namespace FramebufferSupport
