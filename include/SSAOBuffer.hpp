#pragma once

#include <GL/glew.h>

#include <filesystem>

class SSAOBuffer {
public:
    SSAOBuffer(int width, int height);
    ~SSAOBuffer();

    SSAOBuffer(const SSAOBuffer&) = delete;
    SSAOBuffer& operator=(const SSAOBuffer&) = delete;

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
