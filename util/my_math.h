#pragma once
#include <cmath>

#define DEG2RAD M_PI / 180.0f
#define RAD2DEG 180.0f / M_PI

struct vec3
{
    float x, y, z;

    // Default constructor (initializes to 0)
    vec3() : x(0.0f), y(0.0f), z(0.0f) {}

    // Parameterized constructor
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Vector addition
    vec3 operator+(const vec3& v) const
    {
        return vec3(x + v.x, y + v.y, z + v.z);
    }

    // Scalar multiplication
    vec3 operator*(float s) const { return vec3(x * s, y * s, z * s); }

    // 1. The Writeable Version (Returns a reference)
    float& operator[](int i) { return (&x)[i]; }

    // 2. The Read-Only Version (For const vectors)
    const float& operator[](int i) const { return (&x)[i]; }
};

inline vec3 normalize(const vec3& v)
{
    float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length > 0.0f) return vec3(v.x / length, v.y / length, v.z / length);
    return v;
}

inline vec3 cross(const vec3& a, const vec3& b)
{
    return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

struct vec4
{
    float x, y, z, w;

    // Default constructor (initializes to 0)
    vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}

    // Parameterized constructor
    vec4(float x, float y, float z, float w = 1.0f) : x(x), y(y), z(z), w(w) {}

    // Vector addition
    vec4 operator+(const vec4& v) const
    {
        return vec4(x + v.x, y + v.y, z + v.z, w + v.w);
    }

    // Scalar multiplication
    vec4 operator*(float s) const { return vec4(x * s, y * s, z * s, w * s); }

    // 1. The Writeable Version (Returns a reference)
    float& operator[](int i) { return (&x)[i]; }

    // 2. The Read-Only Version (For const vectors)
    const float& operator[](int i) const { return (&x)[i]; }
};

inline float dot(const vec3& u, const vec3& v)
{
    return u.x * v.x + u.y * v.y + u.z * v.z;
}

inline float dot(const vec4& u, const vec4& v)
{
    return u.x * v.x + u.y * v.y + u.z * v.z + u.w * v.w;
}

struct mat4
{
    // Array layout: m[column][row]
    // This allows direct passing to glUniformMatrix4fv
    vec4 d[4];

    // Default constructor generates an Identity Matrix
    mat4()
    {
        d[0] = vec4(1.0f, 0.0f, 0.0f, 0.0f);
        d[1] = vec4(0.0f, 1.0f, 0.0f, 0.0f);
        d[2] = vec4(0.0f, 0.0f, 1.0f, 0.0f);
        d[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    // Matrix-Matrix Multiplication
    mat4 operator*(const mat4& m) const
    {
        mat4 result;
        for (int c = 0; c < 4; ++c)
        {
            for (int r = 0; r < 4; ++r)
            {
                result.d[c][r] = d[0][r] * m.d[c].x + d[1][r] * m.d[c].y +
                                 d[2][r] * m.d[c].z + d[3][r] * m.d[c].w;
            }
        }
        return result;
    }

    // Matrix-Vector Multiplication (Transforms a vertex)
    vec4 operator*(const vec4& v) const
    {
        return vec4(d[0].x * v.x + d[1].x * v.y + d[2].x * v.z + d[3].x * v.w,
                    d[0].y * v.x + d[1].y * v.y + d[2].y * v.z + d[3].y * v.w,
                    d[0].z * v.x + d[1].z * v.y + d[2].z * v.z + d[3].z * v.w,
                    d[0].w * v.x + d[1].w * v.y + d[2].w * v.z + d[3].w * v.w);
    }
};

inline mat4 Translate(float x, float y, float z)
{
    mat4 result;  // Starts as identity
    // Remember: d[col]
    result.d[3] = vec4(x, y, z, 1.0f);
    return result;
}

inline mat4 Scale(float x, float y, float z)
{
    mat4 result;
    result.d[0].x = x;
    result.d[1].y = y;
    result.d[2].z = z;
    return result;
}

inline mat4 RotateX(float degrees)
{
    mat4 result;

    float radians = degrees * DEG2RAD;
    float c = cos(radians);
    float s = sin(radians);

    result.d[1][1] = c;
    result.d[2][1] = -s;
    result.d[1][2] = s;
    result.d[2][2] = c;
    return result;
}

inline mat4 RotateY(float degrees)
{
    mat4 result;

    float radians = degrees * DEG2RAD;
    float c = cos(radians);
    float s = sin(radians);

    result.d[0][0] = c;
    result.d[2][0] = -s;
    result.d[0][2] = s;
    result.d[2][2] = c;
    return result;
}

inline mat4 RotateZ(float degrees)
{
    mat4 result;

    float radians = degrees * DEG2RAD;
    float c = cos(radians);
    float s = sin(radians);

    result.d[0][0] = c;
    result.d[1][0] = -s;
    result.d[0][1] = s;
    result.d[1][1] = c;
    return result;
}

inline mat4 LookAt(const vec3& eye, const vec3& at, const vec3& up)
{
    // 1. Calculate the forward (n), right (u), and true up (v) vectors
    vec3 n = normalize(vec3(eye.x - at.x, eye.y - at.y, eye.z - at.z));
    vec3 u = normalize(cross(up, n));
    vec3 v = normalize(cross(n, u));

    // 2. Build the rotation part of the matrix (rows become the directional
    // axes)
    mat4 view;
    view.d[0].x = u.x;
    view.d[1].x = u.y;
    view.d[2].x = u.z;
    view.d[0].y = v.x;
    view.d[1].y = v.y;
    view.d[2].y = v.z;
    view.d[0].z = n.x;
    view.d[1].z = n.y;
    view.d[2].z = n.z;

    // 3. Translate the world in the opposite direction of the eye
    mat4 translation = Translate(-eye.x, -eye.y, -eye.z);

    // Combine them!
    return view * translation;
}

inline mat4 Perspective(float fovy, float aspect, float near, float far)
{
    float rad = fovy * DEG2RAD;
    float top = near * tanf(rad / 2.0f);
    float right = top * aspect;

    mat4 result;
    result.d[0].x = near / right;
    result.d[1].y = near / top;
    result.d[2].z = -(far + near) / (far - near);
    result.d[2].w = -1.0f;
    result.d[3].z = -2.0f * far * near / (far - near);
    result.d[3].w = 0.0f;
    return result;
}

inline mat4 Ortho(float left, float right, float bottom, float top, float near,
                  float far)
{
    mat4 result;  // Starts as identity matrix

    // Scale the X, Y, and Z axes
    result.d[0].x = 2.0f / (right - left);
    result.d[1].y = 2.0f / (top - bottom);
    result.d[2].z = -2.0f / (far - near);

    // Translate the box to the center
    result.d[3].x = -(right + left) / (right - left);
    result.d[3].y = -(top + bottom) / (top - bottom);
    result.d[3].z = -(far + near) / (far - near);

    return result;
}