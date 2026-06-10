#pragma once

#include "Math.hpp"

#include <GL/glew.h>

class ShaderProgram {
public:
    ShaderProgram(const char* vertexSource, const char* fragmentSource);
    ShaderProgram(const char* vertexSource, const char* geometrySource, const char* fragmentSource);
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    void use() const;
    void setBool(const char* name, bool value) const;
    void setFloat(const char* name, float value) const;
    void setInt(const char* name, int value) const;
    void setMat4(const char* name, const Mat4& value) const;
    void setVec3(const char* name, const Vec3& value) const;

private:
    GLuint program_ = 0;
};
