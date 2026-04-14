#include "includes.h"

int num_segments = 3;

float scale_factor = num_segments / 3.0f;
float radius = 0.5f * scale_factor;
float zStep = 0.15f * scale_factor;

const int num_vertices = 36;
vec3 segment_vertices[num_vertices];
vec3 half_segment_vertices[num_vertices];

GLuint segment_vao, segment_vbo;
GLuint half_segment_vao, half_segment_vbo;
GLuint program;
GLint mvp_loc;
GLuint color_loc;

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

    //////////////////////////////////
    // Same things for half segment //
    //////////////////////////////////

    L = L / 2;

    // 1. Inner Edge
    vec3 h_v_in_L = vec3(-L - dx_inner, -half_thick, 0.0f);
    vec3 h_v_in_R = vec3(L, -half_thick, 0.0f);

    // 2. Center Ridges
    vec3 h_v_rid_L = vec3(-L - dx_center, 0.0f, ridge_z);
    vec3 h_v_rid_R = vec3(L, 0.0f, ridge_z);
    vec3 h_v_bot_L = vec3(-L - dx_center, 0.0f, -ridge_z);
    vec3 h_v_bot_R = vec3(L, 0.0f, -ridge_z);

    // 3. Outer Edge
    vec3 h_v_out_L = vec3(-L - dx_outer, half_thick, 0.0f);
    vec3 h_v_out_R = vec3(L, half_thick, 0.0f);

    idx = 0;
    // 1. Top Inner Face (Vertices 0 to 5)
    half_segment_vertices[idx++] = h_v_in_L;
    half_segment_vertices[idx++] = h_v_in_R;
    half_segment_vertices[idx++] = h_v_rid_R;
    half_segment_vertices[idx++] = h_v_in_L;
    half_segment_vertices[idx++] = h_v_rid_R;
    half_segment_vertices[idx++] = h_v_rid_L;

    // 2. Top Outer Face (Vertices 6 to 11)
    half_segment_vertices[idx++] = h_v_rid_L;
    half_segment_vertices[idx++] = h_v_rid_R;
    half_segment_vertices[idx++] = h_v_out_R;
    half_segment_vertices[idx++] = h_v_rid_L;
    half_segment_vertices[idx++] = h_v_out_R;
    half_segment_vertices[idx++] = h_v_out_L;

    // 3. Bottom Outer Face (Vertices 12 to 17)
    half_segment_vertices[idx++] = h_v_out_L;
    half_segment_vertices[idx++] = h_v_out_R;
    half_segment_vertices[idx++] = h_v_bot_R;
    half_segment_vertices[idx++] = h_v_out_L;
    half_segment_vertices[idx++] = h_v_bot_R;
    half_segment_vertices[idx++] = h_v_bot_L;

    // 4. Bottom Inner Face (Vertices 18 to 23)
    half_segment_vertices[idx++] = h_v_bot_L;
    half_segment_vertices[idx++] = h_v_bot_R;
    half_segment_vertices[idx++] = h_v_in_R;
    half_segment_vertices[idx++] = h_v_bot_L;
    half_segment_vertices[idx++] = h_v_in_R;
    half_segment_vertices[idx++] = h_v_in_L;

    // 5. Left Tip (Vertices 24 to 29)
    half_segment_vertices[idx++] = h_v_in_L;
    half_segment_vertices[idx++] = h_v_rid_L;
    half_segment_vertices[idx++] = h_v_bot_L;
    half_segment_vertices[idx++] = h_v_rid_L;
    half_segment_vertices[idx++] = h_v_bot_L;
    half_segment_vertices[idx++] = h_v_out_L;

    // 6. Right Tip (Vertices 30 to 35)
    half_segment_vertices[idx++] = h_v_in_R;
    half_segment_vertices[idx++] = h_v_rid_R;
    half_segment_vertices[idx++] = h_v_bot_R;
    half_segment_vertices[idx++] = h_v_rid_R;
    half_segment_vertices[idx++] = h_v_bot_R;
    half_segment_vertices[idx++] = h_v_out_R;
}

void polygon_init()
{
    polygon_create_solid_segment(num_segments, radius, 0.15f * log(num_segments));

    program = InitShader("../shaders/vshader.glsl", "../shaders/fshader.glsl");
    glUseProgram(program);

    GLuint loc = glGetAttribLocation(program, "vPosition");
    color_loc = glGetUniformLocation(program, "uFaceColor");
    mvp_loc = glGetUniformLocation(program, "MVP");

    // Bind vao&vbo for segment
    glGenVertexArrays(1, &segment_vao);
    glBindVertexArray(segment_vao);

    glGenBuffers(1, &segment_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, segment_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(segment_vertices), segment_vertices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    // Bind vao&vbo for half segment
    glGenVertexArrays(1, &half_segment_vao);
    glBindVertexArray(half_segment_vao);

    glGenBuffers(1, &half_segment_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, half_segment_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(half_segment_vertices), half_segment_vertices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glBindVertexArray(0);
}

void polygon_display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);

    float eye_x = camera_radius * sinf(camera_phi) * cosf(camera_theta);
    float eye_y = camera_radius * cosf(camera_phi);
    float eye_z = camera_radius * sinf(camera_phi) * sinf(camera_theta);

    vec3 eye(eye_x, eye_y, eye_z);
    vec3 at(0.0f, 0.0f, 0.0f);
    vec3 up(0.0f, 1.0f, 0.0f);
    mat4 view = LookAt(eye, at, up);

    float view_size = scale_factor + 0.5f;
    mat4 proj = Ortho(-view_size, view_size, -view_size, view_size, -1000.0f, 1000.0f);

    // --- SPIN MATH ---
    float current_time = glfwGetTime();
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
    mat4 viewProj = proj * view * global_spin;

    vec3 light(0.73f, 0.58f, 0.62f);        // pink
    vec3 med(0.45f, 0.47f, 0.65f);          // blue medium
    vec3 dark(0.35f, 0.38f, 0.58f);         // blue dark
    vec3 bottomColor(0.28f, 0.30f, 0.45f);  // dark bottom

    vec3 colors[3] = {dark, med, light};
    int coloring_index = 0;

    for (int bar_index = 0; bar_index < num_segments; bar_index++)
    {
        float angle = bar_index * (360.0f / num_segments);
        float zDepth = -bar_index * zStep + (num_segments * zStep);

        if (bar_index == 0)
        {
            float L = radius * tanf(M_PI / num_segments);

            mat4 model_l = Translate(-L / 2, radius, zDepth);
            mat4 model_r = Translate(L / 2, radius, zDepth - (num_segments * zStep)) * RotateZ(180) * RotateX(180);
            mat4 mvp_l = viewProj * model_l;
            mat4 mvp_r = viewProj * model_r;

            glBindVertexArray(half_segment_vao);

            // Draw left half
            glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &mvp_l.d[0].x);

            glUniform3fv(color_loc, 1, &colors[(coloring_index) % 3].x);
            glDrawArrays(GL_TRIANGLES, 6, 6);

            coloring_index++;

            glUniform3fv(color_loc, 1, &colors[coloring_index % 3].x);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glUniform3fv(color_loc, 1, &bottomColor.x);
            glDrawArrays(GL_TRIANGLES, 12, 12);

            glUniform3fv(color_loc, 1, &bottomColor.x);
            glDrawArrays(GL_TRIANGLES, 24, 12);

            coloring_index--;

            // Draw right half
            glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &mvp_r.d[0].x);

            glUniform3fv(color_loc, 1, &bottomColor.x);
            glDrawArrays(GL_TRIANGLES, 0, 12);

            glUniform3fv(color_loc, 1, &colors[coloring_index % 3].x);
            glDrawArrays(GL_TRIANGLES, 12, 6);

            coloring_index++;

            glUniform3fv(color_loc, 1, &colors[coloring_index % 3].x);
            glDrawArrays(GL_TRIANGLES, 18, 6);

            glUniform3fv(color_loc, 1, &bottomColor.x);
            glDrawArrays(GL_TRIANGLES, 24, 12);
        }
        else
        {
            mat4 model = RotateZ(angle) * Translate(0.0f, radius, zDepth);
            mat4 mvp = viewProj * model;

            glBindVertexArray(segment_vao);

            glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &mvp.d[0].x);

            glUniform3fv(color_loc, 1, &colors[(coloring_index) % 3].x);
            glDrawArrays(GL_TRIANGLES, 6, 6);

            coloring_index++;

            glUniform3fv(color_loc, 1, &colors[coloring_index % 3].x);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glUniform3fv(color_loc, 1, &bottomColor.x);
            glDrawArrays(GL_TRIANGLES, 12, 12);

            glUniform3fv(color_loc, 1, &bottomColor.x);
            glDrawArrays(GL_TRIANGLES, 24, 12);
        }
    }

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

    polygon_create_solid_segment(n_segments, radius, 0.15f * log(num_segments));

    glBindBuffer(GL_ARRAY_BUFFER, segment_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(segment_vertices), segment_vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, half_segment_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(half_segment_vertices), half_segment_vertices, GL_DYNAMIC_DRAW);
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

        camera_theta += deltaX * 0.01f;
        camera_phi += deltaY * 0.01f;

        if (camera_phi < 0.01f) camera_phi = 0.01f;
        if (camera_phi > M_PI - 0.01f) camera_phi = M_PI - 0.01f;
    }
}