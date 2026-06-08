#include "obj_shape.h"

// ─────────────────────────────────────────────────────────────────────────────
// Blocked Penrose — Penrose Triangle block variant (penrose_blocks.obj).
//
// Animation: "Crystal Lightning" — a radial shatter effect that ignites the
// crystalline facets of the blocks outward from the centre.  Structurally
// different from the CCW angular sweep used by water/earth/lava:
//   * Reveal front is a growing CIRCLE (not an angular position).
//   * Voronoi cells light up as the electric front reaches them.
//   * After full coverage, concentric resonance rings pulse from the centre.
//   * Background: deep space with rotating branching lightning bolts + rings.
// ─────────────────────────────────────────────────────────────────────────────

static ObjShape g_shape;

// ── Electric Storm Background ─────────────────────────────────────────────────
static GLuint  bg_program    = 0;
static GLuint  bg_vao        = 0;
static GLuint  bg_vbo        = 0;
static GLint   bg_baseLoc    = -1;
static GLint   bg_timeLoc    = -1;
static GLint   bg_amountLoc  = -1;

static void bg_init()
{
    bg_program  = InitShader(SHADER_DIR "core/vshader_halo.glsl",
        SHADER_DIR "backgrounds/fshader_crystal_background.glsl");
    bg_baseLoc   = glGetUniformLocation(bg_program, "uBaseColor");
    bg_timeLoc   = glGetUniformLocation(bg_program, "uTime");
    bg_amountLoc = glGetUniformLocation(bg_program, "uAmount");

    // Same fullscreen triangle strip as the other bg slots
    static const float quad[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };
    glGenVertexArrays(1, &bg_vao);
    glBindVertexArray(bg_vao);
    glGenBuffers(1, &bg_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, bg_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    GLint aPos = glGetAttribLocation(bg_program, "aPos");
    glEnableVertexAttribArray(aPos);
    glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glBindVertexArray(0);
}

// ── Crystal / Electric palette ────────────────────────────────────────────────
// Dark anthracite mineral tones — the crystal shader will overlay electric
// blue-white arcs on top, so the base just needs to read as dark stone.
static ObjColorPalette crystal_palette()
{
    ObjColorPalette p;
    p.xp = vec4(0.14f, 0.18f, 0.30f, 1.0f);   // X+ — dark blue-grey flint
    p.xn = vec4(0.10f, 0.13f, 0.22f, 1.0f);   // X- — deeper shadow
    p.yp = vec4(0.18f, 0.22f, 0.36f, 1.0f);   // Y+ — top face (catches light)
    p.yn = vec4(0.07f, 0.09f, 0.16f, 1.0f);   // Y- — dark underside
    p.zp = vec4(0.12f, 0.16f, 0.27f, 1.0f);   // Z+ — front face
    p.zn = vec4(0.08f, 0.11f, 0.19f, 1.0f);   // Z-
    p.generic = vec4(0.12f, 0.16f, 0.26f, 1.0f);
    return p;
}

void pbp_init()
{
    // Use the dedicated crystal shader so uPostSolveTime triggers the radial
    // shatter.  The blue-dominant light tint activates the "electric" gate
    // inside the shader (uLightColor.b >> uLightColor.r/g).
    g_shape.init(SHADER_DIR"../models/penrose_blocks.obj", crystal_palette(),
                 nullptr, SHADER_DIR"objects/fshader_crystal.glsl");

    // Electric blue-white key light — strong blue channel activates the gate
    g_shape.setCustomLight(vec3(0.35f, 0.55f, 0.85f));
    g_shape.setCustomLightColor(vec3(0.30f, 0.55f, 1.00f));

    bg_init();
}

void pbp_display()
{
    // Derive reveal progress matching CRYSTAL_DELAY / CRYSTAL_TRACE in the shader
    constexpr float CRYSTAL_DELAY = 0.4f;
    constexpr float CRYSTAL_TRACE = 5.0f;
    float crystalT = g_shape.postSolveTime - CRYSTAL_DELAY;
    if (crystalT < 0.0f) crystalT = 0.0f;
    float amount = crystalT / CRYSTAL_TRACE;
    if (amount > 1.0f) amount = 1.0f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the electric storm background — stays neutral until solved,
    // then fades in as the crystal front expands across the blocks.
    glUseProgram(bg_program);
    glUniform3f(bg_baseLoc,   0.75f, 0.78f, 0.80f);   // pale slate base
    glUniform1f(bg_timeLoc,   (float)glfwGetTime());
    glUniform1f(bg_amountLoc, amount);
    glBindVertexArray(bg_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    g_shape.display();
}

void pbp_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void pbp_cursorPosCallback(GLFWwindow*, double x, double y)    { g_shape.cursorPos(x, y); }
void pbp_scrollCallback(GLFWwindow*, double, double y)         { g_shape.scroll(y); }
void pbp_keyCallback(GLFWwindow* w, int k, int, int a, int)   { g_shape.key(w, k, a); }
