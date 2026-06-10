#include "obj_shape.h"

// ─────────────────────────────────────────────────────────────────────────────
// Blocked Penrose (Zero-G Decomposition & Blueprint Illusion)
//
// Animation: "Deconstruction & Blueprint Illusion"
//   - Clean minimalist matte porcelain styling with elegant lighting.
//   - Loops every 13.0s:
//       1. [0.0s - 1.0s] Perfect Illusion: brief 1s hold after solve.
//       2. [1.0s - 4.0s] Subtle Shift: gaps open slightly at impossible corners.
//       3. [4.0s - 9.0s] Camera Drift & Zero-G Decomposition: orthographic camera
//          drifts away, while blocks scatter dramatically to random places,
//          undergoing independent scales, rotations, and zero-G bobbing in space.
//       4. [9.0s - 13.0s] Settle & Reconstruct back to seamless illusion.
//   - Background: premium mathematical blueprint grid (fades in on solve).
// ─────────────────────────────────────────────────────────────────────────────

static ObjShape g_shape;

// ── Blueprint Background ─────────────────────────────────────────────────────
static GLuint bg_program = 0;
static GLuint bg_vao = 0;
static GLuint bg_vbo = 0;
static GLint bg_baseLoc = -1;
static GLint bg_timeLoc = -1;
static GLint bg_amountLoc = -1;
static GLint bg_aspectLoc = -1;

static void bg_init()
{
    // Fullscreen quad with dark blue glow background shader
    bg_program = InitShader("../shaders/core/vshader_halo.glsl", "../shaders/backgrounds/fshader_dark_blue_glow.glsl");
    bg_baseLoc = glGetUniformLocation(bg_program, "uBaseColor");
    bg_timeLoc = glGetUniformLocation(bg_program, "uTime");
    bg_amountLoc = glGetUniformLocation(bg_program, "uAmount");
    bg_aspectLoc = glGetUniformLocation(bg_program, "uAspect");

    static const float quad[] = {
        -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
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

// ── Minimalist Porcelain Palette ──────────────────────────────────────────────
static ObjColorPalette minimalist_palette()
{
    ObjColorPalette p;
    p.xp = vec4(0.82f, 0.82f, 0.84f, 1.0f);  // warm light grey
    p.xn = vec4(0.70f, 0.70f, 0.72f, 1.0f);  // slightly darker
    p.yp = vec4(0.92f, 0.92f, 0.94f, 1.0f);  // brightest top face
    p.yn = vec4(0.50f, 0.50f, 0.52f, 1.0f);  // dark underside shadow
    p.zp = vec4(0.78f, 0.78f, 0.80f, 1.0f);  // mid grey
    p.zn = vec4(0.60f, 0.60f, 0.62f, 1.0f);  // darker back face
    p.generic = vec4(0.75f, 0.75f, 0.75f, 1.0f);
    return p;
}

// ── Geometry Data ────────────────────────────────────────────────────────────
struct BlockData
{
    int startVertex;
    int vertexCount;
    vec3 centroid;
    vec3 topCenter;
    vec3 topNormal;
};

// Extracted centroids and top faces of the 13 blocks in penrose_blocks.obj
static const BlockData g_blocks[13] = {
    {0, 30, vec3(5.2001f, 3.64386f, 5.55619f), vec3(5.72232f, 4.59941f, 5.51174f),
     vec3(0.666666f, 0.666667f, -0.333333f)},
    {30, 36, vec3(5.83343f, 2.37719f, 4.28952f), vec3(6.50009f, 3.04386f, 3.95619f),
     vec3(0.666666f, 0.666667f, -0.333333f)},
    {66, 36, vec3(6.66676f, 0.710527f, 2.62285f), vec3(6.33343f, 1.37719f, 3.28952f),
     vec3(-0.333333f, 0.666667f, 0.666667f)},
    {102, 36, vec3(5.00007f, 1.54387f, 0.956163f), vec3(4.66674f, 2.21054f, 1.62283f),
     vec3(-0.333334f, 0.666667f, 0.666667f)},
    {138, 36, vec3(3.33338f, 2.37722f, -0.710527f), vec3(3.00005f, 3.04388f, -0.0438602f),
     vec3(-0.333333f, 0.666667f, 0.666667f)},
    {174, 36, vec3(1.66669f, 3.21056f, -2.37722f), vec3(2.33336f, 3.87723f, -2.71055f),
     vec3(0.666667f, 0.666667f, -0.333333f)},
    {210, 36, vec3(0.00000f, 4.04391f, -4.04391f), vec3(0.666666f, 4.71058f, -4.37724f),
     vec3(0.666667f, 0.666667f, -0.333333f)},
    {246, 36, vec3(-1.66669f, 2.37722f, -3.21056f), vec3(-2.00002f, 3.04388f, -2.5439f),
     vec3(-0.333333f, 0.666667f, 0.666667f)},
    {282, 36, vec3(-3.33338f, 0.710527f, -2.37722f), vec3(-2.66672f, 1.37719f, -2.71055f),
     vec3(0.666667f, 0.666667f, -0.333333f)},
    {318, 36, vec3(-5.00007f, -0.956164f, -1.54387f), vec3(-5.33341f, -0.289497f, -0.877205f),
     vec3(-0.333333f, 0.666667f, 0.666667f)},
    {354, 36, vec3(-6.66676f, -2.62285f, -0.710526f), vec3(-7.0001f, -1.95619f, -0.0438597f),
     vec3(-0.333333f, 0.666667f, 0.666667f)},
    {390, 36, vec3(-5.83343f, -4.28952f, -2.37719f), vec3(-6.16676f, -3.62285f, -1.71053f),
     vec3(-0.333333f, 0.666667f, 0.666667f)},
    {426, 30, vec3(-5.2001f, -5.55619f, -3.64386f), vec3(-4.55565f, -4.84508f, -3.93275f),
     vec3(0.666666f, 0.666667f, -0.333333f)}};

// Expanded unique scatter destination offsets for each block
static const vec3 g_scatter_offsets[13] = {
    vec3(1.5f, 0.9f, 4.5f),     // Block 0
    vec3(2.8f, -1.8f, -3.6f),   // Block 1
    vec3(-1.8f, 2.2f, 3.2f),    // Block 2
    vec3(-3.0f, -0.5f, -2.5f),  // Block 3
    vec3(1.5f, 3.0f, 4.0f),     // Block 4
    vec3(3.2f, -2.0f, -4.2f),   // Block 5
    vec3(-2.2f, 2.8f, 3.5f),    // Block 6
    vec3(-0.5f, -3.2f, -3.0f),  // Block 7
    vec3(2.8f, 1.0f, 2.8f),     // Block 8
    vec3(-3.5f, -1.8f, -4.5f),  // Block 9
    vec3(1.0f, 3.5f, 3.8f),     // Block 10
    vec3(-2.0f, -2.8f, -3.2f),  // Block 11
    vec3(3.0f, 1.5f, 4.2f)      // Block 12
};

// Helper to construct axis-angle rotations
static mat4 RotateAxisAngle(vec3 axis, float angleDegrees)
{
    float rad = angleDegrees * (float)M_PI / 180.0f;
    return ObjShape::axisAngleRotationRad(axis, rad);
}

// Compute the custom model matrix for block i
static mat4 getBlockModelMatrix(int i, float shiftFactor, float decompFactor, const mat4& lockedRot, double now)
{
    vec3 outward = normalize(g_blocks[i].centroid);

    // Shift offset (Phase 2)
    vec3 shift_offset = outward * 0.18f;

    // Decomposition offset (Phase 3) - suspended at different locations
    vec3 decomp_offset = g_scatter_offsets[i] * 1.2f;

    // Floating bobbing (zero-G effect)
    float oscX = sinf((float)now * 1.5f + i * 1.0f) * 0.25f;
    float oscY = cosf((float)now * 1.2f + i * 1.5f) * 0.25f;
    float oscZ = sinf((float)now * 0.9f + i * 2.0f) * 0.25f;
    vec3 bobbing = vec3(oscX, oscY, oscZ) * decompFactor;

    vec3 total_disp = shift_offset * shiftFactor + decomp_offset * decompFactor + bobbing;

    // Rotations: minor shift rotations + large unique decomposition rotations
    float shift_rot_ang = shiftFactor * 2.5f;
    float decomp_rot_ang = decompFactor * (35.0f + 15.0f * sinf(i * 1.3f));
    float total_rot_ang = shift_rot_ang + decomp_rot_ang;

    // Rotation axis
    vec3 rot_axes[13] = {vec3(0, 1, 0),   vec3(1, 0, 0),  vec3(0, 0, 1),  vec3(1, 1, 0),  vec3(0, 1, 1),
                         vec3(1, 0, 1),   vec3(-1, 1, 0), vec3(0, -1, 1), vec3(1, -1, 0), vec3(1, 1, 1),
                         vec3(-1, -1, 1), vec3(0, 1, -1), vec3(1, 0, -1)};
    vec3 rot_axis = normalize(rot_axes[i]);

    // Scale variation (size shifts slightly when decomposed)
    float scale = 1.0f + decompFactor * 0.15f * sinf(i * 2.3f);

    // Translate to local centroid, scale & rotate & translate, translate back
    mat4 M_local = Translate(g_blocks[i].centroid.x, g_blocks[i].centroid.y, g_blocks[i].centroid.z);
    M_local = M_local * Translate(total_disp.x, total_disp.y, total_disp.z);
    if (total_rot_ang != 0.0f)
    {
        M_local = M_local * RotateAxisAngle(rot_axis, total_rot_ang);
    }
    M_local = M_local * Scale(scale, scale, scale);
    M_local = M_local * Translate(-g_blocks[i].centroid.x, -g_blocks[i].centroid.y, -g_blocks[i].centroid.z);

    return lockedRot * M_local;
}

// ── Lifecycle Callbacks ──────────────────────────────────────────────────────
void pbp_init()
{
    // Initialize shape using default impossible shader + minimalist palette
    g_shape.init("../models/penrose_blocks.obj", minimalist_palette());

    // Clean warm-white key light
    g_shape.setCustomLight(vec3(0.5f, 0.8f, 0.4f));
    g_shape.setCustomLightColor(vec3(1.00f, 0.98f, 0.95f));

    bg_init();
}

void pbp_display()
{
    float aspect = (screen_w > 0 && screen_h > 0) ? (float)screen_w / (float)screen_h : 1.0f;

    // ── Normal Play Loop (unsolved / transitioning) ──
    if (!g_shape.isSolved())
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw neutral slate backdrop (amount = 0.0)
        glUseProgram(bg_program);
        glUniform3f(bg_baseLoc, 0.05f, 0.05f, 0.08f);
        glUniform1f(bg_timeLoc, (float)glfwGetTime());
        glUniform1f(bg_amountLoc, 0.0f);
        glUniform1f(bg_aspectLoc, aspect);
        glBindVertexArray(bg_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        g_shape.display();
        return;
    }

    // ── Locked looping animation takeover (13.0s loop) ──
    double now = glfwGetTime();
    constexpr float PERIOD = 13.0f;
    float t = fmod(now - g_shape.phaseStartTime, PERIOD);

    float shiftFactor = 0.0f;
    float decompFactor = 0.0f;
    float cameraYOffset = 0.0f;
    float cameraXOffset = 0.0f;

    // Accelerating timeline: 1.0s hold, 3.0s shift, 5.0s float, 4.0s settle
    if (t >= 0.0f && t < 1.0f)
    {
        // Phase 1: Perfect Illusion (1s brief hold)
        shiftFactor = 0.0f;
        decompFactor = 0.0f;
    }
    else if (t >= 1.0f && t < 4.0f)
    {
        // Phase 2: Subtle Shifting
        if (t < 2.5f)
        {
            float k = (t - 1.0f) / 1.5f;
            shiftFactor = k * k * (3.0f - 2.0f * k);  // smoothstep
        }
        else
        {
            shiftFactor = 1.0f;
        }
        decompFactor = 0.0f;
    }
    else if (t >= 4.0f && t < 9.0f)
    {
        // Phase 3: Camera Drift & Zero-G Decomposition
        shiftFactor = 1.0f;
        float driftTime = (t - 4.0f) / 5.0f;
        float driftFactor = sinf(driftTime * (float)M_PI);
        decompFactor = driftFactor;
        cameraYOffset = driftFactor * 28.0f;
        cameraXOffset = driftFactor * 14.0f;
    }
    else
    {
        // Phase 4: Settle & Reconstruct
        if (t < 10.5f)
        {
            float k = (t - 9.0f) / 1.5f;
            shiftFactor = 1.0f - k * k * (3.0f - 2.0f * k);
        }
        else
        {
            shiftFactor = 0.0f;
        }
        decompFactor = 0.0f;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Draw premium mathematical blueprint grid background
    glUseProgram(bg_program);
    glUniform3f(bg_baseLoc, 0.05f, 0.05f, 0.08f);
    glUniform1f(bg_timeLoc, (float)now);
    // Fade blueprint in over 1.5 seconds from solve
    float amount = fminf(g_shape.postSolveTime / 1.5f, 1.0f);
    glUniform1f(bg_amountLoc, amount);
    glUniform1f(bg_aspectLoc, aspect);
    glBindVertexArray(bg_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    // 2. Compute camera view & projection (orthographic)
    vec3 base_eye = g_shape.cameraEye;
    vec4 eye4 = RotateY(cameraYOffset) * RotateX(cameraXOffset) * vec4(base_eye.x, base_eye.y, base_eye.z, 1.0f);
    vec3 eye(eye4.x, eye4.y, eye4.z);

    mat4 view = LookAt(eye, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    mat4 projection = Ortho(-g_shape.orthoSize * aspect, g_shape.orthoSize * aspect, -g_shape.orthoSize,
                            g_shape.orthoSize, 0.1f, 200.0f);

    // 3. Draw individual Blocks
    mat4 lockedRot = RotateY(g_shape.angleY) * RotateX(g_shape.angleX) * RotateZ(g_shape.angleZ);

    glUseProgram(g_shape.shaderProgram);
    glBindVertexArray(g_shape.vao);

    glUniformMatrix4fv(g_shape.viewLoc, 1, GL_FALSE, &view.d[0].x);
    glUniformMatrix4fv(g_shape.projLoc, 1, GL_FALSE, &projection.d[0].x);

    // Lighting uniforms matching camera movement
    vec3 lightPos = vec3(eye.x * 0.8f, eye.y * 1.6f, eye.z * 1.2f);
    glUniform3fv(g_shape.lightLoc, 1, &lightPos.x);
    if (g_shape.lightColorLoc >= 0) glUniform3fv(g_shape.lightColorLoc, 1, &g_shape.customLightColor.x);
    glUniform3fv(g_shape.eyeLoc, 1, &eye.x);
    glUniform1f(g_shape.timeLoc, (float)now);
    glUniform1f(g_shape.heightLoc, g_shape.objHeight);
    if (g_shape.lockGlowLoc >= 0) glUniform1f(g_shape.lockGlowLoc, 0.0f);
    if (g_shape.isBallLoc >= 0) glUniform1i(g_shape.isBallLoc, 0);

    for (int i = 0; i < 13; ++i)
    {
        mat4 blockModel = getBlockModelMatrix(i, shiftFactor, decompFactor, lockedRot, now);
        glUniformMatrix4fv(g_shape.modelLoc, 1, GL_FALSE, &blockModel.d[0].x);
        glDrawArrays(GL_TRIANGLES, g_blocks[i].startVertex, g_blocks[i].vertexCount);
    }

    glFinish();
}

void pbp_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void pbp_cursorPosCallback(GLFWwindow*, double x, double y) { g_shape.cursorPos(x, y); }
void pbp_scrollCallback(GLFWwindow*, double, double y) { g_shape.scroll(y); }
void pbp_keyCallback(GLFWwindow* w, int k, int, int a, int) { g_shape.key(w, k, a); }
