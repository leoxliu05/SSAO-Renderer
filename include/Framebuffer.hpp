#pragma once

#include <GL/glew.h>

#include <filesystem>

class Framebuffer {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    void bind() const;
    void writeColor(const std::filesystem::path& path) const;
    void writeNormalDebug(const std::filesystem::path& path) const;
    void writeDepthDebug(const std::filesystem::path& path) const;

private:
    int width_ = 0;
    int height_ = 0;
    GLuint fbo_ = 0;
    GLuint colorTexture_ = 0;
    GLuint normalTexture_ = 0;
    GLuint depthTexture_ = 0;
};
