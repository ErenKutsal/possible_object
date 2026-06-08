#pragma once
#include "includes.h"

void penrose_m_init();
void penrose_m_display();
void penrose_m_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void penrose_m_cursorPosCallback(GLFWwindow* window, double x, double y);
void penrose_m_scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void penrose_m_keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
