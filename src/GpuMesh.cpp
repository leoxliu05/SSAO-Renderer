#include "ssao/GpuMesh.hpp"

#include <cstddef>
#include <utility>

GpuMesh::GpuMesh(const std::string& name, const std::vector<Vertex>& vertices, bool emissive)
    : vertexCount_(static_cast<GLsizei>(vertices.size()))
    , emissive_(emissive)
    , name_(name)
{
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
        vertices.data(),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, color)));

    glBindVertexArray(0);
}

GpuMesh::~GpuMesh()
{
    release();
}

GpuMesh::GpuMesh(GpuMesh&& other) noexcept
    : vao_(std::exchange(other.vao_, 0))
    , vbo_(std::exchange(other.vbo_, 0))
    , vertexCount_(std::exchange(other.vertexCount_, 0))
    , emissive_(std::exchange(other.emissive_, false))
    , name_(std::move(other.name_))
{
}

GpuMesh& GpuMesh::operator=(GpuMesh&& other) noexcept
{
    if (this != &other) {
        release();
        vao_ = std::exchange(other.vao_, 0);
        vbo_ = std::exchange(other.vbo_, 0);
        vertexCount_ = std::exchange(other.vertexCount_, 0);
        emissive_ = std::exchange(other.emissive_, false);
        name_ = std::move(other.name_);
    }
    return *this;
}

void GpuMesh::draw() const
{
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount_);
}

void GpuMesh::release()
{
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
}
