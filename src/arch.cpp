#include "obj_shape.h"

// Impossible Arch — loaded from models/impossible_arch.obj
// (Paradox Toolkit primitive).

static ObjShape g_shape;

void arch_init()
{
    g_shape.init("../models/impossible_arch.obj", default_palette());

    // Rotate 90° CCW around Z (screen-plane CCW from the default iso view).
    // Default angleZ was -45; -45 + 90 = +45 lays the arch on its side.
    g_shape.angleZ        =  45.0f;
    g_shape.defaultAngleZ =  45.0f;
}
void arch_display() { g_shape.display(); }
void arch_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void arch_cursorPosCallback(GLFWwindow*, double x, double y) { g_shape.cursorPos(x, y); }
void arch_scrollCallback(GLFWwindow*, double, double y) { g_shape.scroll(y); }
void arch_keyCallback(GLFWwindow* w, int k, int, int a, int) { g_shape.key(w, k, a); }
