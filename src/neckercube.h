#pragma once
#include "includes.h"

void cube_init();
void cube_display();
void cube_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cube_cursorPosCallback(GLFWwindow* window, double x, double y);
void cube_scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void cube_keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
