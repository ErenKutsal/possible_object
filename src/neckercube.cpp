#include "obj_shape.h"

// Impossible Cube — loaded from models/impossible_cube.obj
// (Paradox Toolkit primitive). Replaces the previous hand-built 12-bar version.

static ObjShape g_shape;

// Distinct cube palette
static ObjColorPalette cube_palette()
{
    ObjColorPalette p;
    p.xp = vec4(0.67f, 0.75f, 0.92f, 1.0f);
    p.xn = vec4(0.50f, 0.56f, 0.75f, 1.0f);
    p.yp = vec4(0.88f, 0.88f, 0.70f, 1.0f);
    p.yn = vec4(0.75f, 0.75f, 0.58f, 1.0f);
    p.zp = vec4(0.66f, 0.56f, 0.63f, 1.0f);
    p.zn = vec4(0.52f, 0.43f, 0.50f, 1.0f);
    p.generic = vec4(0.55f, 0.55f, 0.58f, 1.0f);
    return p;
}

void cube_init()
{
    g_shape.init("../models/impossible_cube.obj", cube_palette());
}
void cube_display() { g_shape.display(); }
void cube_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void cube_cursorPosCallback(GLFWwindow*, double x, double y) { g_shape.cursorPos(x, y); }
void cube_scrollCallback(GLFWwindow*, double, double y) { g_shape.scroll(y); }
void cube_keyCallback(GLFWwindow* w, int k, int, int a, int) { g_shape.key(w, k, a); }
