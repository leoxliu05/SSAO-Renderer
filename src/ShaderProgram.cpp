#include "ssao/ShaderProgram.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace {

GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<size_t>(length), '\0');
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("shader compilation failed: " + log);
    }
    return shader;
}

} // namespace

ShaderProgram::ShaderProgram(const char* vertexSource, const char* fragmentSource)
    : ShaderProgram(vertexSource, nullptr, fragmentSource)
{
}

ShaderProgram::ShaderProgram(const char* vertexSource, const char* geometrySource, const char* fragmentSource)
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint geometryShader = geometrySource != nullptr ? compileShader(GL_GEOMETRY_SHADER, geometrySource) : 0;
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    program_ = glCreateProgram();
    glAttachShader(program_, vertexShader);
    if (geometryShader != 0) {
        glAttachShader(program_, geometryShader);
    }
    glAttachShader(program_, fragmentShader);
    glLinkProgram(program_);

    glDeleteShader(vertexShader);
    if (geometryShader != 0) {
        glDeleteShader(geometryShader);
    }
    glDeleteShader(fragmentShader);

    GLint ok = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint length = 0;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<size_t>(length), '\0');
        glGetProgramInfoLog(program_, length, nullptr, log.data());
        glDeleteProgram(program_);
        program_ = 0;
        throw std::runtime_error("shader link failed: " + log);
    }
}

ShaderProgram::~ShaderProgram()
{
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : program_(std::exchange(other.program_, 0))
{
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
    if (this != &other) {
        if (program_ != 0) {
            glDeleteProgram(program_);
        }
        program_ = std::exchange(other.program_, 0);
    }
    return *this;
}

void ShaderProgram::use() const
{
    glUseProgram(program_);
}

void ShaderProgram::setBool(const char* name, bool value) const
{
    glUniform1i(glGetUniformLocation(program_, name), value ? 1 : 0);
}

void ShaderProgram::setFloat(const char* name, float value) const
{
    glUniform1f(glGetUniformLocation(program_, name), value);
}

void ShaderProgram::setInt(const char* name, int value) const
{
    glUniform1i(glGetUniformLocation(program_, name), value);
}

void ShaderProgram::setMat4(const char* name, const Mat4& value) const
{
    glUniformMatrix4fv(glGetUniformLocation(program_, name), 1, GL_FALSE, value.m);
}

void ShaderProgram::setVec3(const char* name, const Vec3& value) const
{
    glUniform3f(glGetUniformLocation(program_, name), value.x, value.y, value.z);
}
