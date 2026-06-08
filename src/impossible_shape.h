#pragma once
#include "includes.h"

void impossible_init();
void impossible_display();
void impossible_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void impossible_cursorPosCallback(GLFWwindow* window, double x, double y);
void impossible_scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void impossible_keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
