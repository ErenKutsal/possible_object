#pragma once
#include "includes.h"

void penrose_init();
void penrose_display();
void penrose_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void penrose_cursorPosCallback(GLFWwindow* window, double x, double y);
void penrose_scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void penrose_keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
