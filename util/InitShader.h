#ifndef INITSHADER
#define INITSHADER

#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>

#include <cstdio>
#include <iostream>

GLuint InitShader(const char* vShaderFile, const char* fShaderFile);

#endif