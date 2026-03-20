#ifndef INCLUDES
#define INCLUDES

// 1. Apple (Mac) Setup
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>

// 2. Windows / Linux Setup
#else
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#endif

#include <iostream>

#include "InitShader.h"
#include "my_math.h"

#endif  // GL_INCLUDES_H