#include "obj_shape.h"

// Impossible Arch — loaded from models/impossible_arch.obj
// (Paradox Toolkit primitive).

static ObjShape g_shape;

// ── Tier 2 — "Forge / Fire" palette ─────────────────────────────────────────
// Dark warm iron instead of the cool default blue/mauve, so the amber key light
// (set in arch_init) can "heat" the lit faces while faces turned away stay dark
// forged iron. The top face (Y+) is pre-warmed toward heated-steel straw; the
// undersides (Y-) sit in cool-shadow iron. Tuned to read as hot metal, not
// plastic. Per-slot only — every other slot keeps default_palette().
static ObjColorPalette forge_palette()
{
    ObjColorPalette p;
    p.xp = vec4(0.44f, 0.34f, 0.29f, 1.0f);   // X+ — warm iron (side)
    p.xn = vec4(0.30f, 0.23f, 0.20f, 1.0f);   // X- — darker iron
    p.yp = vec4(0.56f, 0.41f, 0.30f, 1.0f);   // Y+ — heated steel / straw (top)
    p.yn = vec4(0.24f, 0.19f, 0.18f, 1.0f);   // Y- — shadow iron (underside)
    p.zp = vec4(0.49f, 0.37f, 0.31f, 1.0f);   // Z+ — clay-iron
    p.zn = vec4(0.31f, 0.25f, 0.22f, 1.0f);   // Z-
    p.generic = vec4(0.40f, 0.31f, 0.27f, 1.0f);
    return p;
}

void arch_init()
{
    g_shape.init("../models/impossible_arch.obj", forge_palette());

    // Rotate 90° CCW around Z (screen-plane CCW from the default iso view).
    // Default angleZ was -45; -45 + 90 = +45 lays the arch on its side.
    g_shape.angleZ        =  45.0f;
    g_shape.defaultAngleZ =  45.0f;

    // ── Tier 2 — "Forge / Fire" lighting ────────────────────────────────────
    // A warm, low-angle amber key light gives this slot a hot-metal mood that
    // sets it apart from the cool/neutral slots. Both the direction and the
    // color are DIRECTIONAL / global-per-draw, so shading still depends only
    // on the face normal + a constant tint — the seamless solved join from
    // the previous pass is preserved.
    g_shape.setCustomLight(vec3(-0.45f, 0.32f, 0.82f));     // low warm sun, front-left
    g_shape.setCustomLightColor(vec3(1.00f, 0.74f, 0.48f)); // amber

    // No ball orbit on this slot — only slots 1 (procedural polygon) and
    // 2 (Penrose Triangle) keep their rolling indicator.
}
void arch_display()
{
    // ── Backdrop — shared pale slate, same as every other slot ───────────────
    // (Previously a dark forge charcoal, but the lighter default reads better
    // and keeps this slot consistent with the others; the PAYOFF — the figure
    // igniting into rock + lava once solved — lives in fshader_impossible.glsl,
    // so the fire is on the OBJECT, not the background.)
    glClearColor(0.75f, 0.78f, 0.80f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    g_shape.display();
}
void arch_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void arch_cursorPosCallback(GLFWwindow*, double x, double y) { g_shape.cursorPos(x, y); }
void arch_scrollCallback(GLFWwindow*, double, double y) { g_shape.scroll(y); }
void arch_keyCallback(GLFWwindow* w, int k, int, int a, int) { g_shape.key(w, k, a); }
