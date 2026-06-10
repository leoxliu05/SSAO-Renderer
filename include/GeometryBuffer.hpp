#pragma once

#include <GL/glew.h>

#include <filesystem>

class GeometryBuffer {
public:
    GeometryBuffer(int width, int height);
    ~GeometryBuffer();

    GeometryBuffer(const GeometryBuffer&) = delete;
    GeometryBuffer& operator=(const GeometryBuffer&) = delete;

    void bind() const;
    void bindPosition(GLenum textureUnit) const;
    void bindNormal(GLenum textureUnit) const;
    void bindMaterial(GLenum textureUnit) const;
    void bindDepth(GLenum textureUnit) const;

    void writeNormalDebug(const std::filesystem::path& path) const;
    void writeDepthDebug(const std::filesystem::path& path) const;

private:
    int width_ = 0;
    int height_ = 0;
    GLuint fbo_ = 0;
    GLuint positionTexture_ = 0;
    GLuint normalTexture_ = 0;
    GLuint materialTexture_ = 0;
    GLuint depthTexture_ = 0;
};
