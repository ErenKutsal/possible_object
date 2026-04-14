#ifndef INCLUDES_H
#define INCLUDES_H

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

// ------------------------------------------------
// Math types matching the project's usage:
//   vec3, vec4, mat4
//   mat4.d[4] is an array of vec4 (column-major)
// ------------------------------------------------

struct vec2
{
    float x, y;
    vec2() : x(0), y(0) {}
    vec2(float x, float y) : x(x), y(y) {}
};

struct vec3
{
    float x, y, z;
    vec3() : x(0), y(0), z(0) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    vec3 operator-(const vec3& b) const { return vec3(x - b.x, y - b.y, z - b.z); }
    vec3 operator+(const vec3& b) const { return vec3(x + b.x, y + b.y, z + b.z); }
    vec3 operator*(float s) const { return vec3(x * s, y * s, z * s); }
};

inline vec3 cross(const vec3& a, const vec3& b)
{
    return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
inline float dot(const vec3& a, const vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline vec3 normalize(const vec3& v)
{
    float len = sqrtf(dot(v, v));
    return len > 0 ? v * (1.0f / len) : v;
}

struct vec4
{
    float x, y, z, w;
    vec4() : x(0), y(0), z(0), w(0) {}
    vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

struct mat4
{
    vec4 d[4];  // columns

    mat4()
    {
        d[0] = vec4(1, 0, 0, 0);
        d[1] = vec4(0, 1, 0, 0);
        d[2] = vec4(0, 0, 1, 0);
        d[3] = vec4(0, 0, 0, 1);
    }

    mat4 operator*(const mat4& b) const
    {
        mat4 r;
        // Access as arrays for multiplication
        const float* A = &d[0].x;
        const float* B = &b.d[0].x;
        float* R = &r.d[0].x;
        for (int c = 0; c < 4; c++)
            for (int row = 0; row < 4; row++)
            {
                float sum = 0;
                for (int k = 0; k < 4; k++) sum += A[k * 4 + row] * B[c * 4 + k];
                R[c * 4 + row] = sum;
            }
        return r;
    }
};

// ------------------------------------------------
// Transform functions
// ------------------------------------------------

inline mat4 Translate(float x, float y, float z)
{
    mat4 m;
    m.d[3] = vec4(x, y, z, 1);
    return m;
}

inline mat4 Scale(float x, float y, float z)
{
    mat4 m;
    m.d[0].x = x;
    m.d[1].y = y;
    m.d[2].z = z;
    return m;
}

inline mat4 RotateX(float angle)
{
    float rad = angle * M_PI / 180.0f;
    float c = cosf(rad), s = sinf(rad);
    mat4 m;
    m.d[1] = vec4(0, c, s, 0);
    m.d[2] = vec4(0, -s, c, 0);
    return m;
}

inline mat4 RotateY(float angle)
{
    float rad = angle * M_PI / 180.0f;
    float c = cosf(rad), s = sinf(rad);
    mat4 m;
    m.d[0] = vec4(c, 0, -s, 0);
    m.d[2] = vec4(s, 0, c, 0);
    return m;
}

inline mat4 RotateZ(float angle)
{
    float rad = angle * M_PI / 180.0f;
    float c = cosf(rad), s = sinf(rad);
    mat4 m;
    m.d[0] = vec4(c, s, 0, 0);
    m.d[1] = vec4(-s, c, 0, 0);
    return m;
}

inline mat4 LookAt(const vec3& eye, const vec3& at, const vec3& up)
{
    vec3 f = normalize(at - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);
    mat4 m;
    m.d[0] = vec4(s.x, u.x, -f.x, 0);
    m.d[1] = vec4(s.y, u.y, -f.y, 0);
    m.d[2] = vec4(s.z, u.z, -f.z, 0);
    m.d[3] = vec4(-dot(s, eye), -dot(u, eye), dot(f, eye), 1);
    return m;
}

inline mat4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
{
    mat4 m;
    memset(&m, 0, sizeof(m));
    m.d[0].x = 2.0f / (right - left);
    m.d[1].y = 2.0f / (top - bottom);
    m.d[2].z = -2.0f / (zFar - zNear);
    m.d[3].x = -(right + left) / (right - left);
    m.d[3].y = -(top + bottom) / (top - bottom);
    m.d[3].z = -(zFar + zNear) / (zFar - zNear);
    m.d[3].w = 1.0f;
    return m;
}

inline mat4 Perspective(float fovy, float aspect, float zNear, float zFar)
{
    float rad = fovy * M_PI / 180.0f;
    float f = 1.0f / tanf(rad / 2.0f);
    mat4 m;
    memset(&m, 0, sizeof(m));
    m.d[0].x = f / aspect;
    m.d[1].y = f;
    m.d[2].z = (zFar + zNear) / (zNear - zFar);
    m.d[2].w = -1.0f;
    m.d[3].z = (2.0f * zFar * zNear) / (zNear - zFar);
    return m;
}

// ------------------------------------------------
// Shader loading
// ------------------------------------------------
inline GLuint InitShader(const char* vertPath, const char* fragPath)
{
    auto readFile = [](const char* path) -> std::string
    {
        std::ifstream f(path);
        if (!f.is_open())
        {
            fprintf(stderr, "Cannot open shader: %s\n", path);
            return "";
        }
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    };

    std::string vSrc = readFile(vertPath);
    std::string fSrc = readFile(fragPath);

    auto compile = [](GLenum type, const char* src) -> GLuint
    {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, NULL);
        glCompileShader(s);
        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            char log[512];
            glGetShaderInfoLog(s, 512, NULL, log);
            fprintf(stderr, "Shader compile error: %s\n", log);
        }
        return s;
    };

    GLuint vs = compile(GL_VERTEX_SHADER, vSrc.c_str());
    GLuint fs = compile(GL_FRAGMENT_SHADER, fSrc.c_str());

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[512];
        glGetProgramInfoLog(prog, 512, NULL, log);
        fprintf(stderr, "Shader link error: %s\n", log);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ------------------------------------------------
// Forward declarations for all modules
// ------------------------------------------------

// Penrose Triangle
void penrose_tri_init();
void penrose_tri_display();
void penrose_tri_mouseButtonCallback(GLFWwindow*, int, int, int);
void penrose_tri_cursorPosCallback(GLFWwindow*, double, double);
void penrose_tri_scrollCallback(GLFWwindow*, double, double);
void penrose_tri_keyCallback(GLFWwindow*, int, int, int, int);

// Penrose Triangle Block
void penrose_block_init();
void penrose_block_display();
void penrose_block_mouseButtonCallback(GLFWwindow*, int, int, int);
void penrose_block_cursorPosCallback(GLFWwindow*, double, double);
void penrose_block_scrollCallback(GLFWwindow*, double, double);
void penrose_block_keyCallback(GLFWwindow*, int, int, int, int);

// Reutersvard Rectangle
void reutersvard_init();
void reutersvard_display();
void reutersvard_mouseButtonCallback(GLFWwindow*, int, int, int);
void reutersvard_cursorPosCallback(GLFWwindow*, double, double);
void reutersvard_scrollCallback(GLFWwindow*, double, double);
void reutersvard_keyCallback(GLFWwindow*, int, int, int, int);

// Impossible Arch
void arch_init();
void arch_display();
void arch_mouseButtonCallback(GLFWwindow*, int, int, int);
void arch_cursorPosCallback(GLFWwindow*, double, double);
void arch_scrollCallback(GLFWwindow*, double, double);
void arch_keyCallback(GLFWwindow*, int, int, int, int);

// Penrose Staircase
void stair_init();
void stair_display();
void stair_mouseButtonCallback(GLFWwindow*, int, int, int);
void stair_cursorPosCallback(GLFWwindow*, double, double);
void stair_scrollCallback(GLFWwindow*, double, double);
void stair_keyCallback(GLFWwindow*, int, int, int, int);

// Impossible Cube
void icube_init();
void icube_display();
void icube_mouseButtonCallback(GLFWwindow*, int, int, int);
void icube_cursorPosCallback(GLFWwindow*, double, double);
void icube_scrollCallback(GLFWwindow*, double, double);
void icube_keyCallback(GLFWwindow*, int, int, int, int);

// Impossible Triangle (Classic)
void itri_init();
void itri_display();
void itri_mouseButtonCallback(GLFWwindow*, int, int, int);
void itri_cursorPosCallback(GLFWwindow*, double, double);
void itri_scrollCallback(GLFWwindow*, double, double);
void itri_keyCallback(GLFWwindow*, int, int, int, int);

#endif
