#pragma once

#include <GL/glew.h>

#include <filesystem>

class LightingBuffer {
public:
    LightingBuffer(int width, int height);
    ~LightingBuffer();

    LightingBuffer(const LightingBuffer&) = delete;
    LightingBuffer& operator=(const LightingBuffer&) = delete;

    void bind() const;
    void writeColor(const std::filesystem::path& path) const;

private:
    int width_ = 0;
    int height_ = 0;
    GLuint fbo_ = 0;
    GLuint texture_ = 0;
};
