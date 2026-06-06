#include "obj_shape.h"

// Impossible Cube — loaded from models/impossible_cube.obj
// (Paradox Toolkit primitive). Earthy/Mossy effect variant.

static ObjShape g_shape;

// ── Forest Dapple Background (fades in when finished) ────────────────────────
static GLuint  bg_program          = 0;
static GLuint  bg_vao              = 0;
static GLuint  bg_vbo              = 0;
static GLint   bg_baseLoc          = -1;
static GLint   bg_timeLoc          = -1;
static GLint   bg_amountLoc        = -1;

static void bg_init()
{
    // Reuses the fullscreen quad vertex shader with our new earth background fragment shader
    bg_program = InitShader("../shaders/vshader_halo.glsl",
                            "../shaders/fshader_earth_background.glsl");
    bg_baseLoc   = glGetUniformLocation(bg_program, "uBaseColor");
    bg_timeLoc   = glGetUniformLocation(bg_program, "uTime");
    bg_amountLoc = glGetUniformLocation(bg_program, "uAmount");

    // Fullscreen triangle strip — 4 NDC corners.
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

// ── Earth / Moss palette ─────────────────────────────────────────────────────
// Warm organic brown/terracotta soil colors.
static ObjColorPalette earth_palette()
{
    ObjColorPalette p;
    p.xp = vec4(0.40f, 0.28f, 0.22f, 1.0f);   // X+ — terracotta/soil
    p.xn = vec4(0.30f, 0.21f, 0.16f, 1.0f);   // X- — darker brown
    p.yp = vec4(0.48f, 0.35f, 0.27f, 1.0f);   // Y+ — top soil
    p.yn = vec4(0.20f, 0.14f, 0.11f, 1.0f);   // Y- — shadow underside
    p.zp = vec4(0.42f, 0.30f, 0.24f, 1.0f);   // Z+ — soil
    p.zn = vec4(0.25f, 0.18f, 0.14f, 1.0f);   // Z-
    p.generic = vec4(0.35f, 0.25f, 0.20f, 1.0f);
    return p;
}

void cube_init()
{
    // Load mesh using the earth palette and our dedicated earth fragment shader
    g_shape.init("../models/impossible_cube.obj", earth_palette(), nullptr, "../shaders/fshader_earth.glsl");

    // Warm golden-green light direction and color (triggers earth shader branch)
    g_shape.setCustomLight(vec3(0.30f, 0.80f, 0.52f));
    g_shape.setCustomLightColor(vec3(0.70f, 1.00f, 0.50f));

    // Pull the ortho frustum out a bit so it appears at a similar on-screen size
    g_shape.orthoSize = 12.5f;

    bg_init();
}

void cube_display()
{
    // ── Calculate solve reveal completion amount ──
    constexpr float EARTH_DELAY = 0.7f;
    constexpr float EARTH_TRACE = 6.0f;
    float earthT  = g_shape.postSolveTime - EARTH_DELAY;
    if (earthT < 0.0f) earthT = 0.0f;
    float amount = earthT / EARTH_TRACE;
    if (amount > 1.0f) amount = 1.0f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ── Render the forest dapple background (fades in as amount increases) ──
    glUseProgram(bg_program);
    glUniform3f(bg_baseLoc,    0.75f, 0.78f, 0.80f);   // pale slate base
    glUniform1f(bg_timeLoc,    (float)glfwGetTime());
    glUniform1f(bg_amountLoc,  amount);
    glBindVertexArray(bg_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    g_shape.display();
}

void cube_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void cube_cursorPosCallback(GLFWwindow*, double x, double y) { g_shape.cursorPos(x, y); }
void cube_scrollCallback(GLFWwindow*, double, double y) { g_shape.scroll(y); }
void cube_keyCallback(GLFWwindow* w, int k, int, int a, int) { g_shape.key(w, k, a); }
