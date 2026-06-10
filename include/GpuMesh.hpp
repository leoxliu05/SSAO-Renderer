#pragma once

#include "Vertex.hpp"

#include <GL/glew.h>
#include <string>
#include <vector>

class GpuMesh {
public:
    GpuMesh() = default;
    GpuMesh(const std::string& name, const std::vector<Vertex>& vertices, bool emissive);
    ~GpuMesh();

    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;
    GpuMesh(GpuMesh&& other) noexcept;
    GpuMesh& operator=(GpuMesh&& other) noexcept;

    void draw() const;
    bool emissive() const { return emissive_; }

private:
    void release();

    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLsizei vertexCount_ = 0;
    bool emissive_ = false;
    std::string name_;
};
