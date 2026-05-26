#include "includes.h"
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

// Ball
float ball_t = 0.0f;  // 0.0 to 1.0, position along the loop
const int   SPHERE_STACKS = 10;
const int   SPHERE_SLICES = 10;
const float BALL_RADIUS   = 0.04f;
std::vector<vec3> sphere_vertices;
std::vector<vec3> sphere_normals;
GLuint sphere_vao = 0, sphere_vbo = 0, sphere_nbo_id = 0;
GLuint ball_pos_loc = 0, is_ball_loc = 0;

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

// Forward decls — bodies live further down with the keyboard handler.
static unsigned g_polygon_session_seed = 0;
static void polygon_randomize_unsolved();
static void polygon_solve();
static void polygon_apply_pose_for(int n);

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
// `L_left` / `L_right` are the left/right tip half-lengths (so left edge sits at
// -L_left - dx_*, right edge at +L_right + dx_*). The mitre offsets `dx_*`
// EXTEND the bar outward (they overshoot the nominal length so the bar's tip
// can be sliced at the polygon's corner angle and still join its neighbour
// cleanly). `mitre_left` / `mitre_right` say whether that tip is mitered (true)
// or flat-cut (false, used for the bisect cut on the two halves of bar 0).
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
}

// =============================================
// Ball
// =============================================
void display_ball(mat4 viewProj, mat4 global_spin, vec3 local_pos)
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
    vec3 ballColor(1.0f, 0.85f, 0.2f);
    glUniform3fv(base_color_loc, 1, &ballColor.x);
    glUniform1f(bar_t_loc, ball_t);

    bool crossing_seam = (ball_t > 0.85f);
    if (crossing_seam) glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLES, 0, sphere_vertices.size());
    if (crossing_seam) glEnable(GL_DEPTH_TEST);
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

    mat4 global_spin =
        RotateX(global_spin_angle) * RotateY(global_spin_angle * 1.3f) * RotateZ(global_spin_angle * 0.7f);
    mat4 viewProj = proj * view;

    vec3 lightPos(2.0f, 3.0f, 2.0f);
    glUniform3fv(light_pos_loc, 1, &lightPos.x);
    glUniform3fv(eye_pos_loc, 1, &eye.x);
    glUniform1i(num_segments_loc, num_segments);

    // --- Ball position along the loop ---
    ball_t += delta_time * 0.25f;
    ball_t = fmodf(ball_t, 1.0f);

    float thickness  = POLYGON_BAR_THICKNESS * scale_factor;
    float half_thick = thickness / 2.0f;
    float ball_angle = ball_t * 2.0f * M_PI;

    float base_z       = num_segments * zStep * (1.0f - ball_t);
    float ball_local_z = base_z + half_thick + BALL_RADIUS;

    float segment_angle = 2.0f * M_PI / num_segments;
    float nearest_bar   = roundf(ball_angle / segment_angle) * segment_angle;
    float delta_angle   = ball_angle - nearest_bar;
    float r_poly        = radius / cosf(delta_angle);

    vec3 ball_local_pos(-r_poly * sinf(ball_angle), r_poly * cosf(ball_angle), ball_local_z);

    // Cool slate-blue palette borrowed from slot 2 (Penrose Triangle). Three
    // shades per bar so top / bottom / tip-caps read distinctly — matches the
    // Paradox figures' multi-tone look instead of a flat single color.
    vec3 topColor (0.74f, 0.82f, 0.92f);   // top faces — light sky
    vec3 botColor (0.52f, 0.62f, 0.78f);   // bottom faces — mid slate
    vec3 tipColor (0.46f, 0.56f, 0.72f);   // end caps — darker slate

    // Helper: draw one bar's faces in 3 colored chunks (12 verts each:
    // 0..11 = top faces, 12..23 = bottom faces, 24..35 = tip caps).
    auto draw_bar_tritone = [&]() {
        glUniform3fv(base_color_loc, 1, &topColor.x); glDrawArrays(GL_TRIANGLES,  0, 12);
        glUniform3fv(base_color_loc, 1, &botColor.x); glDrawArrays(GL_TRIANGLES, 12, 12);
        glUniform3fv(base_color_loc, 1, &tipColor.x); glDrawArrays(GL_TRIANGLES, 24, 12);
    };

    float L = radius * tanf(M_PI / num_segments);

    // Bars 1 .. N-1 — full mitered segments, spiraling down in z.
    for (int bar_index = 1; bar_index < num_segments; bar_index++)
    {
        float angle  = bar_index * (360.0f / num_segments);
        float zDepth = -bar_index * zStep + (num_segments * zStep);
        float bar_t  = (float)bar_index / num_segments;

        // Teleport the lighting source across the seam so the illusion's z-jump
        // doesn't cast inconsistent shadows on the front/back bars.
        vec3 illusory_light_pos = ball_local_pos;
        float loop_length = num_segments * zStep;
        if (ball_t - bar_t > 0.5f)       illusory_light_pos.z += loop_length;
        else if (bar_t - ball_t > 0.5f)  illusory_light_pos.z -= loop_length;

        vec4 wbp = global_spin * vec4(illusory_light_pos.x, illusory_light_pos.y, illusory_light_pos.z, 1.0f);
        glUniform3fv(ball_pos_loc, 1, &wbp.x);

        mat4 model = RotateZ(angle) * Translate(0.0f, radius, zDepth);
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
    //   left half  : at z = N*zStep (top), joins bar 1 on its left mitered end
    //   right half : at z = 0       (bot), joins bar N-1 on its left mitered end
    //                                      (we rotate it 180° so its mitered
    //                                       end faces toward bar N-1)
    // The flat right-tip cut on each half sits at x=0, and in the default ortho
    // view both cuts project to the same vertical screen line → polygon closes.
    {
        float zDepth_top = num_segments * zStep;  // bar 0's nominal z
        float zDepth_bot = 0.0f;                  // jumped-down z for right half

        vec3 illusory_light_pos = ball_local_pos;
        vec4 wbp = global_spin * vec4(illusory_light_pos.x, illusory_light_pos.y, illusory_light_pos.z, 1.0f);
        glUniform3fv(ball_pos_loc, 1, &wbp.x);
        glUniform1i(is_ball_loc, 0);
        glUniform1f(bar_t_loc, 0.0f);

        glBindVertexArray(half_segment_vao);

        // Left half: shifted left by L/2 so its flat right-cut (local x=+L/2)
        // lands at world x=0. Mitered left tip lands at world x=-L-dx_inner,
        // exactly where a full bar 0's left mitered tip would be — joins the
        // polygon corner shared with bar 1.
        mat4 model_l = Translate(-L * 0.5f, radius, zDepth_top);
        mat4 mvp_l   = viewProj * global_spin * model_l;
        mat4 wmodel_l = global_spin * model_l;
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &mvp_l.d[0].x);
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, &wmodel_l.d[0].x);
        draw_bar_tritone();

        // Right half: same geometry flipped 180° on Z AND X so its mitered tip
        // points to the +X side (joining bar N-1's corner) AND its inner edge
        // (-y in local) stays on the polygon-interior side after the flip.
        // Without RotateX(180), RotateZ(180) alone would also flip y, putting
        // the inner edge outside the polygon. The combined flip is
        //   (x, y, z) → (-x, y, -z)
        // — inner/outer y preserved, ridge/bot z swapped (cosmetic, hex is
        // symmetric), x mirrored. Translated to (+L/2, radius, 0) so its flat
        // cut (originally at +L/2 → now at -L/2 → +0 after translate) meets
        // the left half's cut at world x=0, on the BOTTOM z plane.
        mat4 model_r = Translate(L * 0.5f, radius, zDepth_bot) * RotateZ(180.0f) * RotateX(180.0f);
        mat4 mvp_r    = viewProj * global_spin * model_r;
        mat4 wmodel_r = global_spin * model_r;
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &mvp_r.d[0].x);
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, &wmodel_r.d[0].x);
        draw_bar_tritone();
    }

    // Ball
    display_ball(viewProj, global_spin, ball_local_pos);

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

    float jt = next_signed() * 1.6f;   // ±0.8 rad on theta
    float jp = next_signed() * 1.0f;   // ±0.5 rad on phi

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

// "Solved" pose for the S-key — snap the camera back to the magic ortho
// angle where the depth-trick cuts align in 2D and the polygon reads as a
// single closed loop. Preserves the CURRENT N (so the player sees the
// random polygon they were given click into place at its own shape).
static void polygon_solve()
{
    camera_radius = 0.5f;
    camera_theta  = M_PI / 2.0f;
    camera_phi    = M_PI / 2.0f;
    global_spin_angle = 0.0f;
    spin_momentum     = 0.0f;
}

// =============================================
// Input
// =============================================
void polygon_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch (key)
    {
        case GLFW_KEY_SPACE:
            // SPACE: tap-to-grow. Adds an edge AND snaps the camera to the
            // new N's per-N unsolved pose — so each new polygon size starts
            // from its own fresh puzzle position. Hold also keeps the figure
            // spinning (momentum-based spin from before).
            if (action == GLFW_PRESS)
            {
                is_space_pressed = true;
                polygon_set_constants(num_segments + 1);
                polygon_apply_pose_for(num_segments);
            }
            else if (action == GLFW_RELEASE)
            {
                is_space_pressed = false;
            }
            break;
        case GLFW_KEY_A:
            // A: shrink by an edge — same per-N pose snap as SPACE.
            if (action == GLFW_PRESS)
            {
                if (num_segments > 3)
                {
                    polygon_set_constants(num_segments - 1);
                    polygon_apply_pose_for(num_segments);
                }
            }
            break;
        case GLFW_KEY_R:
            // R: snap back to THIS N's fixed unsolved pose (same one every
            // press for the same N — like defaultAngle* on slots 2-8).
            if (action == GLFW_PRESS) polygon_apply_pose_for(num_segments);
            break;
        case GLFW_KEY_S:
            // S: snap the camera back to the magic ortho angle. The depth
            // trick aligns, bar 0's two halves line up in screen-x, and the
            // polygon reads as a closed loop for the CURRENT random N.
            if (action == GLFW_PRESS) polygon_solve();
            break;
        case GLFW_KEY_ESCAPE:
        case GLFW_KEY_Q:
            exit(EXIT_SUCCESS);
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
    if (is_dragging)
    {
        double deltaX = xpos - last_mouse_x;
        double deltaY = ypos - last_mouse_y;

        last_mouse_x = xpos;
        last_mouse_y = ypos;

        camera_theta -= deltaX * 0.01f;
        camera_phi   += deltaY * 0.01f;

        if (camera_phi < 0.01f)         camera_phi = 0.01f;
        if (camera_phi > M_PI - 0.01f)  camera_phi = M_PI - 0.01f;
    }
}
