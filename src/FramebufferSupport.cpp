#include "FramebufferSupport.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

namespace FramebufferSupport {

GLuint createTexture(int width, int height, GLenum internalFormat, GLenum format)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat),
        width, height, 0, format, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

void requireComplete(const char* name)
{
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error(std::string(name) + " is incomplete");
    }
}

void bindTexture(GLuint texture, GLenum textureUnit)
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, texture);
}

void writePpm(const std::filesystem::path& path,
    int width,
    int height,
    const std::vector<unsigned char>& rgb)
{
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("failed to open output: " + path.string());
    }

    out << "P6\n" << width << " " << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y) {
        const unsigned char* row = rgb.data() + static_cast<size_t>(y) * width * 3;
        out.write(reinterpret_cast<const char*>(row), width * 3);
    }
}

} // namespace FramebufferSupport
