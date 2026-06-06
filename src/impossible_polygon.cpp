#include "includes.h"
#include "polygon_bg.h"
#include <cstdlib>   // rand, srand
#include <ctime>     // time

int num_segments = 3;

// ─────────────────────────────────────────────────────────────────────────────
// IMPOSSIBLE POLYGON — depth trick
//
// Every bar steps DOWN in z by `zStep` as bar_index increases, so the loop
// spirals from top (bar 0) to bottom (bar N-1).
//
// The closing bar (bar 0) is SPLIT INTO TWO HALVES:
//   - left  half: sits at the TOP    z  (z = N*zStep), joins bar 1
//   - right half: jumps to the BOTTOM z (z = 0),       joins bar N-1
//
// Each half is a normal bar piece mitred on one end (where it joins its
// neighbour) and FLAT-CUT on the other end (where they meet at the polygon
// centre). The two flat cuts sit on the same vertical line in 2D screen-space
// when the camera is on the (0, ±1, 0) up-axis at theta=phi=π/2 — i.e. our
// default ortho view — so the z difference between the two halves projects
// to nothing, and the polygon reads as a CLOSED loop. That's the illusion.
//
// (This is the same construction we had in the very first version of this
// file. zStep was briefly set to 0 to make the figure "continuous", which
// killed the illusion — restored here.)
// ─────────────────────────────────────────────────────────────────────────────

float scale_factor = num_segments / 3.0f;
float radius       = 0.70f * scale_factor;   // bumped (was 0.45) → longer edges, less chunky look
float zStep        = 0.55f * scale_factor;   // depth-trick z-step per bar

// Bar thickness — slimmer to match the proportions of the Penrose Triangle /
// Blocked Penrose figures (slots 2 & 3). Was 0.32 (chunky), dropped to 0.20.
static constexpr float POLYGON_BAR_THICKNESS = 0.20f;

const int num_vertices = 36;
vec3 segment_vertices[num_vertices];
vec3 segment_normals[num_vertices];

// Bar 0 is drawn as two of these instead of one full segment. The half has the
// same hex cross-section as the full bar, but its right tip is FLAT (a vertical
// bisect cut) instead of mitered, so the two halves butt cleanly at x=0.
vec3 half_segment_vertices[num_vertices];
vec3 half_segment_normals[num_vertices];

GLuint segment_vao = 0, segment_vbo = 0, segment_nbo = 0;
GLuint half_segment_vao = 0, half_segment_vbo = 0, half_segment_nbo = 0;

GLuint program;
GLint  mvp_loc;
GLint  color_loc;
GLint  light_pos_loc, eye_pos_loc, model_loc, bar_t_loc, base_color_loc;
GLint  num_segments_loc;
static GLint lock_glow_loc = -1;  // uLockGlow — HDR boost when solved

// ── Ball state ────────────────────────────────────────────────────────────
// Two-coordinate surface parameter:
//   ball_s ∈ [0, 1)  position along the impossible loop (cut → CCW → cut)
//   ball_u ∈ [0, 4)  position around the bar's diamond cross-section:
//                      0 = inner   corner (y = -h_t, z = 0)
//                      1 = top     corner (y = 0,    z = +h_t)
//                      2 = outer   corner (y = +h_t, z = 0)
//                      3 = bottom  corner (y = 0,    z = -h_t)
//                    fractional values interpolate along the diamond face
//                    between two corners — the ball appears to roll on a real
//                    surface (not the ridge LINE).
//
// While LOCKED, WASD controls these:
//   W / S → ball_s +/-
//   D / A → ball_u +/-
//
// If no WASD input for IDLE_DRIFT_THRESHOLD_SEC seconds, ball_s drifts on its
// own at the old auto-orbit speed so the polygon still has the "alive" feel
// when the player isn't actively driving.
float ball_s = 0.0f;
float ball_u = 1.5f;   // start at top-outer face midpoint (matches old default)
const int   SPHERE_STACKS = 10;
const int   SPHERE_SLICES = 10;
const float BALL_RADIUS   = 0.04f;
std::vector<vec3> sphere_vertices;
std::vector<vec3> sphere_normals;
GLuint sphere_vao = 0, sphere_vbo = 0, sphere_nbo_id = 0;
GLuint ball_pos_loc = 0, is_ball_loc = 0, ball_color_loc = 0;

// WASD held-flags (event-driven press/release → polled every frame).
bool   key_w_held = false;
bool   key_a_held = false;
bool   key_s_held = false;
bool   key_d_held = false;
double last_wasd_input_time = 0.0;
static constexpr float BALL_S_SPEED            = 0.25f;   // loops per second
static constexpr float BALL_U_SPEED            = 1.30f;   // perimeter units per second
static constexpr float IDLE_DRIFT_THRESHOLD_SEC = 1.5f;   // idle → resume auto-orbit

float camera_radius = 0.5f;
float camera_theta  = M_PI / 2.0f;
float camera_phi    = M_PI / 2.0f;

bool   is_dragging   = false;
double last_mouse_x  = 0.0;
double last_mouse_y  = 0.0;

bool  is_space_pressed   = false;
float spin_momentum      = 0.0f;
float global_spin_angle  = 0.0f;
float last_frame_time    = 0.0f;

// ─── "You solved it!" animation state ───────────────────────────────────────
// Two phases:
//   1. snap_in       — camera eases from wherever it was to the magic angle
//   2. lock_pulse    — once on the magic angle, a brief scale + brightness
//                      pulse plays so the solve feels satisfying.
// Triggered by S key OR auto-triggered when the user drags the camera within
// a small tolerance of the magic angle. Cancelled by R / SPACE / A or by the
// user dragging the camera back out of tolerance.
static bool  anim_snapping       = false;
static float anim_start_time     = 0.0f;
static float anim_start_theta    = 0.0f;
static float anim_start_phi      = 0.0f;
// The actual target pose for the in-flight snap animation AND the pinned
// pose while is_locked is true. The "magic" condition is a whole 1-param
// family (eye_x = 0 — see below), not a single angle, so when the player
// finds the alignment from the BACK side we lock there instead of teleport-
// ing them to the front.
static float anim_target_theta   = 0.0f;
static float anim_target_phi     = 0.0f;
static constexpr float ANIM_SNAP_DURATION = 0.85f;   // seconds — camera ease-in (longer for smoother feel)

static bool  is_locked           = false;
static float lock_start_time     = 0.0f;
static constexpr float LOCK_PULSE_DURATION = 1.0f;   // seconds — bump+glow window

// The depth-trick cuts on bar 0's two halves are flat planes perpendicular
// to the X axis. They project to a single vertical screen line — making the
// illusion click — whenever the camera's view direction has zero X
// component. With camera at
//     eye = R · ( sin φ cos θ,   cos φ,   sin φ sin θ )
// that's the condition  sin φ · cos θ = 0,  i.e. eye_x = 0. The locus is a
// CIRCLE in the YZ plane: theta = π/2 (front meridian) OR theta = -π/2 ≡
// 3π/2 (back meridian), with any φ. So we detect alignment by |eye_x|, and
// snap to whichever meridian (front/back) is closer.
static constexpr float CANONICAL_THETA = M_PI / 2.0f;
static constexpr float CANONICAL_PHI   = M_PI / 2.0f;

// Tolerance is now on eye_x magnitude (proportional to camera_radius).
// Tightened — the helper was too aggressive, snapping in before the player
// felt they'd actually FOUND the angle. 0.035 ≈ 2.0° off the magic YZ
// plane, which is "you just got it" territory.
static constexpr float AUTO_SNAP_EYEX_TOL = 0.035f;
static constexpr float UNLOCK_EYEX_TOL    = 0.18f;   // hysteresis (room to escape)

// Forward decls — bodies live further down with the keyboard handler.
static unsigned g_polygon_session_seed = 0;
static void polygon_randomize_unsolved();
static void polygon_solve();
static void polygon_apply_pose_for(int n);
static void polygon_start_snap_animation();
static void polygon_cancel_lock();

// =============================================
// Geometry helpers
// =============================================

void generate_sphere()
{
    sphere_vertices.clear();
    sphere_normals.clear();

    for (int i = 0; i < SPHERE_STACKS; i++)
    {
        float phi1 = M_PI * i / SPHERE_STACKS;
        float phi2 = M_PI * (i + 1) / SPHERE_STACKS;

        for (int j = 0; j < SPHERE_SLICES; j++)
        {
            float theta1 = 2 * M_PI * j / SPHERE_SLICES;
            float theta2 = 2 * M_PI * (j + 1) / SPHERE_SLICES;

            vec3 v[4] = {
                vec3(sinf(phi1) * cosf(theta1), cosf(phi1), sinf(phi1) * sinf(theta1)),
                vec3(sinf(phi2) * cosf(theta1), cosf(phi2), sinf(phi2) * sinf(theta1)),
                vec3(sinf(phi2) * cosf(theta2), cosf(phi2), sinf(phi2) * sinf(theta2)),
                vec3(sinf(phi1) * cosf(theta2), cosf(phi1), sinf(phi1) * sinf(theta2)),
            };

            sphere_vertices.push_back(v[0]); sphere_normals.push_back(v[0]);
            sphere_vertices.push_back(v[1]); sphere_normals.push_back(v[1]);
            sphere_vertices.push_back(v[2]); sphere_normals.push_back(v[2]);
            sphere_vertices.push_back(v[0]); sphere_normals.push_back(v[0]);
            sphere_vertices.push_back(v[2]); sphere_normals.push_back(v[2]);
            sphere_vertices.push_back(v[3]); sphere_normals.push_back(v[3]);
        }
    }
}

static void set_face_normal(vec3* normals, int start_idx, vec3 a, vec3 b, vec3 c)
{
    vec3 n = normalize(cross(b - a, c - a));
    normals[start_idx + 0] = n;
    normals[start_idx + 1] = n;
    normals[start_idx + 2] = n;
}

// Emit the 6 hex-prism faces of a bar piece into out_verts/out_norms (36 verts).
//
// Diamond cross-section (4 vertices, 4 slanted long faces meeting at a top
// ridge and a bottom ridge). The chamfered profile is what makes the depth-
// trick illusion read cleanly when viewed down the magic axis — a flat-top
// rectangular bar covers too much screen width and breaks the alignment of
// bar 0's two halves with bars 1 and N-1.
//
//   Cross-section (looking down the bar's length axis x):
//             +z
//              |
//            * top ridge (y=0, z=+half_thick)
//           / \
//          /   \
//   inner *     * outer
//   y=-h_t \   / y=+h_t
//           \ /
//            * bot ridge (y=0, z=-half_thick)
//              |
//             -z
//
// The ball rolls on the TOP-OUTER slanted face (the upper-right slope when
// looking from outside the polygon), not on the thin ridge line itself.
// See polygon_display for the ball positioning math.
//
// `L_left`/`L_right`: tip half-lengths. `mitre_*`: true → tip is mitered
// outward by dx_inner / dx_outer to fit the polygon corner; false → flat
// vertical cut (used for the two halves of bar 0).
static void build_bar_geometry(vec3* out_verts, vec3* out_norms,
                               int n_segments, float radius, float thickness,
                               float L_left, float L_right,
                               bool mitre_left, bool mitre_right)
{
    float half_thick = thickness / 2.0f;
    float ridge_z    = half_thick;

    float dx_inner  = (half_thick / sinf(2 * M_PI / n_segments)) - (half_thick * tanf(M_PI / n_segments));
    float dx_center = (half_thick / sinf(2 * M_PI / n_segments));
    float dx_outer  = (half_thick / sinf(2 * M_PI / n_segments)) - (half_thick / tanf(2 * M_PI / n_segments));

    float dxi_L = mitre_left  ? dx_inner  : 0.0f;
    float dxc_L = mitre_left  ? dx_center : 0.0f;
    float dxo_L = mitre_left  ? dx_outer  : 0.0f;
    float dxi_R = mitre_right ? dx_inner  : 0.0f;
    float dxc_R = mitre_right ? dx_center : 0.0f;
    float dxo_R = mitre_right ? dx_outer  : 0.0f;

    vec3 v_in_L  = vec3(-L_left  - dxi_L, -half_thick, 0.0f);
    vec3 v_in_R  = vec3( L_right + dxi_R, -half_thick, 0.0f);

    vec3 v_rid_L = vec3(-L_left  - dxc_L, 0.0f,  ridge_z);
    vec3 v_rid_R = vec3( L_right + dxc_R, 0.0f,  ridge_z);
    vec3 v_bot_L = vec3(-L_left  - dxc_L, 0.0f, -ridge_z);
    vec3 v_bot_R = vec3( L_right + dxc_R, 0.0f, -ridge_z);

    vec3 v_out_L = vec3(-L_left  - dxo_L,  half_thick, 0.0f);
    vec3 v_out_R = vec3( L_right + dxo_R,  half_thick, 0.0f);

    int idx = 0;
    // Top inner
    out_verts[idx++] = v_in_L;  out_verts[idx++] = v_in_R;  out_verts[idx++] = v_rid_R;
    out_verts[idx++] = v_in_L;  out_verts[idx++] = v_rid_R; out_verts[idx++] = v_rid_L;
    // Top outer
    out_verts[idx++] = v_rid_L; out_verts[idx++] = v_rid_R; out_verts[idx++] = v_out_R;
    out_verts[idx++] = v_rid_L; out_verts[idx++] = v_out_R; out_verts[idx++] = v_out_L;
    // Bottom outer
    out_verts[idx++] = v_out_L; out_verts[idx++] = v_out_R; out_verts[idx++] = v_bot_R;
    out_verts[idx++] = v_out_L; out_verts[idx++] = v_bot_R; out_verts[idx++] = v_bot_L;
    // Bottom inner
    out_verts[idx++] = v_bot_L; out_verts[idx++] = v_bot_R; out_verts[idx++] = v_in_R;
    out_verts[idx++] = v_bot_L; out_verts[idx++] = v_in_R;  out_verts[idx++] = v_in_L;
    // Left tip
    out_verts[idx++] = v_in_L;  out_verts[idx++] = v_rid_L; out_verts[idx++] = v_bot_L;
    out_verts[idx++] = v_rid_L; out_verts[idx++] = v_bot_L; out_verts[idx++] = v_out_L;
    // Right tip
    out_verts[idx++] = v_in_R;  out_verts[idx++] = v_rid_R; out_verts[idx++] = v_bot_R;
    out_verts[idx++] = v_rid_R; out_verts[idx++] = v_bot_R; out_verts[idx++] = v_out_R;

    set_face_normal(out_norms,  0, v_in_L,  v_in_R,  v_rid_R);
    set_face_normal(out_norms,  3, v_in_L,  v_rid_R, v_rid_L);
    set_face_normal(out_norms,  6, v_rid_L, v_rid_R, v_out_R);
    set_face_normal(out_norms,  9, v_rid_L, v_out_R, v_out_L);
    set_face_normal(out_norms, 12, v_out_L, v_out_R, v_bot_R);
    set_face_normal(out_norms, 15, v_out_L, v_bot_R, v_bot_L);
    set_face_normal(out_norms, 18, v_bot_L, v_bot_R, v_in_R);
    set_face_normal(out_norms, 21, v_bot_L, v_in_R,  v_in_L);
    set_face_normal(out_norms, 24, v_in_L,  v_rid_L, v_bot_L);
    set_face_normal(out_norms, 27, v_rid_L, v_bot_L, v_out_L);
    set_face_normal(out_norms, 30, v_in_R,  v_rid_R, v_bot_R);
    set_face_normal(out_norms, 33, v_rid_R, v_bot_R, v_out_R);
}

void polygon_create_solid_segment(int n_segments, float radius, float thickness)
{
    float L = radius * tanf(M_PI / n_segments);

    // Full bar: mitered on both ends (joins polygon corners on both sides).
    build_bar_geometry(segment_vertices, segment_normals,
                       n_segments, radius, thickness,
                       L, L, /*mitre_left=*/true, /*mitre_right=*/true);

    // Half bar: half-length on each side. Mitered LEFT tip (joins a polygon
    // corner like a normal bar), FLAT-CUT right tip at local x=+L/2 (the
    // bisect cut where two halves of bar 0 will visually align in screen-x).
    build_bar_geometry(half_segment_vertices, half_segment_normals,
                       n_segments, radius, thickness,
                       L * 0.5f, L * 0.5f, /*mitre_left=*/true, /*mitre_right=*/false);
}

void polygon_init()
{
    // Session seed — used so each launch of the app gets a different set of
    // per-N poses, but those poses stay consistent across R presses within a
    // single session.
    g_polygon_session_seed = (unsigned)time(nullptr);

    polygon_create_solid_segment(num_segments, radius, POLYGON_BAR_THICKNESS * scale_factor);

    program = InitShader("../shaders/vshader_new.glsl", "../shaders/fshader_new.glsl");
    glUseProgram(program);

    GLint vert_ok, frag_ok, link_ok;
    GLuint shaders[2];
    GLsizei count;
    glGetAttachedShaders(program, 2, &count, shaders);
    glGetShaderiv(shaders[0], GL_COMPILE_STATUS, &vert_ok);
    glGetShaderiv(shaders[1], GL_COMPILE_STATUS, &frag_ok);
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if (!vert_ok || !frag_ok || !link_ok)
    {
        char log[2048];
        glGetProgramInfoLog(program, 2048, nullptr, log);
        fprintf(stderr, "SHADER ERROR: %s\n", log);
        exit(1);
    }

    GLint loc        = glGetAttribLocation(program, "vPosition");
    GLint normal_loc = glGetAttribLocation(program, "vNormal");
    color_loc        = glGetUniformLocation(program, "uFaceColor");
    mvp_loc          = glGetUniformLocation(program, "MVP");
    light_pos_loc    = glGetUniformLocation(program, "uLightPos");
    eye_pos_loc      = glGetUniformLocation(program, "uEyePos");
    model_loc        = glGetUniformLocation(program, "uModel");
    bar_t_loc        = glGetUniformLocation(program, "uBarT");
    base_color_loc   = glGetUniformLocation(program, "uBaseColor");
    num_segments_loc = glGetUniformLocation(program, "uNumSegments");
    ball_pos_loc     = glGetUniformLocation(program, "uBallPos");
    is_ball_loc      = glGetUniformLocation(program, "uIsBall");
    ball_color_loc   = glGetUniformLocation(program, "uBallColor");

    // Full-bar VAO
    glGenVertexArrays(1, &segment_vao);
    glBindVertexArray(segment_vao);

    glGenBuffers(1, &segment_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, segment_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(segment_vertices), segment_vertices, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glGenBuffers(1, &segment_nbo);
    glBindBuffer(GL_ARRAY_BUFFER, segment_nbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(segment_normals), segment_normals, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(normal_loc);
    glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    // Half-bar VAO (used twice per frame to draw the two halves of bar 0)
    glGenVertexArrays(1, &half_segment_vao);
    glBindVertexArray(half_segment_vao);

    glGenBuffers(1, &half_segment_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, half_segment_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(half_segment_vertices), half_segment_vertices, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glGenBuffers(1, &half_segment_nbo);
    glBindBuffer(GL_ARRAY_BUFFER, half_segment_nbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(half_segment_normals), half_segment_normals, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(normal_loc);
    glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    // Ball
    generate_sphere();

    glGenVertexArrays(1, &sphere_vao);
    glBindVertexArray(sphere_vao);

    glGenBuffers(1, &sphere_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo);
    glBufferData(GL_ARRAY_BUFFER, sphere_vertices.size() * sizeof(vec3), sphere_vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glGenBuffers(1, &sphere_nbo_id);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_nbo_id);
    glBufferData(GL_ARRAY_BUFFER, sphere_normals.size() * sizeof(vec3), sphere_normals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(normal_loc);
    glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glBindVertexArray(0);

    // Pick a random N and a random off-axis camera angle so the figure starts
    // in the "unsolved" pose — player has to press S to snap it back to the
    // magic angle and see the illusion click.
    polygon_randomize_unsolved();

    // Initialise the bloom pipeline + background shaders.
    // Must come AFTER the GL context is current and screen_w / screen_h are set.
    lock_glow_loc = glGetUniformLocation(program, "uLockGlow");
    polygon_bg_init();
}

// =============================================
// Ball
// =============================================
void display_ball(mat4 viewProj, mat4 global_spin, vec3 local_pos, vec3 ballColor)
{
    vec4 wbp = global_spin * vec4(local_pos.x, local_pos.y, local_pos.z, 1.0f);
    vec3 world_ball_pos(wbp.x, wbp.y, wbp.z);
    glUniform3fv(ball_pos_loc, 1, &world_ball_pos.x);

    mat4 ball_translate =
        Translate(local_pos.x, local_pos.y, local_pos.z) * Scale(BALL_RADIUS, BALL_RADIUS, BALL_RADIUS);
    mat4 ball_mvp   = viewProj * global_spin * ball_translate;
    mat4 ball_model = global_spin * ball_translate;

    glBindVertexArray(sphere_vao);
    glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &ball_mvp.d[0].x);
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, &ball_model.d[0].x);

    glUniform1i(is_ball_loc, 1);
    glUniform3fv(base_color_loc, 1, &ballColor.x);
    glUniform1f(bar_t_loc, ball_s);   // ball_s drives the Escher gradient too

    // The ball is conceptually a 2D "you solved it" indicator riding the
    // figure — not a 3D object that should be occluded by the polygon. With
    // the impossible-polygon's depth-trick (bars spiraling z, bar 0 split
    // into two halves at top/bottom z), the ball would get clipped behind
    // bar tips at every corner and behind bar 0 LEFT during the seam
    // crossing. Disable depth test for the WHOLE ball draw so it always
    // floats on top of the loop wherever the orbit takes it.
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLES, 0, sphere_vertices.size());
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

// =============================================
// Display
// =============================================
void polygon_display()
{
    float eye_x = camera_radius * sinf(camera_phi) * cosf(camera_theta);
    float eye_y = camera_radius * cosf(camera_phi);
    float eye_z = camera_radius * sinf(camera_phi) * sinf(camera_theta);

    vec3 eye(eye_x, eye_y, eye_z);
    vec3 at(0.0f, 0.0f, 0.0f);
    vec3 up(0.0f, 1.0f, 0.0f);
    mat4 view = LookAt(eye, at, up);

    float view_size = scale_factor + 0.5f;
    // Aspect-correct ortho so the polygon doesn't squash on non-square windows.
    float aspect = (screen_w > 0 && screen_h > 0) ? (float)screen_w / (float)screen_h : 1.0f;
    mat4 proj = Ortho(-view_size * aspect, view_size * aspect,
                      -view_size,          view_size,
                      -1000.0f, 1000.0f);

    float current_time = glfwGetTime();

    glUseProgram(program);

    // --- SPIN MATH (hold SPACE → momentum spin) ---
    if (last_frame_time == 0.0f) last_frame_time = current_time;
    float delta_time = current_time - last_frame_time;
    last_frame_time  = current_time;

    if (is_space_pressed)
    {
        spin_momentum += 0.4f * delta_time;
        if (spin_momentum > 1.0f) spin_momentum = 1.0f;
    }
    else
    {
        spin_momentum -= 0.6f * delta_time;
        if (spin_momentum < 0.0f) spin_momentum = 0.0f;
    }
    float spin_speed = (spin_momentum * spin_momentum * spin_momentum) * 2000.0f;
    global_spin_angle += spin_speed * delta_time;

    // ─── SNAP-TO-SOLVED / LOCK animation ──────────────────────────────────
    // 1. If we're inside the snap animation, ease camera_theta/phi toward
    //    the magic angle. When done, kick off the lock pulse.
    // 2. If we're NOT animating or locked and the user is dragging close
    //    enough to magic, auto-trigger the snap.
    // 3. If we're locked and the user dragged well out of tolerance, drop
    //    the lock so they can play with the figure again.
    // Illusion-visible region: view direction must be in the YZ plane
    // (eye_x = 0) AND φ near π/2 so LookAt doesn't go degenerate (which
    // happens when up is parallel to view direction, near φ = 0 or π).
    float eye_x_unit = sinf(camera_phi) * cosf(camera_theta);
    float eye_y_unit = cosf(camera_phi);                        // = 0 at φ = π/2

    if (anim_snapping)
    {
        float t = (current_time - anim_start_time) / ANIM_SNAP_DURATION;
        if (t >= 1.0f) {
            t = 1.0f;
            anim_snapping  = false;
            is_locked      = true;
            lock_start_time = current_time;
        }
        // Smootherstep (Ken Perlin's improved smoothstep) — zero velocity AND
        // zero acceleration at both ends.  6t⁵ − 15t⁴ + 10t³.  Reads much
        // smoother than ease-out cubic, which has a noticeably abrupt start.
        float et = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
        camera_theta = anim_start_theta + (anim_target_theta - anim_start_theta) * et;
        camera_phi   = anim_start_phi   + (anim_target_phi   - anim_start_phi)   * et;
    }
    else if (!is_locked && !is_space_pressed)
    {
        // Auto-snap fires when the camera is BOTH in the (tight) magic
        // YZ-plane band AND close enough to the top-down meridian that the
        // illusion will look clean once we snap (no LookAt degeneracy).
        // Tightened phi-band too (was 0.30) — the player has to find the
        // pose, not just drift in its general direction.
        if (fabsf(eye_x_unit) < AUTO_SNAP_EYEX_TOL && fabsf(eye_y_unit) < 0.15f)
            polygon_start_snap_animation();
    }
    else if (is_locked && is_dragging)
    {
        // Locked + dragging out of either tolerance → release.
        if (fabsf(eye_x_unit) > UNLOCK_EYEX_TOL || fabsf(eye_y_unit) > 0.40f)
            is_locked = false;
    }

    // While locked, the camera is held at whichever magic pose we snapped
    // to (front OR back) so the player can't accidentally drift off it
    // during the celebration pulse.
    if (is_locked) {
        camera_theta = anim_target_theta;
        camera_phi   = anim_target_phi;
    }

    // Lock-pulse factors driven by elapsed time since lock latched. Fades to
    // zero after LOCK_PULSE_DURATION so the figure settles to its normal look.
    float lock_scale = 1.0f;
    float lock_glow  = 0.0f;
    if (is_locked) {
        float lt = (current_time - lock_start_time) / LOCK_PULSE_DURATION;
        if (lt < 1.0f) {
            // Critically-damped overshoot: bump up, settle back to 1.
            //   bump(t) = sin(π t) * (1 - t)   → peaks ~0.385 at t≈0.3
            float bump  = sinf((float)M_PI * lt) * (1.0f - lt);
            lock_scale = 1.0f + 0.08f * bump;
            // Glow rides the bump but fades out faster.
            lock_glow  = 0.45f * bump;
        }
    }

    // Apply the lock pulse to the camera radius too so the bump reads from
    // every angle (otherwise pure model scale + ortho fight each other).
    mat4 lock_scale_mat = Scale(lock_scale, lock_scale, lock_scale);

    mat4 global_spin =
        RotateX(global_spin_angle) * RotateY(global_spin_angle * 1.3f) * RotateZ(global_spin_angle * 0.7f);
    mat4 viewProj = proj * view;

    // Note: view/proj have to be recomputed since camera_theta/phi changed
    // (the animation modified them).
    {
        float ex = camera_radius * sinf(camera_phi) * cosf(camera_theta);
        float ey = camera_radius * cosf(camera_phi);
        float ez = camera_radius * sinf(camera_phi) * sinf(camera_theta);
        eye = vec3(ex, ey, ez);
        view = LookAt(eye, at, up);
        viewProj = proj * view;
    }

    // ── Background bloom pre-pass ──────────────────────────────────────────
    // begin_scene binds the HDR scene FBO and clears it.  bg_draw renders
    // the chosen background (camera-aware, no depth write) before the
    // polygon geometry is drawn into the same FBO.
    polygon_bg_begin_scene();
    {
        vec3 fwd = normalize(at - eye);               // eye → origin
        vec3 rgt = normalize(cross(fwd, vec3(0,1,0)));
        // Guard against degenerate up (camera directly above/below)
        if (rgt.x == 0.0f && rgt.y == 0.0f && rgt.z == 0.0f)
            rgt = vec3(1, 0, 0);
        vec3 bgUp = normalize(cross(rgt, fwd));
        polygon_bg_draw(eye, rgt, bgUp, fwd, aspect, current_time, num_segments);
    }
    // Restore the polygon's shader program after the background draw changed it.
    glUseProgram(program);
    // ──────────────────────────────────────────────────────────────────────

    vec3 lightPos(2.0f, 3.0f, 2.0f);
    glUniform3fv(light_pos_loc, 1, &lightPos.x);
    glUniform3fv(eye_pos_loc, 1, &eye.x);
    glUniform1i(num_segments_loc, num_segments);
    // Upload lock glow: sustained low-level HDR boost while locked + pulse burst
    if (lock_glow_loc >= 0) {
        float sustained = is_locked ? 0.12f : 0.0f;
        glUniform1f(lock_glow_loc, sustained + lock_glow);
    }

    // ─── BALL POSITION on the figure's surface ─────────────────────────────
    // The ball lives in two surface coordinates (ball_s, ball_u) — see decl.
    //
    // Update rules:
    //   - Unlocked → reset to defaults (so each new solve starts cleanly).
    //   - Locked, WASD pressed → drive (s, u) directly.
    //   - Locked, no WASD for IDLE_DRIFT_THRESHOLD_SEC → resume auto-orbit
    //     on s so the polygon doesn't look frozen when idle.
    if (!is_locked)
    {
        ball_s = 0.0f;
        ball_u = 1.5f;
    }
    else
    {
        float s_input = 0.0f, u_input = 0.0f;
        if (key_w_held) s_input += 1.0f;
        if (key_s_held) s_input -= 1.0f;
        if (key_d_held) u_input += 1.0f;
        if (key_a_held) u_input -= 1.0f;

        if (s_input != 0.0f || u_input != 0.0f) {
            ball_s += s_input * BALL_S_SPEED * delta_time;
            ball_u += u_input * BALL_U_SPEED * delta_time;
        } else {
            // Idle drift kicks in only after the player has gone silent for
            // a beat — that way a quick stop doesn't immediately start the
            // ball moving away from where the player parked it.
            double now = glfwGetTime();
            if (now - last_wasd_input_time > (double)IDLE_DRIFT_THRESHOLD_SEC)
                ball_s += BALL_S_SPEED * delta_time;
        }
        // Wrap (ball_s ∈ [0,1), ball_u ∈ [0,4)).
        ball_s -= floorf(ball_s);
        ball_u -= 4.0f * floorf(ball_u * 0.25f);
    }

    float thickness  = POLYGON_BAR_THICKNESS * scale_factor;
    float half_thick = thickness / 2.0f;

    // ─── 1. Decide which bar segment the ball is on, and its bar-local x. ──
    // Loop topology (N+1 sub-segments because bar 0 is split into 2 halves):
    //   ball_s ∈ [0,        0.5/N] → bar 0 LEFT  (half),  local x : +L/2 → -L/2
    //   ball_s ∈ [(k-0.5)/N, (k+0.5)/N] → bar k  (full),  local x : +L   → -L
    //   ball_s ∈ [(N-0.5)/N, 1]    → bar 0 RIGHT (half),  local x : -L/2 → +L/2
    //
    // Each bar has its own model matrix (the same one used to draw it).
    // World ball position = bar_model · (x_local, y_local, z_local).
    float L = radius * tanf(M_PI / (float)num_segments);
    mat4  bar_model;
    float x_local;
    {
        int N  = num_segments;
        int k  = (int)floorf(ball_s * (float)N + 0.5f);
        if (k == 0) {
            // bar 0 LEFT half
            x_local   = L * 0.5f - 2.0f * (float)N * L * ball_s;
            bar_model = Translate(-L * 0.5f, radius, (float)N * zStep);
        } else if (k >= N) {
            // bar 0 RIGHT half
            float ts  = ball_s - (float)(N) / (float)N + 0.5f / (float)N;  // ts ∈ [0, 0.5/N]
            x_local   = -L * 0.5f + 2.0f * (float)N * L * ts;
            bar_model = Translate(L * 0.5f, radius, 0.0f) * RotateZ(180.0f) * RotateX(180.0f);
        } else {
            // full bar k (k=1..N-1)
            float ts  = ball_s - ((float)k - 0.5f) / (float)N;             // ts ∈ [0, 1/N]
            x_local   = L - 2.0f * (float)N * L * ts;
            float angle  = (float)k * (360.0f / (float)N);
            float zDepth = (float)(N - k) * zStep;
            bar_model = RotateZ(angle) * Translate(0.0f, radius, zDepth);
        }
    }

    // ─── 2. Decide (y_local, z_local) on the diamond cross-section. ────────
    // Diamond perimeter corners (in bar-local y,z):
    //     ball_u = 0  →  in  (-h, 0)
    //     ball_u = 1  →  rid ( 0, +h)
    //     ball_u = 2  →  out (+h, 0)
    //     ball_u = 3  →  bot ( 0, -h)
    // Fractional ball_u interpolates along the face between two corners, so
    // the ball can smoothly roll OVER a ridge. Offset by BALL_RADIUS along
    // the face's outward normal so the ball rests ON the surface, not in it.
    const float SQRT2_INV = 0.70710678f;
    float y_local, z_local;
    {
        int   face = (int)floorf(ball_u);
        face       = ((face % 4) + 4) % 4;
        float ft   = ball_u - floorf(ball_u);
        float ay, az, by, bz, ny, nz;
        switch (face) {
            case 0: ay=-half_thick; az=0;          by=0;           bz=+half_thick; ny=-SQRT2_INV; nz=+SQRT2_INV; break;  // in→rid (top-inner face)
            case 1: ay=0;           az=+half_thick;by=+half_thick; bz=0;           ny=+SQRT2_INV; nz=+SQRT2_INV; break;  // rid→out (top-outer face)
            case 2: ay=+half_thick; az=0;          by=0;           bz=-half_thick; ny=+SQRT2_INV; nz=-SQRT2_INV; break;  // out→bot (bot-outer face)
            default:ay=0;           az=-half_thick;by=-half_thick; bz=0;           ny=-SQRT2_INV; nz=-SQRT2_INV; break;  // bot→in (bot-inner face)
        }
        float y_perim = ay + ft * (by - ay);
        float z_perim = az + ft * (bz - az);
        y_local = y_perim + BALL_RADIUS * ny;
        z_local = z_perim + BALL_RADIUS * nz;
    }

    // ─── 3. Transform to world via the bar's model matrix. ─────────────────
    vec3 ball_local_pos;
    {
        vec4 wp = bar_model * vec4(x_local, y_local, z_local, 1.0f);
        ball_local_pos = vec3(wp.x, wp.y, wp.z);
    }

    // Helper to get adjusted ball position for a given target zDepth
    auto get_adjusted_ball_pos = [&](float zDepth_draw) {
        if (!is_locked) return vec3(1.0e6f, 1.0e6f, 1.0e6f);
        
        int N = num_segments;
        int k = (int)floorf(ball_s * (float)N + 0.5f);
        float zDepth_ball = 0.0f;
        if (k == 0) zDepth_ball = (float)N * zStep;
        else if (k >= N) zDepth_ball = 0.0f;
        else zDepth_ball = (float)(N - k) * zStep;

        vec3 light = ball_local_pos;
        // Adjust the Z coordinate of the ball to match the target Z plane
        light.z = light.z - zDepth_ball + zDepth_draw;
        return light;
    };

    // ── Per-background colour palette ─────────────────────────────────────────
    // Background index = (num_segments - 3) % 4  (mirrors polygon_bg.cpp)
    //   0 → Escher corridor  : jade / emerald greens
    //   1 → Mandelbulb       : deep violet + warm gold
    //   2 → Gyroid SDF       : dark teal + cyan
    //   3 → Neon N-gon tunnel: electric cyan + hot magenta
    vec3 topColor, botColor, tipColor;
    vec3 ballColor;
    switch ((num_segments - 3) % 4) {
        case 0:  // jade-green — echoes the Escher corridor walls
            topColor = vec3(0.55f, 0.82f, 0.58f);
            botColor = vec3(0.28f, 0.58f, 0.36f);
            tipColor = vec3(0.14f, 0.38f, 0.22f);
            ballColor = vec3(0.20f, 0.95f, 0.40f); // bright neon green
            break;
        case 1:  // violet + gold — matches Mandelbulb deep-purple/gold palette
            topColor = vec3(0.72f, 0.55f, 0.92f);
            botColor = vec3(0.48f, 0.20f, 0.70f);
            tipColor = vec3(0.82f, 0.68f, 0.18f);
            ballColor = vec3(1.00f, 0.82f, 0.15f); // bright golden yellow
            break;
        case 2:  // teal + ice-cyan — matches Gyroid teal/navy palette
            topColor = vec3(0.40f, 0.82f, 0.82f);
            botColor = vec3(0.14f, 0.52f, 0.62f);
            tipColor = vec3(0.06f, 0.30f, 0.42f);
            ballColor = vec3(0.00f, 0.95f, 0.95f); // bright electric cyan
            break;
        default: // electric cyan + magenta — matches Neon tunnel palette
            topColor = vec3(0.15f, 0.88f, 0.95f);
            botColor = vec3(0.88f, 0.18f, 0.65f);
            tipColor = vec3(0.06f, 0.55f, 0.72f);
            ballColor = vec3(1.00f, 0.15f, 0.75f); // bright hot pink/magenta
            break;
    }

    glUniform3fv(ball_color_loc, 1, &ballColor.x);

    // While locked, brighten the palette by lock_glow so the figure visibly
    // pulses. Clamp to [0,1] so we don't blow out the shader.
    auto add_glow = [&](vec3 c) {
        c.x = fminf(1.0f, c.x + lock_glow);
        c.y = fminf(1.0f, c.y + lock_glow);
        c.z = fminf(1.0f, c.z + lock_glow);
        return c;
    };
    vec3 topC = add_glow(topColor);
    vec3 botC = add_glow(botColor);
    vec3 tipC = add_glow(tipColor);


    // Helper: draw one bar's faces in 3 colored chunks (12 verts each:
    // 0..11 = top faces, 12..23 = bottom faces, 24..35 = tip caps).
    // Matches the simple 3-tone look of slot 2's Penrose Triangle.
    auto draw_bar_tritone = [&]() {
        glUniform3fv(base_color_loc, 1, &topC.x); glDrawArrays(GL_TRIANGLES,  0, 12);
        glUniform3fv(base_color_loc, 1, &botC.x); glDrawArrays(GL_TRIANGLES, 12, 12);
        glUniform3fv(base_color_loc, 1, &tipC.x); glDrawArrays(GL_TRIANGLES, 24, 12);
    };

    // Bars 1 .. N-1 — full mitered segments, spiraling down in z.
    for (int bar_index = 1; bar_index < num_segments; bar_index++)
    {
        float angle  = bar_index * (360.0f / num_segments);
        float zDepth = -bar_index * zStep + (num_segments * zStep);
        float bar_t  = (float)bar_index / num_segments;

        vec3 light_for_this_bar = get_adjusted_ball_pos(zDepth);
        vec4 wbp = global_spin * vec4(light_for_this_bar.x, light_for_this_bar.y, light_for_this_bar.z, 1.0f);
        glUniform3fv(ball_pos_loc, 1, &wbp.x);

        mat4 model = lock_scale_mat * RotateZ(angle) * Translate(0.0f, radius, zDepth);
        mat4 mvp   = viewProj * global_spin * model;
        mat4 world_model = global_spin * model;

        glBindVertexArray(segment_vao);
        glUniform1i(is_ball_loc, 0);
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &mvp.d[0].x);
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, &world_model.d[0].x);
        glUniform1f(bar_t_loc, bar_t);
        draw_bar_tritone();
    }

    // Bar 0 — THE DEPTH-TRICK BAR. Drawn as two half-segments at different z.
    {
        float zDepth_top = num_segments * zStep;  // bar 0's nominal z
        float zDepth_bot = 0.0f;                  // jumped-down z for right half

        glUniform1i(is_ball_loc, 0);
        glUniform1f(bar_t_loc, 0.0f);

        glBindVertexArray(half_segment_vao);

        // Left half — bar 0 LEFT is the "start of the loop" (bar_t = 0).
        vec3 light_l = get_adjusted_ball_pos(zDepth_top);
        vec4 wbp_l = global_spin * vec4(light_l.x, light_l.y, light_l.z, 1.0f);
        glUniform3fv(ball_pos_loc, 1, &wbp_l.x);

        mat4 model_l = lock_scale_mat * Translate(-L * 0.5f, radius, zDepth_top);
        mat4 mvp_l   = viewProj * global_spin * model_l;
        mat4 wmodel_l = global_spin * model_l;
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &mvp_l.d[0].x);
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, &wmodel_l.d[0].x);
        draw_bar_tritone();

        // Right half — bar 0 RIGHT is the "end of the loop" (bar_t = 1).
        vec3 light_r = get_adjusted_ball_pos(zDepth_bot);
        vec4 wbp_r = global_spin * vec4(light_r.x, light_r.y, light_r.z, 1.0f);
        glUniform3fv(ball_pos_loc, 1, &wbp_r.x);

        mat4 model_r = lock_scale_mat * Translate(L * 0.5f, radius, zDepth_bot) * RotateZ(180.0f) * RotateX(180.0f);
        mat4 mvp_r    = viewProj * global_spin * model_r;
        mat4 wmodel_r = global_spin * model_r;
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &mvp_r.d[0].x);
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, &wmodel_r.d[0].x);
        draw_bar_tritone();
    }

    // Ball — only orbits / draws when the polygon is solved + locked.
    // It's the "reward" indicator: solve it, and the little yellow ball runs
    // around the impossible loop, jumping the seam invisibly.
    if (is_locked) display_ball(viewProj, global_spin, ball_local_pos, ballColor);

    // ── Bloom composite: extract bright → Gaussian blur → add to scene ────
    polygon_bg_end_scene();

    glFinish();
}

// =============================================
// Reconfigure for a new segment count (SPACE / A)
//
// Does NOT touch camera_theta/phi — so growing/shrinking the polygon doesn't
// snap the camera back to the magic angle. That snap is reserved for the
// S-key ("solve"), and the R-key ("randomize unsolved").
// =============================================
void polygon_set_constants(int n_segments)
{
    num_segments = n_segments;

    scale_factor = n_segments / 3.0f;
    radius       = 0.70f * scale_factor;  // matches file-scope default
    zStep        = 0.55f * scale_factor;  // matches file-scope default (dramatic spiral)

    polygon_create_solid_segment(n_segments, radius, POLYGON_BAR_THICKNESS * scale_factor);

    generate_sphere();

    glBindBuffer(GL_ARRAY_BUFFER, segment_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(segment_vertices), segment_vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, segment_nbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(segment_normals), segment_normals, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, half_segment_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(half_segment_vertices), half_segment_vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, half_segment_nbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(half_segment_normals), half_segment_normals, GL_DYNAMIC_DRAW);

    glUniform1i(num_segments_loc, num_segments);
}

// ─────────────────────────────────────────────────────────────────────────────
// Per-N "unsolved" pose.
//
// The other slots (2-8) each have a fixed defaultAngle* triple so R always
// returns to that same starting pose. For the polygon, N is dynamic (SPACE
// grows it, A shrinks it) — so we need ONE fixed unsolved pose PER N, not
// per slot. We compute it from a small LCG seeded by (session_seed XOR n):
//
//   - Same N within one session → same pose every time you press R.
//   - Different N (e.g. after SPACE) → different pose, distinctive per N.
//   - Different session → different set of poses (so the app feels fresh).
// ─────────────────────────────────────────────────────────────────────────────

// (g_polygon_session_seed is declared up at the top with the forward decls.)

static void polygon_apply_pose_for(int n)
{
    unsigned state = g_polygon_session_seed ^ ((unsigned)n * 2654435761u);
    auto next_signed = [&state]() {
        state = state * 1103515245u + 12345u;
        return (float)((state >> 8) & 0xFFFFFFu) / (float)0x1000000 - 0.5f;  // [-0.5, 0.5)
    };

    // Theta jitter must keep the starting pose CLEARLY off the magic YZ
    // plane (|eye_x| > UNLOCK_EYEX_TOL), otherwise auto-snap fires on init
    // and the player never gets to find the puzzle. Map [-0.5, 0.5] →
    // [-0.85, -0.35] ∪ [0.35, 0.85] — guarantees |sin(jt)| ≥ sin(0.35) ≈
    // 0.343, which gives |eye_x_unit| ≳ 0.30 > UNLOCK_EYEX_TOL (0.22).
    float r  = next_signed();
    float jt = (r >= 0.0f ? 1.0f : -1.0f) * (0.35f + fabsf(r) * 1.0f);
    float jp = next_signed() * 1.0f;   // phi jitter unchanged (±0.5 rad)

    camera_radius = 0.5f;
    camera_theta  = M_PI / 2.0f + jt;
    camera_phi    = M_PI / 2.0f + jp;
    if (camera_phi < 0.15f)         camera_phi = 0.15f;
    if (camera_phi > M_PI - 0.15f)  camera_phi = M_PI - 0.15f;

    global_spin_angle = 0.0f;
    spin_momentum     = 0.0f;
    is_dragging       = false;
    last_mouse_x      = 0.0;
    last_mouse_y      = 0.0;
}

// Back-compat shim — old callers (polygon_init) used this name; now it just
// applies the deterministic per-N pose for the current N.
static void polygon_randomize_unsolved()
{
    polygon_apply_pose_for(num_segments);
}

// Compute the magic camera pose CLOSEST to the current one. The full magic
// locus is eye_x = 0 (theta ≡ ±π/2, any phi), so we snap theta to whichever
// of those two meridians is nearer and keep the user's current phi — that
// way solving from the back doesn't teleport them around to the front.
//
// `out_theta` returned in [-π, π] for clean delta-lerp toward it.
static void polygon_pick_nearest_magic_pose(float& out_theta, float& out_phi)
{
    // Pick the NEAREST magic meridian — front (+π/2) OR back (-π/2) — so
    // the camera stays inside the YZ-plane locus where cuts are aligned.
    // A snap from "back" to "front" would have the camera cross eye_x ≠ 0
    // territory mid-animation and re-cut the figure visually; this avoids
    // that by accepting a "B" pose (the back canonical view) as a valid
    // solved pose. φ is locked to π/2 either way (canonical top-down).
    float t = fmodf(camera_theta, 2.0f * (float)M_PI);
    if (t >  (float)M_PI) t -= 2.0f * (float)M_PI;
    if (t < -(float)M_PI) t += 2.0f * (float)M_PI;
    float d_front = fabsf(t - 0.5f * (float)M_PI);
    float d_back  = fabsf(t + 0.5f * (float)M_PI);
    out_theta = (d_front <= d_back) ? 0.5f * (float)M_PI : -0.5f * (float)M_PI;
    out_phi   = 0.5f * (float)M_PI;
}

// Kick off the smooth ease-in from the current camera pose to the NEAREST
// magic pose (front or back). Called by S key (snaps to the canonical front
// pose explicitly) and auto-fired when the user drags close to the magic
// YZ-plane locus from either side.
static void polygon_start_snap_animation()
{
    if (anim_snapping || is_locked) return;
    polygon_pick_nearest_magic_pose(anim_target_theta, anim_target_phi);
    // Wrap-normalise camera_theta so the lerp goes the SHORT way.
    while (camera_theta - anim_target_theta >  (float)M_PI) camera_theta -= 2.0f * (float)M_PI;
    while (camera_theta - anim_target_theta < -(float)M_PI) camera_theta += 2.0f * (float)M_PI;
    anim_snapping    = true;
    anim_start_time  = (float)glfwGetTime();
    anim_start_theta = camera_theta;
    anim_start_phi   = camera_phi;
    global_spin_angle = 0.0f;
    spin_momentum     = 0.0f;
}

// Drop out of the locked state — user wants to play with the figure again.
// Called by R / SPACE / A and when the user drags well off the magic angle.
static void polygon_cancel_lock()
{
    is_locked      = false;
    anim_snapping  = false;
}

// "Solved" pose for the S-key — trigger the smooth animated snap to the
// magic ortho angle (instant snap replaced with an ease-in). Preserves the
// CURRENT N so the player sees the random polygon they were given click
// into place at its own shape, complete with the lock pulse.
static void polygon_solve()
{
    camera_radius = 0.5f;
    polygon_start_snap_animation();
}

// =============================================
// Input
// =============================================
void polygon_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // CONTEXT-SENSITIVE WASD:
    //   - Unlocked: SPACE / A grow & shrink the polygon, S snaps to solved.
    //   - Locked (puzzle solved): WASD wakes up and drives the ball around
    //     the figure's surface. S no longer triggers a solve (the puzzle's
    //     already solved); it backs the ball up along the loop instead.
    //     The growth keys (SPACE / A) still un-lock + reshape the polygon
    //     since the player explicitly asked to change the puzzle.
    const bool press_or_repeat = (action == GLFW_PRESS) || (action == GLFW_REPEAT);

    // ── WASD held-flag updates (drive the ball when locked) ──
    auto note_wasd_input = [&]() { last_wasd_input_time = glfwGetTime(); };

    if (key == GLFW_KEY_W)
    {
        if (action == GLFW_PRESS)   { key_w_held = true;  note_wasd_input(); }
        if (action == GLFW_RELEASE) { key_w_held = false; }
        if (is_locked) return;   // when locked W is ONLY for the ball
    }
    if (key == GLFW_KEY_D)
    {
        if (action == GLFW_PRESS)   { key_d_held = true;  note_wasd_input(); }
        if (action == GLFW_RELEASE) { key_d_held = false; }
        if (is_locked) return;
    }

    switch (key)
    {
        case GLFW_KEY_SPACE:
            // SPACE: tap-to-grow. Adds an edge AND snaps the camera to the
            // new N's per-N unsolved pose — so each new polygon size starts
            // from its own fresh puzzle position. Hold also keeps the figure
            // spinning (momentum-based spin from before). Cancels any active
            // lock — new puzzle, fresh start.
            if (action == GLFW_PRESS)
            {
                is_space_pressed = true;
                polygon_cancel_lock();
                polygon_set_constants(num_segments + 1);
                polygon_apply_pose_for(num_segments);
            }
            else if (action == GLFW_RELEASE)
            {
                is_space_pressed = false;
            }
            break;

        case GLFW_KEY_A:
            // A: locked → ball CCW around the bar.
            //    unlocked → shrink the polygon (with per-N pose snap).
            if (is_locked) {
                if (action == GLFW_PRESS)   { key_a_held = true;  note_wasd_input(); }
                if (action == GLFW_RELEASE) { key_a_held = false; }
            } else {
                if (action == GLFW_RELEASE) { key_a_held = false; }   // safety
                if (action == GLFW_PRESS && num_segments > 3) {
                    polygon_cancel_lock();
                    polygon_set_constants(num_segments - 1);
                    polygon_apply_pose_for(num_segments);
                }
            }
            break;

        case GLFW_KEY_S:
            // S: locked → ball backward along the loop.
            //    unlocked → animate-snap to the magic ortho angle.
            if (is_locked) {
                if (action == GLFW_PRESS)   { key_s_held = true;  note_wasd_input(); }
                if (action == GLFW_RELEASE) { key_s_held = false; }
            } else {
                if (action == GLFW_RELEASE) { key_s_held = false; }
                if (action == GLFW_PRESS) polygon_solve();
            }
            break;

        case GLFW_KEY_R:
            // R: snap back to THIS N's fixed unsolved pose (same one every
            // press for the same N — like defaultAngle* on slots 2-8).
            if (action == GLFW_PRESS)
            {
                polygon_cancel_lock();
                polygon_apply_pose_for(num_segments);
            }
            break;

        case GLFW_KEY_P:
            // Slot 1's polygon is procedural — there's no static mesh to
            // click. Print a hint so the user knows where the editor lives.
            if (action == GLFW_PRESS) {
                fprintf(stderr, "[path-editor] Slot 1 is procedural — the "
                                "interactive editor only works on OBJ slots (2-8).\n");
                fflush(stderr);
            }
            break;

        case GLFW_KEY_ESCAPE:
        case GLFW_KEY_Q:
            exit(EXIT_SUCCESS);
            break;

        default:
            (void)press_or_repeat;
            break;
    }
}

void polygon_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            is_dragging = true;
            glfwGetCursorPos(window, &last_mouse_x, &last_mouse_y);
        }
        else if (action == GLFW_RELEASE)
        {
            is_dragging = false;
        }
    }
}

void polygon_cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (!is_dragging) return;

    // Always advance last_mouse so when the snap finishes the next drag
    // delta doesn't have a giant accumulated jump baked in.
    double deltaX = xpos - last_mouse_x;
    double deltaY = ypos - last_mouse_y;
    last_mouse_x = xpos;
    last_mouse_y = ypos;

    // While the snap animation is running, the camera is being interpolated
    // by polygon_display every frame. Letting the cursor callback ALSO
    // write camera_theta/phi creates a tug-of-war and the figure jitters
    // through the lock. Drop drag input until the snap animation is done.
    if (anim_snapping) return;

    camera_theta -= deltaX * 0.01f;
    camera_phi   += deltaY * 0.01f;

    if (camera_phi < 0.01f)         camera_phi = 0.01f;
    if (camera_phi > M_PI - 0.01f)  camera_phi = M_PI - 0.01f;
}
