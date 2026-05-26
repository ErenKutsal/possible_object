#include "obj_shape.h"

// REPURPOSED: this slot now hosts the Penrose Stair, loaded from
// models/penrose_stair.obj (Paradox Toolkit primitive).
// The original "Penrose Triangle Blocks" variant had no corresponding OBJ
// since the Paradox addon emits the same continuous Penrose Triangle for
// both the regular and "block" variants.

static ObjShape g_shape;

// Warm stair palette (was the old staircase.cpp's color choice)
static ObjColorPalette stair_palette()
{
    ObjColorPalette p;
    p.xp = vec4(0.20f, 0.46f, 0.80f, 1.0f);   // medium blue side
    p.xn = vec4(0.16f, 0.36f, 0.66f, 1.0f);
    p.yp = vec4(0.08f, 0.22f, 0.56f, 1.0f);   // dark blue front
    p.yn = vec4(0.06f, 0.16f, 0.42f, 1.0f);
    p.zp = vec4(0.52f, 0.78f, 0.95f, 1.0f);   // light blue top
    p.zn = vec4(0.20f, 0.30f, 0.55f, 1.0f);
    p.generic = vec4(0.30f, 0.40f, 0.70f, 1.0f);
    return p;
}

void penrose_block_init()
{
    g_shape.init("../models/penrose_stair.obj", stair_palette());

    // Spin the stair 25° around Y so it doesn't read like just another
    // axis-aligned figure when sitting between Impossible Arch (#7) and
    // Reutersvard Rectangle (#9) in the menu. R-key resets to this too.
    g_shape.angleY        = 25.0f;
    g_shape.defaultAngleY = 25.0f;
}
void penrose_block_display() { g_shape.display(); }
void penrose_block_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void penrose_block_cursorPosCallback(GLFWwindow*, double x, double y) { g_shape.cursorPos(x, y); }
void penrose_block_scrollCallback(GLFWwindow*, double, double y) { g_shape.scroll(y); }
void penrose_block_keyCallback(GLFWwindow* w, int k, int, int a, int) { g_shape.key(w, k, a); }
