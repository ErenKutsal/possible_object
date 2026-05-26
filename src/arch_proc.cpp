#include "obj_shape.h"

// ─────────────────────────────────────────────────────────────────────────────
// Impossible Arch (proc slot) — now the Paradox Toolkit's Impossible Arch,
// loaded fresh from Blender via the same ObjShape pipeline as the other
// Blender-loaded slots. Visually coherent with slots #2, #3, #4, #5.
//
// Distinct cool violet palette so it reads as different from slot #6 (which
// loads the same OBJ with the default neutral palette).
//
// Function names keep the `archp_` prefix so main.cpp's dispatch is unchanged.
// ─────────────────────────────────────────────────────────────────────────────

static ObjShape g_shape;

// Cool violet / lilac palette
static ObjColorPalette arch_proc_palette()
{
    ObjColorPalette p;
    p.xp = vec4(0.68f, 0.58f, 0.82f, 1.0f);   // X+ — pale lavender
    p.xn = vec4(0.52f, 0.42f, 0.66f, 1.0f);
    p.yp = vec4(0.76f, 0.66f, 0.86f, 1.0f);   // Y+ — light lilac
    p.yn = vec4(0.58f, 0.48f, 0.72f, 1.0f);
    p.zp = vec4(0.62f, 0.54f, 0.78f, 1.0f);   // Z+ — periwinkle
    p.zn = vec4(0.48f, 0.40f, 0.62f, 1.0f);
    p.generic = vec4(0.60f, 0.52f, 0.74f, 1.0f);
    return p;
}

void archp_init()
{
    g_shape.init("../models/impossible_arch_wide.obj", arch_proc_palette());
}
void archp_display() { g_shape.display(); }
void archp_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void archp_cursorPosCallback(GLFWwindow*, double x, double y) { g_shape.cursorPos(x, y); }
void archp_scrollCallback(GLFWwindow*, double, double y) { g_shape.scroll(y); }
void archp_keyCallback(GLFWwindow* w, int k, int, int a, int) { g_shape.key(w, k, a); }
