#include "obj_shape.h"

// ─────────────────────────────────────────────────────────────────────────────
// Penrose Staircase — loaded from models/penrose_stair.obj.
//
// Animation: "Clockwork & Kintsugi".
//   * The object starts as dark Obsidian stone.
//   * Upon solve, procedural Voronoi cracks form and fill with glowing liquid gold
//     flowing outward from the center.
//   * The background fades into a deep space void filled with 3D ray-marched
//     rotating clockwork gears, symbolizing the infinite loop of the stairs.
// ─────────────────────────────────────────────────────────────────────────────

static ObjShape g_shape;

// ── Clockwork Background ──────────────────────────────────────────────────────
static GLuint  bg_program    = 0;
static GLuint  bg_vao        = 0;
static GLuint  bg_vbo        = 0;
static GLint   bg_baseLoc    = -1;
static GLint   bg_timeLoc    = -1;
static GLint   bg_amountLoc  = -1;

static void bg_init()
{
    bg_program  = InitShader("../shaders/core/vshader_halo.glsl",
                             "../shaders/backgrounds/fshader_clockwork_bg.glsl");
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

// ── Obsidian Palette ─────────────────────────────────────────────────────────
// Dark, nearly black stone to provide maximum contrast for the liquid gold.
static ObjColorPalette obsidian_palette()
{
    ObjColorPalette p;
    p.xp = vec4(0.12f, 0.12f, 0.14f, 1.0f);   
    p.xn = vec4(0.08f, 0.08f, 0.10f, 1.0f);
    p.yp = vec4(0.16f, 0.16f, 0.18f, 1.0f);   
    p.yn = vec4(0.06f, 0.06f, 0.07f, 1.0f);
    p.zp = vec4(0.14f, 0.14f, 0.16f, 1.0f);   
    p.zn = vec4(0.07f, 0.07f, 0.08f, 1.0f);
    p.generic = vec4(0.10f, 0.10f, 0.12f, 1.0f);
    return p;
}

void penrose_block_init()
{
    g_shape.init("../models/penrose_stair.obj", obsidian_palette(), nullptr, "../shaders/objects/fshader_kintsugi.glsl");

    // Warm golden key light to accent the obsidian and gold
    g_shape.setCustomLight(vec3(0.35f, 0.65f, 0.45f));
    g_shape.setCustomLightColor(vec3(1.00f, 0.85f, 0.50f));

    // Spin the stair 25° around Y 
    g_shape.angleY        = 25.0f;
    g_shape.defaultAngleY = 25.0f;

    bg_init();
}

void penrose_block_display() 
{ 
    constexpr float KINTSUGI_DELAY = 0.5f;
    constexpr float KINTSUGI_TRACE = 5.0f;
    float goldT = g_shape.postSolveTime - KINTSUGI_DELAY;
    if (goldT < 0.0f) goldT = 0.0f;
    float amount = goldT / KINTSUGI_TRACE;
    if (amount > 1.0f) amount = 1.0f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(bg_program);
    glUniform3f(bg_baseLoc,   0.75f, 0.78f, 0.80f);   // pale slate base
    glUniform1f(bg_timeLoc,   (float)glfwGetTime());
    glUniform1f(bg_amountLoc, amount);
    glBindVertexArray(bg_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    g_shape.display(); 
}

void penrose_block_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void penrose_block_cursorPosCallback(GLFWwindow*, double x, double y) { g_shape.cursorPos(x, y); }
void penrose_block_scrollCallback(GLFWwindow*, double, double y) { g_shape.scroll(y); }
void penrose_block_keyCallback(GLFWwindow* w, int k, int, int a, int) { g_shape.key(w, k, a); }
