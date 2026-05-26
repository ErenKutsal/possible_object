#include "includes.h"

int num_segments = 3;

float scale_factor = num_segments / 3.0f;
float radius = 0.5f * scale_factor;
float zStep = 0.15f * scale_factor;

const int num_vertices = 36;
vec3 segment_vertices[num_vertices];

vec3 segment_normals[num_vertices];
GLuint segment_nbo;
GLint light_pos_loc, eye_pos_loc, model_loc, bar_t_loc, base_color_loc;
GLint num_segments_loc;

GLuint segment_vao, segment_vbo;
GLuint program;
GLint mvp_loc;
GLint color_loc;

float ball_t = 0.0f;  // 0.0 to 1.0, position along the loop

const int SPHERE_STACKS = 10;
const int SPHERE_SLICES = 10;
const float BALL_RADIUS = 0.04f;
std::vector<vec3> sphere_vertices;

GLuint sphere_vao, sphere_vbo;
GLuint sphere_nbo_id;
std::vector<vec3> sphere_normals;
GLuint ball_pos_loc;
GLuint is_ball_loc;

float camera_radius = 0.5f;
float camera_theta = M_PI / 2.0f;
float camera_phi = M_PI / 2.0f;

bool is_dragging = false;
double last_mouse_x = 0.0;
double last_mouse_y = 0.0;

bool is_space_pressed = false;
float spin_momentum = 0.0f;
float global_spin_angle = 0.0f;
float last_frame_time = 0.0f;

// =============================================
// Impossible Polygon geometry
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

            // Each cell = 2 triangles
            vec3 v[4] = {
                vec3(sinf(phi1) * cosf(theta1), cosf(phi1), sinf(phi1) * sinf(theta1)),
                vec3(sinf(phi2) * cosf(theta1), cosf(phi2), sinf(phi2) * sinf(theta1)),
                vec3(sinf(phi2) * cosf(theta2), cosf(phi2), sinf(phi2) * sinf(theta2)),
                vec3(sinf(phi1) * cosf(theta2), cosf(phi1), sinf(phi1) * sinf(theta2)),
            };

            // Triangle 1
            sphere_vertices.push_back(v[0]);
            sphere_normals.push_back(v[0]);
            sphere_vertices.push_back(v[1]);
            sphere_normals.push_back(v[1]);
            sphere_vertices.push_back(v[2]);
            sphere_normals.push_back(v[2]);
            // Triangle 2
            sphere_vertices.push_back(v[0]);
            sphere_normals.push_back(v[0]);
            sphere_vertices.push_back(v[2]);
            sphere_normals.push_back(v[2]);
            sphere_vertices.push_back(v[3]);
            sphere_normals.push_back(v[3]);
        }
    }
}

void set_face_normal(vec3* normals, int start_idx, vec3 a, vec3 b, vec3 c)
{
    vec3 normal = normalize(cross(b - a, c - a));
    normals[start_idx] = normal;
    normals[start_idx + 1] = normal;
    normals[start_idx + 2] = normal;
}

void polygon_create_solid_segment(int n_segments, float radius, float thickness)
{
    float half_thick = thickness / 2.0f;
    float ridge_z = half_thick;
    float L = radius * tanf(M_PI / n_segments);

    float dx_inner = (half_thick / sinf(2 * M_PI / n_segments)) - (half_thick * tanf(M_PI / n_segments));
    float dx_center = half_thick / sinf(2 * M_PI / n_segments);
    float dx_outer = (half_thick / sinf(2 * M_PI / n_segments)) - (half_thick / tanf(2 * M_PI / n_segments));

    // 1. Inner Edge
    vec3 v_in_L = vec3(-L - dx_inner, -half_thick, 0.0f);
    vec3 v_in_R = vec3(L + dx_inner, -half_thick, 0.0f);

    // 2. Center Ridges
    vec3 v_rid_L = vec3(-L - dx_center, 0.0f, ridge_z);
    vec3 v_rid_R = vec3(L + dx_center, 0.0f, ridge_z);
    vec3 v_bot_L = vec3(-L - dx_center, 0.0f, -ridge_z);
    vec3 v_bot_R = vec3(L + dx_center, 0.0f, -ridge_z);

    // 3. Outer Edge
    vec3 v_out_L = vec3(-L - dx_outer, half_thick, 0.0f);
    vec3 v_out_R = vec3(L + dx_outer, half_thick, 0.0f);

    int idx = 0;
    // 1. Top Inner Face (Vertices 0 to 5)
    segment_vertices[idx++] = v_in_L;
    segment_vertices[idx++] = v_in_R;
    segment_vertices[idx++] = v_rid_R;
    segment_vertices[idx++] = v_in_L;
    segment_vertices[idx++] = v_rid_R;
    segment_vertices[idx++] = v_rid_L;

    // 2. Top Outer Face (Vertices 6 to 11)
    segment_vertices[idx++] = v_rid_L;
    segment_vertices[idx++] = v_rid_R;
    segment_vertices[idx++] = v_out_R;
    segment_vertices[idx++] = v_rid_L;
    segment_vertices[idx++] = v_out_R;
    segment_vertices[idx++] = v_out_L;

    // 3. Bottom Outer Face (Vertices 12 to 17)
    segment_vertices[idx++] = v_out_L;
    segment_vertices[idx++] = v_out_R;
    segment_vertices[idx++] = v_bot_R;
    segment_vertices[idx++] = v_out_L;
    segment_vertices[idx++] = v_bot_R;
    segment_vertices[idx++] = v_bot_L;

    // 4. Bottom Inner Face (Vertices 18 to 23)
    segment_vertices[idx++] = v_bot_L;
    segment_vertices[idx++] = v_bot_R;
    segment_vertices[idx++] = v_in_R;
    segment_vertices[idx++] = v_bot_L;
    segment_vertices[idx++] = v_in_R;
    segment_vertices[idx++] = v_in_L;

    // 5. Left Tip (Vertices 24 to 29)
    segment_vertices[idx++] = v_in_L;
    segment_vertices[idx++] = v_rid_L;
    segment_vertices[idx++] = v_bot_L;
    segment_vertices[idx++] = v_rid_L;
    segment_vertices[idx++] = v_bot_L;
    segment_vertices[idx++] = v_out_L;

    // 6. Right Tip (Vertices 30 to 35)
    segment_vertices[idx++] = v_in_R;
    segment_vertices[idx++] = v_rid_R;
    segment_vertices[idx++] = v_bot_R;
    segment_vertices[idx++] = v_rid_R;
    segment_vertices[idx++] = v_bot_R;
    segment_vertices[idx++] = v_out_R;

    // Set Face Normals
    set_face_normal(segment_normals, 0, v_in_L, v_in_R, v_rid_R);
    set_face_normal(segment_normals, 3, v_in_L, v_rid_R, v_rid_L);
    set_face_normal(segment_normals, 6, v_rid_L, v_rid_R, v_out_R);
    set_face_normal(segment_normals, 9, v_rid_L, v_out_R, v_out_L);
    set_face_normal(segment_normals, 12, v_out_L, v_out_R, v_bot_R);
    set_face_normal(segment_normals, 15, v_out_L, v_bot_R, v_bot_L);
    set_face_normal(segment_normals, 18, v_bot_L, v_bot_R, v_in_R);
    set_face_normal(segment_normals, 21, v_bot_L, v_in_R, v_in_L);
    set_face_normal(segment_normals, 24, v_in_L, v_rid_L, v_bot_L);
    set_face_normal(segment_normals, 27, v_rid_L, v_bot_L, v_out_L);
    set_face_normal(segment_normals, 30, v_in_R, v_rid_R, v_bot_R);
    set_face_normal(segment_normals, 33, v_rid_R, v_bot_R, v_out_R);
}

void polygon_init()
{
    polygon_create_solid_segment(num_segments, radius, 0.22f * scale_factor);

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

    GLint loc = glGetAttribLocation(program, "vPosition");
    GLint normal_loc = glGetAttribLocation(program, "vNormal");
    color_loc = glGetUniformLocation(program, "uFaceColor");
    mvp_loc = glGetUniformLocation(program, "MVP");
    light_pos_loc = glGetUniformLocation(program, "uLightPos");
    eye_pos_loc = glGetUniformLocation(program, "uEyePos");
    model_loc = glGetUniformLocation(program, "uModel");
    bar_t_loc = glGetUniformLocation(program, "uBarT");
    base_color_loc = glGetUniformLocation(program, "uBaseColor");
    num_segments_loc = glGetUniformLocation(program, "uNumSegments");
    ball_pos_loc = glGetUniformLocation(program, "uBallPos");
    is_ball_loc = glGetUniformLocation(program, "uIsBall");

    // Bind vao&vbo for segment
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
}

vec3 bar_top_position(int i)
{
    float angle_rad = i * (2.0f * M_PI / num_segments);
    float zDepth = -i * zStep + (num_segments * zStep);
    float thickness = 0.22f * scale_factor;
    float half_thick = thickness / 2.0f;

    // Outward direction from polygon center for this bar
    float out_x = -sinf(angle_rad);
    float out_y = cosf(angle_rad);

    // Place ball on the outer face, offset by BALL_RADIUS so it sits on surface
    float r = radius + half_thick + BALL_RADIUS;
    return vec3(r * out_x, r * out_y, zDepth);
}

// Update the function signature to accept the local_pos
void display_ball(mat4 viewProj, mat4 global_spin, vec3 local_pos)
{
    // Apply global spin to the physical position so the ball draws in the correct spot
    vec4 wbp = global_spin * vec4(local_pos.x, local_pos.y, local_pos.z, 1.0f);
    vec3 world_ball_pos(wbp.x, wbp.y, wbp.z);

    // Send the real position to the shader just for the ball itself
    glUniform3fv(ball_pos_loc, 1, &world_ball_pos.x);

    mat4 ball_translate =
        Translate(local_pos.x, local_pos.y, local_pos.z) * Scale(BALL_RADIUS, BALL_RADIUS, BALL_RADIUS);
    mat4 ball_mvp = viewProj * global_spin * ball_translate;
    mat4 ball_model = global_spin * ball_translate;

    glBindVertexArray(sphere_vao);
    glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &ball_mvp.d[0].x);
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, &ball_model.d[0].x);

    glUniform1i(is_ball_loc, 1);
    vec3 ballColor(1.0f, 0.85f, 0.2f);
    glUniform3fv(base_color_loc, 1, &ballColor.x);
    glUniform1f(bar_t_loc, ball_t);

    // The Impossible Seam Hack
    bool crossing_seam = (ball_t > 0.85f);
    if (crossing_seam) glDisable(GL_DEPTH_TEST);

    glDrawArrays(GL_TRIANGLES, 0, sphere_vertices.size());

    if (crossing_seam) glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

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
    mat4 proj = Ortho(-view_size, view_size, -view_size, view_size, -1000.0f, 1000.0f);

    float current_time = glfwGetTime();

    glUseProgram(program);

    // --- SPIN MATH ---
    if (last_frame_time == 0.0f) last_frame_time = current_time;  // Safety for frame 1
    float delta_time = current_time - last_frame_time;
    last_frame_time = current_time;

    // 1. Engine Throttle & Friction
    if (is_space_pressed)
    {
        spin_momentum += 0.4f * delta_time;  // Takes 2.5 seconds to reach max speed
        if (spin_momentum > 1.0f) spin_momentum = 1.0f;
    }
    else
    {
        spin_momentum -= 0.6f * delta_time;  // Brakes slightly faster than it accelerates
        if (spin_momentum < 0.0f) spin_momentum = 0.0f;
    }

    // 2. Calculate the heavy exponential speed curve
    float spin_speed = (spin_momentum * spin_momentum * spin_momentum) * 2000.0f;

    // 3. Accumulate the actual rotation angle!
    global_spin_angle += spin_speed * delta_time;

    mat4 global_spin =
        RotateX(global_spin_angle) * RotateY(global_spin_angle * 1.3f) * RotateZ(global_spin_angle * 0.7f);
    mat4 viewProj = proj * view;

    vec3 lightPos(2.0f, 3.0f, 2.0f);  // world-space light
    glUniform3fv(light_pos_loc, 1, &lightPos.x);
    glUniform3fv(eye_pos_loc, 1, &eye.x);  // eye is already computed above
    glUniform1i(num_segments_loc, num_segments);

    // --- 1. UPDATE BALL POSITION ONCE PER FRAME HERE ---
    ball_t += delta_time * 0.25f;
    ball_t = fmodf(ball_t, 1.0f);

    float thickness = 0.22f * scale_factor;
    float half_thick = thickness / 2.0f;
    float ball_angle = ball_t * 2.0f * M_PI;

    float base_z = num_segments * zStep * (1.0f - ball_t);
    float ball_local_z = base_z + half_thick + BALL_RADIUS;

    float segment_angle = 2.0f * M_PI / num_segments;
    float nearest_bar = roundf(ball_angle / segment_angle) * segment_angle;
    float delta_angle = ball_angle - nearest_bar;
    float r_poly = radius / cosf(delta_angle);

    // This is the true, physical position of the ball
    vec3 ball_local_pos(-r_poly * sinf(ball_angle), r_poly * cosf(ball_angle), ball_local_z);

    for (int bar_index = 0; bar_index < num_segments; bar_index++)
    {
        float angle = bar_index * (360.0f / num_segments);
        float zDepth = -bar_index * zStep + (num_segments * zStep);
        float bar_t = (float)bar_index / num_segments;

        // --- 2. TELEPORT THE LIGHT IF CROSSING THE SEAM ---
        vec3 illusory_light_pos = ball_local_pos;
        float loop_length = num_segments * zStep;

        // If ball is physically at the back (>0.5), but we are drawing the front bars
        if (ball_t - bar_t > 0.5f)
        {
            illusory_light_pos.z += loop_length;  // Teleport the light to the front!
        }
        // If ball is physically at the front, but we are drawing the back bars
        else if (bar_t - ball_t > 0.5f)
        {
            illusory_light_pos.z -= loop_length;  // Teleport the light to the back!
        }

        // Apply global spin to the teleported light and send to shader
        vec4 wbp = global_spin * vec4(illusory_light_pos.x, illusory_light_pos.y, illusory_light_pos.z, 1.0f);
        glUniform3fv(ball_pos_loc, 1, &wbp.x);
        // --------------------------------------------------

        vec3 barColor(0.65f, 0.67f, 0.90f);
        glUniform3fv(base_color_loc, 1, &barColor.x);

        mat4 model = RotateZ(angle) * Translate(0.0f, radius, zDepth);
        mat4 mvp = viewProj * global_spin * model;

        mat4 world_model = global_spin * model;

        glBindVertexArray(segment_vao);

        glUniform1i(is_ball_loc, 0);

        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &mvp.d[0].x);
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, &world_model.d[0].x);
        glUniform1f(bar_t_loc, bar_t);
        glDrawArrays(GL_TRIANGLES, 0, num_vertices);
    }

    // Ball
    display_ball(viewProj, global_spin, ball_local_pos);

    glFinish();
}

void polygon_set_constants(int n_segments)
{
    num_segments = n_segments;

    scale_factor = n_segments / 3.0f;
    radius = 0.5f * scale_factor;
    zStep = 0.15f * scale_factor;

    camera_radius = 0.5f;
    camera_theta = M_PI / 2.0f;
    camera_phi = M_PI / 2.0f;

    is_dragging = false;
    last_mouse_x = 0.0;
    last_mouse_y = 0.0;

    global_spin_angle = 0.0;

    polygon_create_solid_segment(n_segments, radius, 0.22f * scale_factor);

    generate_sphere();

    glBindBuffer(GL_ARRAY_BUFFER, segment_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(segment_vertices), segment_vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, segment_nbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(segment_normals), segment_normals, GL_DYNAMIC_DRAW);

    glUniform1i(num_segments_loc, num_segments);
}

void polygon_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch (key)
    {
        case GLFW_KEY_SPACE:
            if (action == GLFW_PRESS)
            {
                is_space_pressed = true;
            }
            else if (action == GLFW_RELEASE)
            {
                is_space_pressed = false;
            }
            break;
        case GLFW_KEY_D:
            if (action == GLFW_PRESS)
            {
                polygon_set_constants(num_segments + 1);
            }
            break;
        case GLFW_KEY_A:
            if (action == GLFW_PRESS)
            {
                polygon_set_constants(num_segments - 1);
            }
            break;
        case GLFW_KEY_R:
            polygon_set_constants(num_segments);
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
        camera_phi += deltaY * 0.01f;

        if (camera_phi < 0.01f) camera_phi = 0.01f;
        if (camera_phi > M_PI - 0.01f) camera_phi = M_PI - 0.01f;
    }
}