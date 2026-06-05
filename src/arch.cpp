#include "obj_shape.h"

// Impossible Arch — loaded from models/impossible_arch.obj
// (Paradox Toolkit primitive).

static ObjShape g_shape;

// ── Firelight halo (per-slot background pre-pass) ───────────────────────────
// Concept: the lava on the figure is a light source that warms the air around
// it. Implementation: a fullscreen quad at the far depth, painting the shared
// pale slate plus a soft gaussian warm spill centred on the figure's screen
// position. Intensity ramps up across the lava reveal so the surroundings
// visibly heat up as the molten cracks ignite, then holds while the ambient
// sweep continues. Pedagogically: the cheapest "object lights its environment"
// effect — one additive falloff, no FBO chain, but it stops the figure from
// reading as a sticker pasted onto the slate.
static GLuint  halo_program        = 0;
static GLuint  halo_vao            = 0;
static GLuint  halo_vbo            = 0;
static GLint   halo_baseLoc        = -1;
static GLint   halo_colorLoc       = -1;
static GLint   halo_centerLoc      = -1;
static GLint   halo_amountLoc      = -1;
static GLint   halo_falloffLoc     = -1;

static void halo_init()
{
    halo_program = InitShader("../shaders/vshader_halo.glsl",
                              "../shaders/fshader_halo.glsl");
    halo_baseLoc    = glGetUniformLocation(halo_program, "uBaseColor");
    halo_colorLoc   = glGetUniformLocation(halo_program, "uHaloColor");
    halo_centerLoc  = glGetUniformLocation(halo_program, "uHaloCenter");
    halo_amountLoc  = glGetUniformLocation(halo_program, "uHaloAmount");
    halo_falloffLoc = glGetUniformLocation(halo_program, "uHaloFalloff");

    // Fullscreen triangle strip — 4 NDC corners.
    static const float quad[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };
    glGenVertexArrays(1, &halo_vao);
    glBindVertexArray(halo_vao);
    glGenBuffers(1, &halo_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, halo_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    GLint aPos = glGetAttribLocation(halo_program, "aPos");
    glEnableVertexAttribArray(aPos);
    glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glBindVertexArray(0);
}

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

    halo_init();

    // No ball orbit on this slot — only slots 1 (procedural polygon) and
    // 2 (Penrose Triangle) keep their rolling indicator.
}

void arch_display()
{
    // ── Halo pre-pass: paint the background with a warm radial spill from
    // the figure. Same pale slate as every other slot at the corners, warming
    // toward amber at the figure. Amount ramps over the lava reveal so the
    // scene visibly heats up alongside the cracks igniting.
    //
    // LAVA_DELAY = 0.7s (must match fshader_impossible) — the wait before the
    // lava starts loading. Halo fades in over the same window the lava trace
    // sweeps, hits full strength as the trace completes, then holds.
    constexpr float LAVA_DELAY = 0.7f;
    constexpr float LAVA_TRACE = 6.0f;
    float lavaT  = g_shape.postSolveTime - LAVA_DELAY;
    if (lavaT < 0.0f) lavaT = 0.0f;
    float amount = lavaT / LAVA_TRACE;
    if (amount > 1.0f) amount = 1.0f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(halo_program);
    glUniform3f(halo_baseLoc,    0.75f, 0.78f, 0.80f);   // pale slate base
    glUniform3f(halo_colorLoc,   1.00f, 0.55f, 0.22f);   // warm amber spill
    glUniform2f(halo_centerLoc,  0.10f, 0.06f);          // figure centre in NDC, measured from captures
    glUniform1f(halo_amountLoc,  amount * 0.65f);        // cap so the halo never overpowers the figure
    glUniform1f(halo_falloffLoc, 2.0f);                  // wide spill — reads as ambient warmth, not a spotlight
    glBindVertexArray(halo_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    g_shape.display();
}

void arch_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void arch_cursorPosCallback(GLFWwindow*, double x, double y) { g_shape.cursorPos(x, y); }
void arch_scrollCallback(GLFWwindow*, double, double y) { g_shape.scroll(y); }
void arch_keyCallback(GLFWwindow* w, int k, int, int a, int) { g_shape.key(w, k, a); }
