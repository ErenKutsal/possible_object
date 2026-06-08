#include "obj_shape.h"

// Reutersvärd Rectangle — loaded from models/reutersvard_rectangle.obj
// (Paradox Toolkit primitive). Watery effect variant.

static ObjShape g_shape;

// ── Water Background (fades in when finished) ────────────────────────────────
// Uses the same fullscreen quad geometry as the old halo, but now drives
// fshader_water_background which draws an animated ocean scene that fades
// in from the neutral slate clear colour as the puzzle is completed.
static GLuint  bg_program    = 0;
static GLuint  bg_vao        = 0;
static GLuint  bg_vbo        = 0;
static GLint   bg_baseLoc    = -1;
static GLint   bg_timeLoc    = -1;
static GLint   bg_amountLoc  = -1;

static void bg_init()
{
    bg_program  = InitShader(SHADER_DIR "core/vshader_halo.glsl", SHADER_DIR "backgrounds/fshader_water_background.glsl");
    bg_baseLoc   = glGetUniformLocation(bg_program, "uBaseColor");
    bg_timeLoc   = glGetUniformLocation(bg_program, "uTime");
    bg_amountLoc = glGetUniformLocation(bg_program, "uAmount");

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

// ── Water / Ice palette ─────────────────────────────────────────────────────
static ObjColorPalette water_palette()
{
    ObjColorPalette p;
    p.xp = vec4(0.20f, 0.28f, 0.35f, 1.0f);
    p.xn = vec4(0.14f, 0.20f, 0.26f, 1.0f);
    p.yp = vec4(0.25f, 0.35f, 0.44f, 1.0f);
    p.yn = vec4(0.10f, 0.15f, 0.20f, 1.0f);
    p.zp = vec4(0.22f, 0.30f, 0.38f, 1.0f);
    p.zn = vec4(0.13f, 0.18f, 0.24f, 1.0f);
    p.generic = vec4(0.18f, 0.25f, 0.32f, 1.0f);
    return p;
}

void reutersvard_init()
{
    g_shape.init(SHADER_DIR"../models/reutersvard_rectangle.obj", water_palette(),
                 nullptr, SHADER_DIR "objects/fshader_water.glsl");

    g_shape.setCustomLight(vec3(0.45f, 0.60f, 0.70f));
    g_shape.setCustomLightColor(vec3(0.40f, 0.74f, 1.00f));

    bg_init();
}

void reutersvard_display()
{
    // Derive the solve-reveal completion amount (mirrors WATER_DELAY / WATER_TRACE
    // constants used in fshader_water.glsl).
    constexpr float WATER_DELAY = 0.7f;
    constexpr float WATER_TRACE = 6.0f;
    float waterT = g_shape.postSolveTime - WATER_DELAY;
    if (waterT < 0.0f) waterT = 0.0f;
    float amount = waterT / WATER_TRACE;
    if (amount > 1.0f) amount = 1.0f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the animated ocean background — stays neutral before solve,
    // gradually reveals as the water streams fill the cracks.
    glUseProgram(bg_program);
    glUniform3f(bg_baseLoc,   0.75f, 0.78f, 0.80f);   // pale slate base
    glUniform1f(bg_timeLoc,   (float)glfwGetTime());
    glUniform1f(bg_amountLoc, amount);
    glBindVertexArray(bg_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    g_shape.display();
}

void reutersvard_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void reutersvard_cursorPosCallback(GLFWwindow*, double x, double y)     { g_shape.cursorPos(x, y); }
void reutersvard_scrollCallback(GLFWwindow*, double, double y)          { g_shape.scroll(y); }
void reutersvard_keyCallback(GLFWwindow* w, int k, int, int a, int)     { g_shape.key(w, k, a); }
