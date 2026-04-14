#pragma once
#include "includes.h"

void polygon_create_solid_segment(int n_segments, float radius, float thickness);
void polygon_init();
void polygon_display();
void polygon_set_constants(int n_segments);
void polygon_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void polygon_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void polygon_cursorPosCallback(GLFWwindow* window, double xpos, double ypos);