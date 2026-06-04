#include "obj_shape.h"

// Reutersvärd Rectangle — loaded from models/reutersvard_rectangle.obj
// (Paradox Toolkit primitive). New module — no procedural equivalent existed.

static ObjShape g_shape;

void reutersvard_init()
{
    g_shape.init("../models/reutersvard_rectangle.obj", default_palette());

    // No ball orbit on this slot — only slots 1 (procedural polygon) and
    // 2 (Penrose Triangle) keep their rolling indicator.
}
void reutersvard_display() { g_shape.display(); }
void reutersvard_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void reutersvard_cursorPosCallback(GLFWwindow*, double x, double y) { g_shape.cursorPos(x, y); }
void reutersvard_scrollCallback(GLFWwindow*, double, double y) { g_shape.scroll(y); }
void reutersvard_keyCallback(GLFWwindow* w, int k, int, int a, int) { g_shape.key(w, k, a); }
