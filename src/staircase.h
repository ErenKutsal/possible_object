#pragma once
#include "includes.h"

void stair_init();
void stair_display();
void stair_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void stair_cursorPosCallback(GLFWwindow* window, double x, double y);
void stair_scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void stair_keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
