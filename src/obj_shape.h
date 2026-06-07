#pragma once
#include "includes.h"
#include "obj_loader.h"

// ─────────────────────────────────────────────────────────────────────────────
// ObjShape — bundles per-shape OpenGL state + interaction for an OBJ figure.
//
// Each impossible figure that comes from a .obj file in models/ owns one
// ObjShape instance. The per-shape .cpp files (penrose.cpp, neckercube.cpp,
// ...) are now thin wrappers that delegate their init/display/callback
// functions to a static ObjShape inside that file.
//
// This struct exists so the boilerplate (load OBJ → make VAO/VBO → compile
// shader → render with axonometric matrix → handle mouse/key rotation) lives
// in exactly one place.
// ─────────────────────────────────────────────────────────────────────────────

struct ObjShape
{
    // OpenGL handles
    GLuint shaderProgram = 0;
    GLuint vao = 0;
    GLuint positionBuffer = 0;
    GLuint colorBuffer = 0;
    GLuint modelLoc = 0, viewLoc = 0, projLoc = 0;
    GLint  lightLoc = -1, eyeLoc = -1, timeLoc = -1, heightLoc = -1;
    GLint  lightColorLoc = -1;   // per-slot light tint (Tier 2); -1 if shader lacks uLightColor
    GLint  lockGlowLoc = -1;
    // Slot-6-specific uniforms (only present in the iridescent shader; any
    // ObjShape that uses the default shader leaves these at -1 and skips
    // sending them).
    GLint  postSolveTimeLoc   = -1;
    GLint  breathAmountLoc    = -1;
    GLint  hueShiftLoc        = -1;
    GLint  iridescenceAmtLoc  = -1;
    GLint  skyboxRotationLoc  = -1;
    // Tracked CPU-side so the shader can animate after LOCKED. Reset to 0
    // whenever the solve phase leaves LOCKED.
    float  postSolveTime      = 0.0f;
    // Slot 6 (chrome) tells us its current skybox rotation; we forward it
    // to the chrome shader so its reflection samples match what the skybox
    // is showing. Other slots leave this at 0 (and the uniform doesn't
    // exist on their shader so the send is skipped).
    float  externalSkyboxRotation = 0.0f;
    void   setSkyboxRotation(float r) { externalSkyboxRotation = r; }
    bool   isSolved() const { return solvePhase == SolvePhase::Locked; }

    // Optional override for the direct light's world position. The default
    // (eye-relative) is used when useCustomLight is false. Slot 6 enables
    // this and feeds the position of its orbiting visible light marker so
    // the chrome's specular highlight clearly tracks that light.
    bool   useCustomLight   = false;
    vec3   customLightPos   = vec3(0.0f, 0.0f, 0.0f);
    void   setCustomLight(const vec3& p) { useCustomLight = true; customLightPos = p; }

    // Optional per-slot light COLOR (Tier 2). Defaults to white (neutral) so
    // slots that don't set it look exactly as before. A warm/cool tint gives
    // each level its own mood. This is a single global value per draw, so it
    // never reintroduces a seam at the solved join (see fshader_impossible).
    vec3   customLightColor = vec3(1.0f, 1.0f, 1.0f);
    void   setCustomLightColor(const vec3& c) { customLightColor = c; }
    GLint  isBallLoc = -1;

    // ── Ball mesh (per-instance — owns a sphere VAO for the indicator) ────
    GLuint ballVao = 0, ballPosBuffer = 0, ballColorBuffer = 0;
    int    ballVertexCount = 0;
    float  ballRadiusWorld = 0.30f;   // sphere radius in OBJ-space units (tuned per slot via setBallPath)

    // ── Path-based ball control (active while locked) ──────────────────────
    // Each shape gets a hand-defined LOOP path made of straight segments.
    // ball_s ∈ [0, 1) is arc-length-parameterized over the whole loop.
    // ball_u ∈ [0, 4) is the cross-section position (inner/top/outer/bot,
    // matching slot 1's diamond convention but interpreted with the
    // PER-SEGMENT up/right axes so the ball rolls on the bar's surface).
    //
    // WASD when LOCKED:
    //   W/S  → forward / backward along the loop
    //   D/A  → CW / CCW around the bar's cross-section

    // A waypoint = mesh-local position + outward "up" direction (face normal).
    struct PathWaypoint { vec3 pos; vec3 up; };

    struct PathSegment {
        vec3 start;        // OBJ-space (post-centering) endpoint
        vec3 end;          // OBJ-space endpoint
        vec3 up;           // local +z (top of bar) — must be perpendicular to (end-start)
        // cached
        float length;
        vec3  forward;     // (end-start)/length
        vec3  right;       // cross(forward, up) — local +y (outer side of bar)
    };
    std::vector<PathSegment> ballPath;
    float ballPathTotalLength = 0.0f;
    float ballThickness       = 1.0f;   // half-thickness of the bar's cross-section

    bool   key_w_held = false, key_a_held = false;
    bool   key_s_held = false, key_d_held = false;
    double last_wasd_input_time = 0.0;
    float  ball_s = 0.0f;
    float  ball_u = 1.5f;
    static constexpr float BALL_S_SPEED              = 0.18f;   // loops per second (slower for OBJ scale)
    static constexpr float BALL_U_SPEED              = 1.30f;   // perimeter units per second
    static constexpr float IDLE_DRIFT_THRESHOLD_SEC  = 1.5f;

    // ── Solve animation state (three-phase) ──────────────────────────────
    // When the player aligns the bisect cuts (iso axis preserved), we play:
    //   1. FOUND     — hold at the user's current pose, soft glow swell so
    //                  they see "yes, you found it!" at the angle they
    //                  discovered. No rotation yet.
    //   2. ROTATING  — slow ease from the user's pose to the canonical S
    //                  pose (identity). Glow held steady during the trip.
    //   3. LOCKED    — at S, play the satisfying scale-bump + brightness
    //                  pulse, then settle.
    // Pressing S manually skips straight to ROTATING. Dragging off the
    // alignment during FOUND aborts the whole thing.
    enum class SolvePhase { None, Found, Rotating, Locked };
    SolvePhase solvePhase     = SolvePhase::None;
    double     phaseStartTime = 0.0;
    float      animStartX     = 0.0f;
    float      animStartY     = 0.0f;
    float      animStartZ     = 0.0f;

    // ROTATING animates via quaternion SLERP from the user's pose to a
    // family-matched TARGET pose:
    //   - F+ family (R·iso ≈ +iso): target = identity (the canonical "S" pose).
    //   - F- family (R·iso ≈ -iso): target = a chosen 180° rotation B_POSE
    //     about (1,-1,0)/√2. Both points sit on the F- great circle of the
    //     quaternion 3-sphere, so the slerp arc stays within F- the WHOLE
    //     way — cuts never re-separate during the animation. (Slerp between
    //     two points on the F+ great circle has the same property for F+.)
    //
    // After ROTATING the figure ends up at either S (identity Euler 0/0/0)
    // or B (R_B in Euler coords) — both are "solved" poses where the
    // illusion is visible. Sync angleX/Y/Z to B at the end so subsequent
    // drags resume cleanly from the B pose.
    float rotateStartQ_x = 0.0f, rotateStartQ_y = 0.0f;
    float rotateStartQ_z = 0.0f, rotateStartQ_w = 1.0f;
    float rotateCurQ_x   = 0.0f, rotateCurQ_y   = 0.0f;
    float rotateCurQ_z   = 0.0f, rotateCurQ_w   = 1.0f;
    // Target quaternion (set at start of ROTATING).
    float rotateTargetQ_x = 0.0f, rotateTargetQ_y = 0.0f;
    float rotateTargetQ_z = 0.0f, rotateTargetQ_w = 1.0f;
    bool  rotateTargetIsB = false;  // true → landed at B (back) pose, not S

    static constexpr float FOUND_DURATION      = 0.55f;   // hold at user pose
    static constexpr float ROTATE_DURATION     = 1.45f;   // slow ease to S
    static constexpr float LOCK_PULSE_DURATION = 1.00f;   // bump+glow at S

    // Auto-snap tolerance is measured against the ISO AXIS (1,1,1)/√3 — the
    // cuts the Paradox toolkit bakes into each figure are perpendicular to
    // that direction, so any rotation that keeps the iso axis pointing at
    // the camera makes the cuts project to lines. Identity is one such pose;
    // every rotation about (1,1,1) is equivalent for the illusion.
    //
    // Tightened from 7° — the auto-snap was firing before the player felt
    // they'd ACTUALLY found the alignment. 3.5° is "you really got it".
    static constexpr float AUTO_SNAP_ISO_TOL_DEG = 3.5f;  // illusion-visible tolerance
    static constexpr float UNLOCK_ISO_TOL_DEG    = 12.0f; // hysteresis — drag well off to release

    // CPU-side mesh data
    std::vector<vec4> positions;
    std::vector<vec4> colors;

    // PUZZLE STARTING POSITION (the "unsolved" pose) — clearly off the magic
    // axonometric so the illusion is broken and the player has to rotate to
    // find the angle where the cut sides "touch". The actual solved angle is
    // (54.736°, 0°, -45°) — that's what S-key snaps to.
    //
    // Per-slot init() can override defaultAngle* with a different starting
    // pose; R-key resets to whichever defaults that slot set.
    float angleX = 40.0f;
    float angleY = 25.0f;
    float angleZ = -30.0f;
    float defaultAngleX = 40.0f;
    float defaultAngleY = 25.0f;
    float defaultAngleZ = -30.0f;

    // Mouse drag state
    bool   isDragging = false;
    double mouseX = 0.0;
    double mouseY = 0.0;

    // Camera placement (orthographic, looking from eye at origin)
    vec3   cameraEye = vec3(25.0f, 25.0f, 25.0f);
    float  orthoSize = 9.5f;     // tighter zoom so the figure fills more of the window
    float  objHeight = 12.0f;

    // ──────────────────────────────────────────────────────────────────────
    // init: load the OBJ file at `path` with `palette` colors, compile the
    // shared impossible-figure shader, upload mesh to GPU.
    //
    // Optional vshader/fshader paths let a slot use its OWN shader pair
    // (e.g. slot 6's iridescent shader) without affecting other slots that
    // share the default impossible-figure shader. Pass nullptr to use the
    // default. Custom shaders are still expected to declare `uModelHeight`,
    // `uLightPos`, etc. — they get the same uniform feed.
    // ──────────────────────────────────────────────────────────────────────
    void init(const char* objPath, const ObjColorPalette& palette,
              const char* vshaderPath = nullptr,
              const char* fshaderPath = nullptr)
    {
        if (!obj_load(objPath, palette, positions, colors))
        {
            fprintf(stderr, "ObjShape::init: failed to load %s\n", objPath);
        }

        const char* vs = vshaderPath ? vshaderPath : "../shaders/core/vshader_impossible.glsl";
        const char* fs = fshaderPath ? fshaderPath : "../shaders/core/fshader_impossible.glsl";
        shaderProgram  = InitShader(vs, fs);

        lightLoc           = glGetUniformLocation(shaderProgram, "uLightPos");
        lightColorLoc      = glGetUniformLocation(shaderProgram, "uLightColor");
        eyeLoc             = glGetUniformLocation(shaderProgram, "uEyePos");
        timeLoc            = glGetUniformLocation(shaderProgram, "uTime");
        heightLoc          = glGetUniformLocation(shaderProgram, "uObjHeight");
        lockGlowLoc        = glGetUniformLocation(shaderProgram, "uLockGlow");
        isBallLoc          = glGetUniformLocation(shaderProgram, "uIsBall");
        // Slot-6-only uniforms — return -1 on the default shader, that's
        // fine; we guard sending them by `loc >= 0`.
        postSolveTimeLoc   = glGetUniformLocation(shaderProgram, "uPostSolveTime");
        breathAmountLoc    = glGetUniformLocation(shaderProgram, "uBreathAmount");
        hueShiftLoc        = glGetUniformLocation(shaderProgram, "uHueShift");
        iridescenceAmtLoc  = glGetUniformLocation(shaderProgram, "uIridescenceAmount");
        skyboxRotationLoc  = glGetUniformLocation(shaderProgram, "uSkyboxRotation");
        modelLoc           = glGetUniformLocation(shaderProgram, "model");
        viewLoc            = glGetUniformLocation(shaderProgram, "view");
        projLoc            = glGetUniformLocation(shaderProgram, "projection");

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &positionBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(vec4),
                     positions.data(), GL_STATIC_DRAW);
        GLuint posLoc = glGetAttribLocation(shaderProgram, "vPosition");
        glEnableVertexAttribArray(posLoc);
        glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

        glGenBuffers(1, &colorBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(vec4),
                     colors.data(), GL_STATIC_DRAW);
        GLuint colLoc = glGetAttribLocation(shaderProgram, "vColor");
        glEnableVertexAttribArray(colLoc);
        glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

        glEnable(GL_DEPTH_TEST);
    }

    // Helper: shortest signed distance from `a` (deg) to the nearest 0 ≡ 360k.
    // Used so wrap-around angles (e.g. 720) lerp the short way to 0.
    static float wrapped_deg_to_zero(float a)
    {
        float r = fmodf(a, 360.0f);
        if (r >  180.0f) r -= 360.0f;
        if (r < -180.0f) r += 360.0f;
        return r;
    }

    // ── Ball path setup (called by per-slot init) ───────────────────────────
    // `waypoints` is a closed loop: segment i connects waypoints[i] to
    // waypoints[(i+1)%N]. For each segment we need an "up" vector — the
    // bar's "top" direction in OBJ space, perpendicular to the bar's axis.
    //
    // `thickness` is the bar's half-cross-section size (used to offset the
    // ball out from the centerline so it visually rests on the surface).
    // `ballRadius` sizes the sphere (OBJ-space units).
    void setBallPath(const std::vector<PathWaypoint>& waypoints,
                     float thickness, float ballRadius)
    {
        ballPath.clear();
        ballPathTotalLength = 0.0f;
        ballThickness   = thickness;
        ballRadiusWorld = ballRadius;
        int n = (int)waypoints.size();
        for (int i = 0; i < n; ++i) {
            const vec3& a = waypoints[i].pos;
            const vec3& b = waypoints[(i + 1) % n].pos;
            const vec3& up = waypoints[i].up;
            vec3 fwd = b - a;
            float len = sqrtf(fwd.x*fwd.x + fwd.y*fwd.y + fwd.z*fwd.z);
            if (len < 1e-6f) continue;
            float ilen = 1.0f / len;
            fwd = vec3(fwd.x*ilen, fwd.y*ilen, fwd.z*ilen);
            // Orthogonalise up against forward (Gram-Schmidt), in case
            // the caller gave a not-quite-perpendicular up.
            float updot = fwd.x*up.x + fwd.y*up.y + fwd.z*up.z;
            vec3 up_ortho(up.x - fwd.x*updot, up.y - fwd.y*updot, up.z - fwd.z*updot);
            float ulen = sqrtf(up_ortho.x*up_ortho.x + up_ortho.y*up_ortho.y + up_ortho.z*up_ortho.z);
            if (ulen > 1e-6f) {
                float iul = 1.0f / ulen;
                up_ortho = vec3(up_ortho.x*iul, up_ortho.y*iul, up_ortho.z*iul);
            }
            vec3 right(fwd.y*up_ortho.z - fwd.z*up_ortho.y,
                       fwd.z*up_ortho.x - fwd.x*up_ortho.z,
                       fwd.x*up_ortho.y - fwd.y*up_ortho.x);
            PathSegment s;
            s.start   = a;
            s.end     = b;
            s.up      = up_ortho;
            s.length  = len;
            s.forward = fwd;
            s.right   = right;
            ballPath.push_back(s);
            ballPathTotalLength += len;
        }

        // One-time sphere mesh generation (lazy: only when a slot opts in to
        // having a ball). Reuses the figure's shader + the new uIsBall mode.
        if (ballVao == 0) generateBallSphere();
    }

    // Generate a sphere mesh, upload to a dedicated VAO. Bright yellow vertex
    // colors so uIsBall mode in the shader can emit (color * 2.0).
    void generateBallSphere()
    {
        const int STACKS = 12, SLICES = 14;
        std::vector<vec4> positions, colors;
        for (int i = 0; i < STACKS; ++i) {
            float phi1 = (float)M_PI * (float)i       / (float)STACKS;
            float phi2 = (float)M_PI * (float)(i + 1) / (float)STACKS;
            for (int j = 0; j < SLICES; ++j) {
                float t1 = 2.0f * (float)M_PI * (float)j       / (float)SLICES;
                float t2 = 2.0f * (float)M_PI * (float)(j + 1) / (float)SLICES;
                vec4 v[4] = {
                    vec4(sinf(phi1)*cosf(t1), cosf(phi1), sinf(phi1)*sinf(t1), 1),
                    vec4(sinf(phi2)*cosf(t1), cosf(phi2), sinf(phi2)*sinf(t1), 1),
                    vec4(sinf(phi2)*cosf(t2), cosf(phi2), sinf(phi2)*sinf(t2), 1),
                    vec4(sinf(phi1)*cosf(t2), cosf(phi1), sinf(phi1)*sinf(t2), 1),
                };
                positions.push_back(v[0]); positions.push_back(v[1]); positions.push_back(v[2]);
                positions.push_back(v[0]); positions.push_back(v[2]); positions.push_back(v[3]);
            }
        }
        ballVertexCount = (int)positions.size();
        colors.assign(ballVertexCount, vec4(1.0f, 0.85f, 0.2f, 1.0f));   // golden yellow

        glGenVertexArrays(1, &ballVao);
        glBindVertexArray(ballVao);
        glGenBuffers(1, &ballPosBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, ballPosBuffer);
        glBufferData(GL_ARRAY_BUFFER, positions.size()*sizeof(vec4), positions.data(), GL_STATIC_DRAW);
        GLuint posLoc = glGetAttribLocation(shaderProgram, "vPosition");
        glEnableVertexAttribArray(posLoc);
        glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
        glGenBuffers(1, &ballColorBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, ballColorBuffer);
        glBufferData(GL_ARRAY_BUFFER, colors.size()*sizeof(vec4), colors.data(), GL_STATIC_DRAW);
        GLuint colLoc = glGetAttribLocation(shaderProgram, "vColor");
        glEnableVertexAttribArray(colLoc);
        glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
        glBindVertexArray(0);
    }

    // Convert (ball_s, ball_u) → world OBJ-space position. Uses arc-length
    // parameterisation along ballPath (closed loop), and the per-segment
    // (up, right) frame for cross-section offset.
    vec3 ballPositionFromSU() const
    {
        if (ballPath.empty()) return vec3(0,0,0);
        // Find which segment ball_s lands on.
        float s_arc = ball_s * ballPathTotalLength;
        const PathSegment* seg = &ballPath[0];
        float prev = 0.0f;
        for (size_t i = 0; i < ballPath.size(); ++i) {
            if (s_arc < prev + ballPath[i].length) { seg = &ballPath[i]; break; }
            prev += ballPath[i].length;
            seg = &ballPath[i];
        }
        float seg_t = (s_arc - prev) / seg->length;
        if (seg_t < 0) seg_t = 0; if (seg_t > 1) seg_t = 1;

        // Centerline point along this segment.
        vec3 center(
            seg->start.x + seg_t * (seg->end.x - seg->start.x),
            seg->start.y + seg_t * (seg->end.y - seg->start.y),
            seg->start.z + seg_t * (seg->end.z - seg->start.z));

        // Cross-section perimeter direction from ball_u (same diamond as slot 1).
        //   0 → inner (-right),   1 → top (+up),   2 → outer (+right),   3 → bot (-up)
        int face = (int)floorf(ball_u);
        face = ((face % 4) + 4) % 4;
        float ft = ball_u - floorf(ball_u);
        vec3 ay_d, by_d, n_d;  // a, b in y-z perimeter, outward normal (all in local frame)
        const float h = ballThickness;
        const float k = 0.70710678f;
        // ( y_local, z_local ) where y → right, z → up.
        float ay, az, by, bz, ny, nz;
        switch (face) {
            case 0: ay=-h; az=0;  by=0;  bz=+h; ny=-k; nz=+k; break;   // in → top
            case 1: ay=0;  az=+h; by=+h; bz=0;  ny=+k; nz=+k; break;   // top → out
            case 2: ay=+h; az=0;  by=0;  bz=-h; ny=+k; nz=-k; break;   // out → bot
            default:ay=0;  az=-h; by=-h; bz=0;  ny=-k; nz=-k; break;   // bot → in
        }
        float y_perim = ay + ft * (by - ay);
        float z_perim = az + ft * (bz - az);
        float r = ballRadiusWorld;
        float y_local = y_perim + r * ny;
        float z_local = z_perim + r * nz;

        // Combine: world = center + y_local*right + z_local*up
        return vec3(
            center.x + y_local * seg->right.x + z_local * seg->up.x,
            center.y + y_local * seg->right.y + z_local * seg->up.y,
            center.z + y_local * seg->right.z + z_local * seg->up.z);
    }

    // Update ball_s, ball_u from held-WASD state (called once per frame from display).
    void updateBallFromInput(float delta_time)
    {
        if (solvePhase != SolvePhase::Locked) {
            // While unsolved or animating: ball idle at start of path.
            ball_s = 0.0f;
            ball_u = 1.5f;   // top-outer-ish default
            return;
        }
        float s_input = 0.0f, u_input = 0.0f;
        if (key_w_held) s_input += 1.0f;
        if (key_s_held) s_input -= 1.0f;
        if (key_d_held) u_input += 1.0f;
        if (key_a_held) u_input -= 1.0f;

        if (s_input != 0.0f || u_input != 0.0f) {
            ball_s += s_input * BALL_S_SPEED * delta_time;
            ball_u += u_input * BALL_U_SPEED * delta_time;
        } else {
            double now = glfwGetTime();
            if (now - last_wasd_input_time > (double)IDLE_DRIFT_THRESHOLD_SEC)
                ball_s += BALL_S_SPEED * delta_time;
        }
        ball_s -= floorf(ball_s);
        ball_u -= 4.0f * floorf(ball_u * 0.25f);
    }

    // ── Quaternion helpers (for ROTATING slerp) ──────────────────────────
    // q = (x, y, z, w). Identity = (0, 0, 0, 1). Convention: x,y,z = sin(θ/2)*axis,
    // w = cos(θ/2). Mat is column-major (d[i] = column i).
    static void quatFromMat(const mat4& M, float& qx, float& qy, float& qz, float& qw)
    {
        float m00 = M.d[0].x, m01 = M.d[1].x, m02 = M.d[2].x;
        float m10 = M.d[0].y, m11 = M.d[1].y, m12 = M.d[2].y;
        float m20 = M.d[0].z, m21 = M.d[1].z, m22 = M.d[2].z;
        float trace = m00 + m11 + m22;
        if (trace > 0.0f) {
            float s = 0.5f / sqrtf(trace + 1.0f);
            qw = 0.25f / s;
            qx = (m21 - m12) * s;
            qy = (m02 - m20) * s;
            qz = (m10 - m01) * s;
        } else if (m00 > m11 && m00 > m22) {
            float s = 2.0f * sqrtf(1.0f + m00 - m11 - m22);
            qw = (m21 - m12) / s;
            qx = 0.25f * s;
            qy = (m01 + m10) / s;
            qz = (m02 + m20) / s;
        } else if (m11 > m22) {
            float s = 2.0f * sqrtf(1.0f + m11 - m00 - m22);
            qw = (m02 - m20) / s;
            qx = (m01 + m10) / s;
            qy = 0.25f * s;
            qz = (m12 + m21) / s;
        } else {
            float s = 2.0f * sqrtf(1.0f + m22 - m00 - m11);
            qw = (m10 - m01) / s;
            qx = (m02 + m20) / s;
            qy = (m12 + m21) / s;
            qz = 0.25f * s;
        }
    }

    static mat4 matFromQuat(float qx, float qy, float qz, float qw)
    {
        float xx = qx*qx, yy = qy*qy, zz = qz*qz;
        float xy = qx*qy, xz = qx*qz, yz = qy*qz;
        float wx = qw*qx, wy = qw*qy, wz = qw*qz;
        mat4 m;
        m.d[0] = vec4(1 - 2*(yy + zz),  2*(xy + wz),     2*(xz - wy),     0.0f);
        m.d[1] = vec4(2*(xy - wz),      1 - 2*(xx + zz), 2*(yz + wx),     0.0f);
        m.d[2] = vec4(2*(xz + wy),      2*(yz - wx),     1 - 2*(xx + yy), 0.0f);
        m.d[3] = vec4(0.0f,             0.0f,            0.0f,            1.0f);
        return m;
    }

    // SLERP between (q1) and (q2) at parameter t ∈ [0,1]. Picks the SHORT
    // arc (negates q2 if dot < 0) — important for the back-side case where
    // raw lerp would take the long way around the quaternion sphere.
    static void quatSlerp(
        float q1x, float q1y, float q1z, float q1w,
        float q2x, float q2y, float q2z, float q2w,
        float t,
        float& outX, float& outY, float& outZ, float& outW)
    {
        float dot = q1x*q2x + q1y*q2y + q1z*q2z + q1w*q2w;
        if (dot < 0.0f) {
            dot = -dot;
            q2x = -q2x; q2y = -q2y; q2z = -q2z; q2w = -q2w;
        }
        if (dot > 0.9995f) {
            // Quaternions very close — just lerp + normalise (slerp goes
            // numerically unstable here, no visible difference).
            outX = q1x + t * (q2x - q1x);
            outY = q1y + t * (q2y - q1y);
            outZ = q1z + t * (q2z - q1z);
            outW = q1w + t * (q2w - q1w);
            float n = sqrtf(outX*outX + outY*outY + outZ*outZ + outW*outW);
            outX /= n; outY /= n; outZ /= n; outW /= n;
            return;
        }
        float theta_0     = acosf(dot);
        float theta       = theta_0 * t;
        float sin_theta   = sinf(theta);
        float sin_theta_0 = sinf(theta_0);
        float s0 = cosf(theta) - dot * sin_theta / sin_theta_0;
        float s1 = sin_theta / sin_theta_0;
        outX = s0 * q1x + s1 * q2x;
        outY = s0 * q1y + s1 * q2y;
        outZ = s0 * q1z + s1 * q2z;
        outW = s0 * q1w + s1 * q2w;
    }

    // Rodrigues' rotation formula: rotation by `angleRad` around unit `axis`.
    // Column-major (matches the project's mat4 convention — d[i] is column i).
    static mat4 axisAngleRotationRad(vec3 axis, float angleRad)
    {
        float c = cosf(angleRad);
        float s = sinf(angleRad);
        float t = 1.0f - c;
        float x = axis.x, y = axis.y, z = axis.z;
        mat4 m;
        m.d[0] = vec4(t*x*x + c,    t*x*y + s*z,  t*x*z - s*y,  0.0f);  // col 0
        m.d[1] = vec4(t*x*y - s*z,  t*y*y + c,    t*y*z + s*x,  0.0f);  // col 1
        m.d[2] = vec4(t*x*z + s*y,  t*y*z - s*x,  t*z*z + c,    0.0f);  // col 2
        m.d[3] = vec4(0.0f,         0.0f,         0.0f,         1.0f);  // col 3
        return m;
    }

    // Extract the SIGNED iso-axis rotation angle of the current Euler pose.
    // We pick a probe vector w perpendicular to the iso axis, apply R, project
    // the result onto the perpendicular plane, and read off the signed angle
    // between w and the projected R·w. For poses that aren't perfectly iso-
    // axis-preserving the projection silently drops the off-axis component —
    // which is exactly what we want (animate the iso component, ignore the
    // small misalignment that the FOUND tolerance let in).
    float extractIsoAxisAngleRad() const
    {
        mat4 R = RotateY(angleY) * RotateX(angleX) * RotateZ(angleZ);
        const float k  = 1.0f / 1.7320508f;            // 1/√3 — iso axis component
        const float kw = 1.0f / 1.4142136f;            // 1/√2 — probe vector
        vec3 u(k, k, k);
        vec3 w(kw, -kw, 0.0f);                          // u · w = 0
        vec4 rw4 = R * vec4(w.x, w.y, w.z, 0.0f);
        vec3 rw(rw4.x, rw4.y, rw4.z);
        // Project Rw onto plane perpendicular to u.
        float duw = rw.x*u.x + rw.y*u.y + rw.z*u.z;
        vec3 rwp(rw.x - u.x*duw, rw.y - u.y*duw, rw.z - u.z*duw);
        float len = sqrtf(rwp.x*rwp.x + rwp.y*rwp.y + rwp.z*rwp.z);
        if (len < 1e-6f) return 0.0f;
        rwp.x /= len; rwp.y /= len; rwp.z /= len;
        // Signed angle: atan2(sin, cos) with sign from cross(w, rwp) · u.
        float cosA = w.x*rwp.x + w.y*rwp.y + w.z*rwp.z;
        vec3  cr(w.y*rwp.z - w.z*rwp.y,
                 w.z*rwp.x - w.x*rwp.z,
                 w.x*rwp.y - w.y*rwp.x);
        float sinA = cr.x*u.x + cr.y*u.y + cr.z*u.z;
        return atan2f(sinA, cosA);
    }

    // Helper: angle (deg) between the rotated iso axis and the original iso
    // axis. When this is near zero, the rotation preserves the iso axis →
    // the bisect cuts project to lines and the illusion is visible. Treats
    // the axis as a line (180° flip is also fine — cuts are 2-sided).
    float isoAxisMisalignmentDeg() const
    {
        mat4 R = RotateY(angleY) * RotateX(angleX) * RotateZ(angleZ);
        const float k = 1.0f / 1.7320508f;   // 1/√3
        vec4 v(k, k, k, 0.0f);
        vec4 rv = R * v;
        float d = fabsf(rv.x * k + rv.y * k + rv.z * k);  // |dot| with original
        if (d > 1.0f)  d = 1.0f;
        if (d < 0.0f)  d = 0.0f;
        return acosf(d) * 180.0f / (float)M_PI;
    }

    // Enter the FOUND phase — hold at the user's pose with a soft glow so
    // they get the "yes, you found it" confirmation before the figure moves.
    // Auto-called when the iso axis aligns. No-op if already in any phase.
    void enterFound()
    {
        if (solvePhase != SolvePhase::None) return;
        solvePhase     = SolvePhase::Found;
        phaseStartTime = glfwGetTime();
    }

    // Start the ROTATING phase — slow slerp from user's pose to a target
    // pose on the SAME iso-axis family, so cuts never re-separate.
    //
    //   R·iso ≈ +iso → user in F+ → target = identity      (S pose)
    //   R·iso ≈ -iso → user in F- → target = R_B (180°     (B pose)
    //                  about (1,-1,0)/√2, also iso-aligned)
    //
    // Both source and target sit on the SAME great circle on the quaternion
    // 3-sphere, so slerp stays within that circle and the cuts project to
    // lines throughout the animation.
    void enterRotating()
    {
        mat4 R = RotateY(angleY) * RotateX(angleX) * RotateZ(angleZ);
        quatFromMat(R, rotateStartQ_x, rotateStartQ_y, rotateStartQ_z, rotateStartQ_w);
        rotateCurQ_x = rotateStartQ_x;
        rotateCurQ_y = rotateStartQ_y;
        rotateCurQ_z = rotateStartQ_z;
        rotateCurQ_w = rotateStartQ_w;

        // Detect F+ vs F- by checking R·iso direction.
        const float k = 1.0f / 1.7320508f;   // 1/√3
        vec4 rv = R * vec4(k, k, k, 0.0f);
        float dot_iso = rv.x * k + rv.y * k + rv.z * k;

        if (dot_iso >= 0.0f) {
            // F+ → snap to S (identity).
            rotateTargetIsB = false;
            rotateTargetQ_x = 0.0f; rotateTargetQ_y = 0.0f;
            rotateTargetQ_z = 0.0f; rotateTargetQ_w = 1.0f;
        } else {
            // F- → snap to B (180° rotation about (1,-1,0)/√2).
            //   q = (sin(90°)·axis, cos(90°)) = (axis, 0)
            //     = (1/√2, -1/√2, 0, 0)
            rotateTargetIsB = true;
            const float kw = 1.0f / 1.4142136f;   // 1/√2
            rotateTargetQ_x =  kw; rotateTargetQ_y = -kw;
            rotateTargetQ_z = 0.0f; rotateTargetQ_w = 0.0f;
        }

        // Wrap-normalise the Euler angles (cosmetic — overwritten at end
        // of ROTATING anyway).
        animStartX = wrapped_deg_to_zero(angleX);
        animStartY = wrapped_deg_to_zero(angleY);
        animStartZ = wrapped_deg_to_zero(angleZ);
        solvePhase     = SolvePhase::Rotating;
        phaseStartTime = glfwGetTime();
    }

    // Bail out of whatever solve phase is running and let the user free-drag
    // again. Called by R key, and from the display update when the player
    // drags off-alignment during FOUND or out past UNLOCK_TOL while locked.
    void cancelLock()
    {
        solvePhase = SolvePhase::None;
    }

    // View + projection getters so slot 6's skybox can use the SAME camera
    // setup as the figure (otherwise the rotating sky and the reflection
    // sampled by the chrome would drift apart).
    mat4 getViewMatrix() const
    {
        vec3 eye(cameraEye);
        return LookAt(eye, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    }
    mat4 getProjectionMatrix() const
    {
        float aspect = (screen_w > 0 && screen_h > 0)
                           ? (float)screen_w / (float)screen_h
                           : 1.0f;
        return Ortho(-orthoSize * aspect, orthoSize * aspect,
                     -orthoSize, orthoSize, 0.1f, 200.0f);
    }

    // ──────────────────────────────────────────────────────────────────────
    // display: render the shape with current rotation against an ortho cam.
    // ──────────────────────────────────────────────────────────────────────
    void display()
    {
        glUseProgram(shaderProgram);
        glBindVertexArray(vao);

        double now = glfwGetTime();

        // ── Three-phase solve animation update ───────────────────────────
        float lockScale = 1.0f;   // model-space scale bump (Locked only)
        float lockGlow  = 0.0f;   // additive brightness (used in every phase)

        switch (solvePhase)
        {
            case SolvePhase::None:
            {
                // Watch for the player aligning the cuts. Cuts are perp to
                // iso axis (1,1,1)/√3, so any rotation preserving that axis
                // → cuts project to lines → illusion visible.
                if (isoAxisMisalignmentDeg() < AUTO_SNAP_ISO_TOL_DEG)
                    enterFound();
                break;
            }

            case SolvePhase::Found:
            {
                // Hold the figure at the user's current pose, build a soft
                // glow that rises and fades. Lets the player SEE the moment
                // they found it. If they drag away, cancel before moving.
                float t = (float)((now - phaseStartTime) / FOUND_DURATION);
                if (isDragging && isoAxisMisalignmentDeg() > UNLOCK_ISO_TOL_DEG) {
                    solvePhase = SolvePhase::None;
                    break;
                }
                // Half-sine: 0 → 1 → 0 over the phase window
                lockGlow = 0.32f * sinf((float)M_PI * fminf(t, 1.0f));
                if (t >= 1.0f) enterRotating();
                break;
            }

            case SolvePhase::Rotating:
            {
                // Slow slerp from rotateStartQ to rotateTargetQ — both on
                // the SAME iso-axis great circle, so the path stays cut-
                // aligned the whole way.
                float t = (float)((now - phaseStartTime) / ROTATE_DURATION);
                if (t >= 1.0f) {
                    t = 1.0f;
                    // Land exactly on the target.
                    rotateCurQ_x = rotateTargetQ_x; rotateCurQ_y = rotateTargetQ_y;
                    rotateCurQ_z = rotateTargetQ_z; rotateCurQ_w = rotateTargetQ_w;
                    // Sync Euler so post-LOCK drags resume cleanly from the
                    // landed pose. For S (target = identity) → 0/0/0. For B
                    // (target = 180° about (1,-1,0)/√2) → equivalent YXZ
                    // Euler angles in degrees, derived below.
                    if (rotateTargetIsB) {
                        // R_B = [[0, -1, 0], [-1, 0, 0], [0, 0, -1]]
                        // YXZ extraction:  β = -asin(d[2].y) = -asin(0) = 0
                        //                  α = atan2(d[0].y, d[1].y) = atan2(-1, 0) = -π/2 (= -90°)
                        //                  γ = atan2(d[2].x, d[2].z) = atan2(0, -1) = π (= 180°)
                        angleX = 0.0f;
                        angleY = 180.0f;     // γ → angleY in our convention
                        angleZ = -90.0f;     // α → angleZ
                    } else {
                        angleX = 0.0f; angleY = 0.0f; angleZ = 0.0f;
                    }
                    solvePhase     = SolvePhase::Locked;
                    phaseStartTime = now;
                } else {
                    // Smootherstep (Perlin's quintic).
                    float et = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
                    quatSlerp(rotateStartQ_x, rotateStartQ_y, rotateStartQ_z, rotateStartQ_w,
                              rotateTargetQ_x, rotateTargetQ_y, rotateTargetQ_z, rotateTargetQ_w,
                              et,
                              rotateCurQ_x, rotateCurQ_y, rotateCurQ_z, rotateCurQ_w);
                }
                lockGlow = 0.18f;
                break;
            }

            case SolvePhase::Locked:
            {
                // Pin to the LANDED solved pose (S or B) so the bump plays
                // from that exact pose, free of cursor drift. The end of
                // ROTATING already sync'd angle X/Y/Z to the right values.
                if (rotateTargetIsB) {
                    angleX = 0.0f; angleY = 180.0f; angleZ = -90.0f;
                } else {
                    angleX = 0.0f; angleY = 0.0f; angleZ = 0.0f;
                }
                if (isDragging && isoAxisMisalignmentDeg() > UNLOCK_ISO_TOL_DEG) {
                    solvePhase = SolvePhase::None;
                    break;
                }
                float lt = (float)((now - phaseStartTime) / LOCK_PULSE_DURATION);
                if (lt < 1.0f) {
                    float bump = sinf((float)M_PI * lt) * (1.0f - lt);
                    lockScale = 1.0f + 0.08f * bump;
                    lockGlow  = 0.40f * bump;
                }
                // After the bump decays, lockScale = 1, lockGlow = 0 — figure
                // sits at the solved pose looking normal until the player
                // drags it away.
                break;
            }
        }

        vec3 eye(cameraEye);
        vec3 at(0.0f, 0.0f, 0.0f);
        vec3 up(0.0f, 1.0f, 0.0f);
        mat4 view = LookAt(eye, at, up);

        float aspect = (screen_w > 0 && screen_h > 0)
                           ? (float)screen_w / (float)screen_h
                           : 1.0f;
        mat4 projection = Ortho(-orthoSize * aspect, orthoSize * aspect,
                                -orthoSize, orthoSize, 0.1f, 200.0f);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view.d[0].x);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection.d[0].x);

        // During ROTATING, build the rotation from the slerped quaternion
        // (shortest-arc 3D rotation from the user's pose to identity).
        // Every other phase uses the normal Euler stack — the player's drag
        // input feeds straight into angleX/Y/Z.
        mat4 rotPart;
        if (solvePhase == SolvePhase::Rotating) {
            rotPart = matFromQuat(rotateCurQ_x, rotateCurQ_y, rotateCurQ_z, rotateCurQ_w);
        } else {
            rotPart = RotateY(angleY) * RotateX(angleX) * RotateZ(angleZ);
        }
        mat4 model = Scale(lockScale, lockScale, lockScale) * rotPart;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model.d[0].x);

        vec3 lightPos = useCustomLight
                            ? customLightPos
                            : vec3(eye.x * 0.8f, eye.y * 1.6f, eye.z * 1.2f);
        glUniform3fv(lightLoc, 1, &lightPos.x);
        // Per-slot light tint (Tier 2). Always fed (white by default) so the
        // uniform is never left at 0 → black. Guarded for shaders lacking it.
        if (lightColorLoc >= 0) glUniform3fv(lightColorLoc, 1, &customLightColor.x);
        glUniform3fv(eyeLoc, 1, &eye.x);
        glUniform1f(timeLoc, (float)now);
        glUniform1f(heightLoc, objHeight);
        if (lockGlowLoc >= 0) glUniform1f(lockGlowLoc, lockGlow);
        if (isBallLoc   >= 0) glUniform1i(isBallLoc,   0);   // figure mode

        // ── Post-solve animation uniforms (only iridescent shader uses) ──
        // Track seconds since LOCKED began. Other phases reset it to 0 so the
        // shimmer/breathing only plays while actually solved.
        if (solvePhase == SolvePhase::Locked) {
            postSolveTime = (float)(now - phaseStartTime);
        } else {
            postSolveTime = 0.0f;
        }
        // Shimmer has THREE sources, max-blended:
        //   1. BASELINE   — always on, so the chrome never looks dead.
        //   2. PROXIMITY  — ramps in as the player rotates closer to the
        //      magic iso-axis alignment. Live "getting warmer" feedback —
        //      makes the unsolved puzzle phase visually interesting too.
        //   3. POST-SOLVE — boost from baseline → 1.0 over RAMP_DURATION
        //      once locked, so solving still feels like a payoff.
        constexpr float RAMP_DURATION       = 1.5f;
        constexpr float BASELINE            = 0.35f;
        constexpr float PROXIMITY_PEAK      = 0.85f;   // how strong proximity gets
        constexpr float PROXIMITY_FULL_DEG  = 8.0f;    // ≤ 8° misalign → max boost
        constexpr float PROXIMITY_NONE_DEG  = 40.0f;   // ≥ 40° → no boost

        // (1) Baseline.
        float postRamp = BASELINE;

        // (2) Proximity — only meaningful when we're not yet in a solve
        // phase (otherwise post-solve dominates and proximity is noise).
        if (solvePhase == SolvePhase::None || solvePhase == SolvePhase::Found) {
            float mis = isoAxisMisalignmentDeg();
            float t = (PROXIMITY_NONE_DEG - mis)
                    / (PROXIMITY_NONE_DEG - PROXIMITY_FULL_DEG);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            // Smoothstep for a satisfying ramp curve.
            float et = t * t * (3.0f - 2.0f * t);
            float proxAmount = BASELINE + (PROXIMITY_PEAK - BASELINE) * et;
            if (proxAmount > postRamp) postRamp = proxAmount;
        }

        // (3) Post-solve.
        if (solvePhase == SolvePhase::Locked) {
            float ramp = fminf(postSolveTime / RAMP_DURATION, 1.0f);
            float solvedAmount = BASELINE + (1.0f - BASELINE) * ramp;
            if (solvedAmount > postRamp) postRamp = solvedAmount;
        }
        // Hue drift — also runs continuously off a free-running clock so
        // the unsolved shimmer cycles too. (Was tied to postSolveTime which
        // is 0 when not locked → no hue change → static rainbow.)
        float freeTime = (float)now;
        float hueShift = fmodf(freeTime * 0.08f, 1.0f);

        if (postSolveTimeLoc  >= 0) glUniform1f(postSolveTimeLoc,  postSolveTime);
        if (breathAmountLoc   >= 0) glUniform1f(breathAmountLoc,   postRamp);
        if (hueShiftLoc       >= 0) glUniform1f(hueShiftLoc,       hueShift);
        if (iridescenceAmtLoc >= 0) glUniform1f(iridescenceAmtLoc, postRamp);
        if (skyboxRotationLoc >= 0) glUniform1f(skyboxRotationLoc, externalSkyboxRotation);

        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)positions.size());

        // ── BALL DRAW (only when LOCKED and a path has been set) ───────────
        // We compute delta_time on the fly from glfwGetTime. This gives the
        // player WASD-driven motion + idle drift once a path is registered.
        // Depth test is disabled for the WHOLE ball draw so it always
        // floats on top of the figure regardless of cuts / corners.
        if (solvePhase == SolvePhase::Locked && !ballPath.empty() && ballVao != 0)
        {
            static double last_t = 0.0;
            double tnow = glfwGetTime();
            float dt = (last_t > 0.0) ? (float)(tnow - last_t) : 0.0f;
            last_t = tnow;
            if (dt > 0.1f) dt = 0.1f;   // clamp giant pauses

            updateBallFromInput(dt);
            vec3 ball_pos_obj = ballPositionFromSU();

            // Apply the same model rotation+scale so the ball follows the
            // figure (it's solved → rotation is identity-or-B, but the
            // pulse scale may make it bigger this frame).
            mat4 ballScale = Scale(ballRadiusWorld, ballRadiusWorld, ballRadiusWorld);
            mat4 ballTranslate = Translate(ball_pos_obj.x, ball_pos_obj.y, ball_pos_obj.z);
            mat4 ballModel = model * ballTranslate * ballScale;

            glBindVertexArray(ballVao);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &ballModel.d[0].x);
            if (isBallLoc >= 0) glUniform1i(isBallLoc, 1);

            glDisable(GL_DEPTH_TEST);
            glDrawArrays(GL_TRIANGLES, 0, ballVertexCount);
            glEnable(GL_DEPTH_TEST);

            if (isBallLoc >= 0) glUniform1i(isBallLoc, 0);
            glBindVertexArray(vao);   // restore figure VAO for any later draws
        }

        glFinish();
    }

    // ──────────────────────────────────────────────────────────────────────
    // Mouse / keyboard handlers
    // ──────────────────────────────────────────────────────────────────────
    void mouseButton(GLFWwindow* window, int button, int action)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            isDragging = (action == GLFW_PRESS);
            if (isDragging) glfwGetCursorPos(window, &mouseX, &mouseY);
        }
    }

    void cursorPos(double x, double y)
    {
        if (!isDragging) return;
        angleY += (float)(x - mouseX) * 0.4f;
        angleX += (float)(y - mouseY) * 0.4f;
        mouseX = x;
        mouseY = y;
    }

    void scroll(double yoffset)
    {
        angleZ += (float)yoffset * 2.0f;
    }

    void key(GLFWwindow* win, int keyCode, int action)
    {
        // ── WASD: PRESS / RELEASE → drive the ball when locked ─────────────
        // We track held-flags here on PRESS/RELEASE; the per-frame update in
        // display() converts the flags into ball_s / ball_u motion. The
        // motion only takes effect while solvePhase == Locked.
        bool wasd_key = (keyCode == GLFW_KEY_W || keyCode == GLFW_KEY_A ||
                         keyCode == GLFW_KEY_S || keyCode == GLFW_KEY_D);
        if (wasd_key) {
            if (action == GLFW_PRESS || action == GLFW_REPEAT) {
                last_wasd_input_time = glfwGetTime();
                if (keyCode == GLFW_KEY_W) key_w_held = true;
                if (keyCode == GLFW_KEY_A) key_a_held = true;
                if (keyCode == GLFW_KEY_S) key_s_held = true;
                if (keyCode == GLFW_KEY_D) key_d_held = true;
            } else if (action == GLFW_RELEASE) {
                if (keyCode == GLFW_KEY_W) key_w_held = false;
                if (keyCode == GLFW_KEY_A) key_a_held = false;
                if (keyCode == GLFW_KEY_S) key_s_held = false;
                if (keyCode == GLFW_KEY_D) key_d_held = false;
            }
            // When locked, WASD is ONLY for the ball — swallow so S doesn't
            // also re-trigger the solve animation, etc. When unlocked, only
            // S has a legacy meaning; just fall through to handle it below.
            if (solvePhase == SolvePhase::Locked) return;
        }

        if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
        if (keyCode == GLFW_KEY_LEFT)  angleY -= 3.0f;
        if (keyCode == GLFW_KEY_RIGHT) angleY += 3.0f;
        if (keyCode == GLFW_KEY_UP)    angleX -= 3.0f;
        if (keyCode == GLFW_KEY_DOWN)  angleX += 3.0f;
        if (keyCode == GLFW_KEY_R)
        {
            // R = reset to this slot's defaults (may include per-slot tweaks
            // like the Y-rotated Penrose Stair or Z-rotated Impossible Arch).
            cancelLock();
            angleX = defaultAngleX;
            angleY = defaultAngleY;
            angleZ = defaultAngleZ;
        }
        if (keyCode == GLFW_KEY_S)
        {
            // S = "solved" pose. Skip the FOUND phase (the player asked
            // for it directly) and go straight to ROTATING — slow ease
            // from current angles to identity, then the lock pulse.
            //
            // Why identity is solved: OBJ files come out of the Paradox
            // Toolkit Blender addon laid out in iso-aligned coords (Blender's
            // axonometric camera maps through the Wavefront axis remap to
            // OBJ direction (1,1,1)/√3). Our OpenGL camera at (25,25,25)
            // looks along the same line, so the bisect cuts project to
            // lines at identity rotation. No additional rotation needed.
            if (solvePhase == SolvePhase::None || solvePhase == SolvePhase::Found)
                enterRotating();
        }
        if (keyCode == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, GL_TRUE);
    }
};

// Default palette that gives axis-aligned faces distinct shades.
inline ObjColorPalette default_palette()
{
    ObjColorPalette p;
    p.xp = vec4(0.45f, 0.48f, 0.62f, 1.0f);
    p.xn = vec4(0.38f, 0.40f, 0.55f, 1.0f);
    p.yp = vec4(0.76f, 0.60f, 0.64f, 1.0f);
    p.yn = vec4(0.35f, 0.37f, 0.52f, 1.0f);
    p.zp = vec4(0.73f, 0.58f, 0.62f, 1.0f);
    p.zn = vec4(0.42f, 0.45f, 0.60f, 1.0f);
    p.generic = vec4(0.55f, 0.55f, 0.60f, 1.0f);
    return p;
}
