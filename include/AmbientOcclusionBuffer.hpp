#pragma once

#include <GL/glew.h>

#include <filesystem>

class AmbientOcclusionBuffer {
public:
    AmbientOcclusionBuffer(int width, int height);
    ~AmbientOcclusionBuffer();

    AmbientOcclusionBuffer(const AmbientOcclusionBuffer&) = delete;
    AmbientOcclusionBuffer& operator=(const AmbientOcclusionBuffer&) = delete;

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
