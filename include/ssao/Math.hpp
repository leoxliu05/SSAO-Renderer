#pragma once

#include <cmath>

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    explicit Vec3(float value)
        : x(value)
        , y(value)
        , z(value)
    {
    }
    Vec3(float xValue, float yValue, float zValue)
        : x(xValue)
        , y(yValue)
        , z(zValue)
    {
    }
};

struct Mat4 {
    float m[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
};

inline Vec3 operator+(const Vec3& a, const Vec3& b)
{
    return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline Vec3 operator-(const Vec3& a, const Vec3& b)
{
    return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline Vec3 operator*(const Vec3& v, float scalar)
{
    return Vec3(v.x * scalar, v.y * scalar, v.z * scalar);
}

inline float dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b)
{
    return Vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x);
}

inline float length(const Vec3& v)
{
    return std::sqrt(dot(v, v));
}

inline Vec3 normalize(const Vec3& v)
{
    float len = length(v);
    if (len <= 1e-8f) {
        return Vec3(0.0f);
    }
    return v * (1.0f / len);
}

inline float radians(float degrees)
{
    return degrees * 0.017453292519943295769f;
}

inline Mat4 operator*(const Mat4& a, const Mat4& b)
{
    Mat4 result{};
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            result.m[col * 4 + row] =
                a.m[0 * 4 + row] * b.m[col * 4 + 0] +
                a.m[1 * 4 + row] * b.m[col * 4 + 1] +
                a.m[2 * 4 + row] * b.m[col * 4 + 2] +
                a.m[3 * 4 + row] * b.m[col * 4 + 3];
        }
    }
    return result;
}

inline Mat4 perspective(float fovyRadians, float aspect, float nearPlane, float farPlane)
{
    Mat4 result{};
    for (float& value : result.m) {
        value = 0.0f;
    }

    float tanHalfFovy = std::tan(fovyRadians * 0.5f);
    result.m[0] = 1.0f / (aspect * tanHalfFovy);
    result.m[5] = 1.0f / tanHalfFovy;
    result.m[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    result.m[11] = -1.0f;
    result.m[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
    return result;
}

inline Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
{
    Vec3 f = normalize(center - eye);
    Vec3 s = normalize(cross(f, up));
    Vec3 u = cross(s, f);

    Mat4 result{};
    result.m[0] = s.x;
    result.m[4] = s.y;
    result.m[8] = s.z;
    result.m[1] = u.x;
    result.m[5] = u.y;
    result.m[9] = u.z;
    result.m[2] = -f.x;
    result.m[6] = -f.y;
    result.m[10] = -f.z;
    result.m[12] = -dot(s, eye);
    result.m[13] = -dot(u, eye);
    result.m[14] = dot(f, eye);
    return result;
}
