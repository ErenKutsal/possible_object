#include "obj_shape.h"
#include <vector>
#include <cmath>

static ObjShape g_shape;

// ── Background Shading ──────────────────────────────────────────────────────
static GLuint  bg_program    = 0;
static GLuint  bg_vao        = 0;
static GLuint  bg_vbo        = 0;

static void bg_init()
{
    bg_program = InitShader("../shaders/core/vshader_halo.glsl",
                            "../shaders/backgrounds/fshader_minimal_background.glsl");

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
    if (aPos >= 0)
    {
        glEnableVertexAttribArray(aPos);
        glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    }
    glBindVertexArray(0);
}

// ── Glowing Tracing Lines ───────────────────────────────────────────────────
static GLuint line_vao = 0;
static GLuint line_vbo = 0;
static GLuint line_program = 0;
static GLint  line_modelLoc = -1;
static GLint  line_viewLoc = -1;
static GLint  line_projLoc = -1;
static GLint  line_pulsePos1Loc = -1;
static GLint  line_pulseIntensity1Loc = -1;
static GLint  line_pulsePos2Loc = -1;
static GLint  line_pulseIntensity2Loc = -1;
static GLint  line_totalLengthLoc = -1;
static GLint  line_colorLoc = -1;

static GLint  isBallLoc = -1;
static GLint  ballColorLoc = -1;

static void line_init()
{
    line_program = InitShader("../shaders/objects/vshader_line.glsl",
                              "../shaders/objects/fshader_line.glsl");
    line_modelLoc = glGetUniformLocation(line_program, "model");
    line_viewLoc = glGetUniformLocation(line_program, "view");
    line_projLoc = glGetUniformLocation(line_program, "projection");
    line_pulsePos1Loc = glGetUniformLocation(line_program, "uPulsePos1");
    line_pulseIntensity1Loc = glGetUniformLocation(line_program, "uPulseIntensity1");
    line_pulsePos2Loc = glGetUniformLocation(line_program, "uPulsePos2");
    line_pulseIntensity2Loc = glGetUniformLocation(line_program, "uPulseIntensity2");
    line_totalLengthLoc = glGetUniformLocation(line_program, "uTotalLength");
    line_colorLoc = glGetUniformLocation(line_program, "uColor");

    // Line vertices: 3D position (xyz) and cumulative arc-length (w)
    static const float line_vertices[] = {
        // Strip 1: Left-back vertical beam C
        -6.0f, -3.0f, -6.0f,  0.0f,
        -6.0f, -7.5f, -6.0f,  4.5f,

        // Strip 2: Right-front vertical beam E, bottom Z-beam D, right Y-beam A, top X-beam B
        6.0f,  4.5f,  6.0f,  4.5f,
        6.0f,  0.0f,  6.0f,  9.0f,
        6.0f,  0.0f, -6.0f,  21.0f,
        6.0f, -3.0f, -6.0f,  24.0f,
       -6.0f, -3.0f, -6.0f,  36.0f
    };

    glGenVertexArrays(1, &line_vao);
    glBindVertexArray(line_vao);
    glGenBuffers(1, &line_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_STATIC_DRAW);

    GLint vPos = glGetAttribLocation(line_program, "vPosition");
    if (vPos >= 0)
    {
        glEnableVertexAttribArray(vPos);
        glVertexAttribPointer(vPos, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    }
    glBindVertexArray(0);
}

// ── Minimal Palette ─────────────────────────────────────────────────────────
static ObjColorPalette minimal_palette()
{
    ObjColorPalette p;
    p.xp = vec4(0.88f, 0.89f, 0.92f, 1.0f);
    p.xn = vec4(0.76f, 0.77f, 0.80f, 1.0f);
    p.yp = vec4(0.94f, 0.95f, 0.98f, 1.0f);
    p.yn = vec4(0.68f, 0.70f, 0.73f, 1.0f);
    p.zp = vec4(0.82f, 0.83f, 0.86f, 1.0f);
    p.zn = vec4(0.72f, 0.73f, 0.76f, 1.0f);
    p.generic = vec4(0.80f, 0.81f, 0.84f, 1.0f);
    return p;
}

// Helper to interpolate position along the 3D tracing path
static vec3 get_pulse_3d_position(float d)
{
    if (d < 4.5f)
    {
        float t = d / 4.5f;
        vec3 p0(-6.0f, -3.0f, -6.0f);
        vec3 p1(-6.0f, -7.5f, -6.0f);
        return vec3(p0.x + t*(p1.x-p0.x), p0.y + t*(p1.y-p0.y), p0.z + t*(p1.z-p0.z));
    }
    else if (d < 9.0f)
    {
        float t = (d - 4.5f) / 4.5f;
        vec3 p2(6.0f, 4.5f, 6.0f);
        vec3 p3(6.0f, 0.0f, 6.0f);
        return vec3(p2.x + t*(p3.x-p2.x), p2.y + t*(p3.y-p2.y), p2.z + t*(p3.z-p2.z));
    }
    else if (d < 21.0f)
    {
        float t = (d - 9.0f) / 12.0f;
        vec3 p3(6.0f, 0.0f, 6.0f);
        vec3 p4(6.0f, 0.0f, -6.0f);
        return vec3(p3.x + t*(p4.x-p3.x), p3.y + t*(p4.y-p3.y), p3.z + t*(p4.z-p3.z));
    }
    else if (d < 24.0f)
    {
        float t = (d - 21.0f) / 3.0f;
        vec3 p4(6.0f, 0.0f, -6.0f);
        vec3 p5(6.0f, -3.0f, -6.0f);
        return vec3(p4.x + t*(p5.x-p4.x), p4.y + t*(p5.y-p4.y), p4.z + t*(p5.z-p4.z));
    }
    else
    {
        float t = (d - 24.0f) / 12.0f;
        vec3 p5(6.0f, -3.0f, -6.0f);
        vec3 p6(-6.0f, -3.0f, -6.0f);
        return vec3(p5.x + t*(p6.x-p5.x), p5.y + t*(p6.y-p5.y), p5.z + t*(p6.z-p5.z));
    }
}

static void draw_tracer_sphere(const vec3& pos, float intensity, const mat4& rotPart, const mat4& view, const mat4& proj, const vec3& color = vec3(0.0f, 0.85f, 1.0f))
{
    if (intensity < 0.01f) return;

    float radius = 0.22f;
    mat4 scale = Scale(radius, radius, radius);
    mat4 finalModel = rotPart * Translate(pos.x, pos.y, pos.z) * scale;

    glUseProgram(g_shape.shaderProgram);
    glUniformMatrix4fv(g_shape.modelLoc, 1, GL_FALSE, &finalModel.d[0].x);
    glUniformMatrix4fv(g_shape.viewLoc, 1, GL_FALSE, &view.d[0].x);
    glUniformMatrix4fv(g_shape.projLoc, 1, GL_FALSE, &proj.d[0].x);

    if (isBallLoc >= 0) glUniform1i(isBallLoc, 1);
    if (ballColorLoc >= 0) glUniform3f(ballColorLoc, color.x, color.y, color.z);

    glBindVertexArray(g_shape.ballVao);
    glDrawArrays(GL_TRIANGLES, 0, g_shape.ballVertexCount);

    if (isBallLoc >= 0) glUniform1i(isBallLoc, 0);
}

// ── API Functions ───────────────────────────────────────────────────────────

void reutersvard_init()
{
    // Initialize standard ObjShape using the custom minimalist fragment shader
    g_shape.init("../models/reutersvard_rectangle.obj", minimal_palette(),
                 nullptr, "../shaders/objects/fshader_reutersvard_minimal.glsl");

    isBallLoc = glGetUniformLocation(g_shape.shaderProgram, "uIsBall");
    ballColorLoc = glGetUniformLocation(g_shape.shaderProgram, "uBallColor");

    // Force ObjShape to generate a sphere mesh for the tracer
    g_shape.generateBallSphere();

    // Enable locked camera orbit on the shape state
    g_shape.allowLockedOrbit = true;

    // Note: default unsolved starting angles (40, 25, -30) are preserved automatically

    bg_init();
    line_init();
}

static double lastFrameTime = 0.0;
static float  transition_t = 0.0f; // 1.0 = fully automatic orbit, 0.0 = manual drag
static float  lastUserAngleX = 0.0f;
static float  lastUserAngleY = 0.0f;
static float  lastUserAngleZ = 0.0f;
static double animTimeAccumulator = 0.0;

void reutersvard_display()
{
    double now = glfwGetTime();
    float dt = (lastFrameTime > 0.0) ? (float)(now - lastFrameTime) : 0.0f;
    lastFrameTime = now;
    if (dt > 0.1f) dt = 0.1f;

    // ── 1. Calculate Orbit / Drag Easing if Locked ──────────────────────────
    if (g_shape.solvePhase == ObjShape::SolvePhase::Locked)
    {
        double lockTime = now - g_shape.phaseStartTime;
        if (lockTime < ObjShape::LOCK_PULSE_DURATION)
        {
            // Lock pulse is playing: freeze orbit angles at magic target (0,0,0)
            animTimeAccumulator = 0.0;
            transition_t = 1.0f;
        }
        else
        {
            // Advance animation timer
            animTimeAccumulator += dt;

            // Compute the target orbit angles
            float orbitAngleX = 0.0f;
            float orbitAngleY = 0.0f;
            double t_anim = fmod(animTimeAccumulator, 20.0);

            if (t_anim < 6.0)
            {
                orbitAngleX = 0.0f;
                orbitAngleY = 0.0f;
            }
            else if (t_anim < 11.0)
            {
                float u = (float)((t_anim - 6.0) / 5.0);
                float ease = 0.5f - 0.5f * cosf(u * (float)M_PI);
                orbitAngleX = ease * -15.0f;
                orbitAngleY = ease * 35.0f;
            }
            else if (t_anim < 14.0)
            {
                orbitAngleX = -15.0f;
                orbitAngleY = 35.0f;
            }
            else
            {
                float u = (float)((t_anim - 14.0) / 6.0);
                float ease = 0.5f - 0.5f * cosf(u * (float)M_PI);
                orbitAngleX = (1.0f - ease) * -15.0f;
                orbitAngleY = (1.0f - ease) * 35.0f;
            }

            if (g_shape.isDragging)
            {
                transition_t = 0.0f;
                lastUserAngleX = g_shape.angleX;
                lastUserAngleY = g_shape.angleY;
                lastUserAngleZ = g_shape.angleZ;
            }
            else
            {
                transition_t = fminf(transition_t + dt * 1.5f, 1.0f);
                g_shape.angleX = (1.0f - transition_t) * lastUserAngleX + transition_t * orbitAngleX;
                g_shape.angleY = (1.0f - transition_t) * lastUserAngleY + transition_t * orbitAngleY;
                g_shape.angleZ = (1.0f - transition_t) * lastUserAngleZ;
            }
        }
    }
    else
    {
        // Unsolved: reset loop parameters so the animation starts fresh upon lock
        animTimeAccumulator = 0.0;
        transition_t = 0.0f;
    }

    // ── 2. Background Pass ──────────────────────────────────────────────────
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glUseProgram(bg_program);
    glBindVertexArray(bg_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    // ── 3. Render Minimalist Mesh (handles standard mouse interaction) ──────
    g_shape.display();

    // ── 4. Render Glowing Tracing Lines (only visible when solved/locked) ───
    if (g_shape.solvePhase == ObjShape::SolvePhase::Locked)
    {
        float misalignment = g_shape.isoAxisMisalignmentDeg();
        float hesitation_factor = fmaxf(0.0f, fminf(misalignment / 15.0f, 1.0f));

        float v_pulse = 9.0f; // units per second
        float D_hes = 0.6f * hesitation_factor; // pause up to 0.6s at the gap
        float T_cycle = 4.0f + D_hes;
        float t_phase = fmodf((float)now, T_cycle);

        float d1 = 0.0f, int1 = 0.0f;
        float d2 = 0.0f, int2 = 0.0f;

        if (t_phase < 0.5f)
        {
            d1 = t_phase * v_pulse;
            int1 = 1.0f;
        }
        else if (t_phase < 0.5f + D_hes)
        {
            d1 = 4.5f;
            float factor = (t_phase - 0.5f) / D_hes;
            int1 = 1.0f - factor;
            d2 = 4.5f;
            int2 = factor;
        }
        else
        {
            d2 = (t_phase - D_hes) * v_pulse;
            int2 = 1.0f;
        }

        mat4 rotPart = RotateY(g_shape.angleY) * RotateX(g_shape.angleX) * RotateZ(g_shape.angleZ);
        mat4 view = g_shape.getViewMatrix();
        mat4 proj = g_shape.getProjectionMatrix();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(line_program);
        glUniformMatrix4fv(line_modelLoc, 1, GL_FALSE, &rotPart.d[0].x);
        glUniformMatrix4fv(line_viewLoc, 1, GL_FALSE, &view.d[0].x);
        glUniformMatrix4fv(line_projLoc, 1, GL_FALSE, &proj.d[0].x);

        glUniform1f(line_pulsePos1Loc, d1);
        glUniform1f(line_pulseIntensity1Loc, int1);
        glUniform1f(line_pulsePos2Loc, d2);
        glUniform1f(line_pulseIntensity2Loc, int2);
        glUniform1f(line_totalLengthLoc, 36.0f);
        glUniform3f(line_colorLoc, 0.55f, 0.60f, 0.70f); // brighter baseline color

        glBindVertexArray(line_vao);

        // Pass 1: Semi-transparent wide halo glow (increased from 5.5 to 9.0)
        glLineWidth(9.0f);
        glDrawArrays(GL_LINE_STRIP, 0, 2);
        glDrawArrays(GL_LINE_STRIP, 2, 5);

        // Pass 2: High-contrast thin core (increased from 1.8 to 3.0)
        glLineWidth(3.0f);
        glDrawArrays(GL_LINE_STRIP, 0, 2);
        glDrawArrays(GL_LINE_STRIP, 2, 5);

        // ── 5. Render Glowing Tracer Beads ─────────────────────────────────────
        draw_tracer_sphere(get_pulse_3d_position(d1), int1, rotPart, view, proj, vec3(1.0f, 0.12f, 0.12f));        // first bead — red
        draw_tracer_sphere(get_pulse_3d_position(d2), int2, rotPart, view, proj, vec3(1.0f, 0.12f, 0.12f));        // second bead — red

        glDisable(GL_BLEND);
        glBindVertexArray(0);
    }
}

void reutersvard_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void reutersvard_cursorPosCallback(GLFWwindow*, double x, double y)     { g_shape.cursorPos(x, y); }
void reutersvard_scrollCallback(GLFWwindow*, double, double y)          { g_shape.scroll(y); }
void reutersvard_keyCallback(GLFWwindow* w, int k, int, int a, int)     { g_shape.key(w, k, a); }
