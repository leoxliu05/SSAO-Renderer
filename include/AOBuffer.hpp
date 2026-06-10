#pragma once

#include <GL/glew.h>

#include <filesystem>

class AOBuffer {
public:
    AOBuffer(int width, int height);
    ~AOBuffer();

    AOBuffer(const AOBuffer&) = delete;
    AOBuffer& operator=(const AOBuffer&) = delete;

    void bind() const;
    void clearNeutral() const;
    void bindTexture(GLenum textureUnit) const;
    void writeDebug(const std::filesystem::path& path) const;

private:
    int width_ = 0;
    int height_ = 0;
    GLuint fbo_ = 0;
    GLuint texture_ = 0;
};
