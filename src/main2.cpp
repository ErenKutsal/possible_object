#include "includes.h"

const int num_segments = 6;

float scale_factor = num_segments / 6.0f;  // How much bigger the shape needs to be compared to the 6-bar original
float radius = 0.5f * scale_factor;
float zStep = 0.15f * scale_factor;

const int num_vertices = 24;
vec3 vertices[num_vertices];

GLuint vao, vbo;
GLuint program;
GLint modelViewProjLoc;
GLuint colorLoc;

float camera_radius = 0.5f;
float camera_theta = M_PI / 2.0f;
float camera_phi = M_PI / 2.0f;

bool isDragging = false;
double lastMouseX = 0.0;
double lastMouseY = 0.0;

void create_solid_segment(float radius, float thickness)
{
    float half_thick = thickness / 2.0f;
    float ridge_z = half_thick;
    float L = radius * tanf(M_PI / num_segments);

    // I really cooked some math here. I am not sure i can do it again.
    // Naming is not good. Inner has lower y coordinate, outer has higher.
    float dx_inner = (half_thick / sinf(2 * M_PI / num_segments)) - (half_thick * tanf(M_PI / num_segments));
    float dx_center = half_thick / sinf(2 * M_PI / num_segments);
    float dx_outer = (half_thick / sinf(2 * M_PI / num_segments)) - (half_thick / tanf(2 * M_PI / num_segments));

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
    vertices[idx++] = v_in_L;
    vertices[idx++] = v_in_R;
    vertices[idx++] = v_rid_R;
    vertices[idx++] = v_in_L;
    vertices[idx++] = v_rid_R;
    vertices[idx++] = v_rid_L;

    // 2. Top Outer Face (Vertices 6 to 11)
    vertices[idx++] = v_rid_L;
    vertices[idx++] = v_rid_R;
    vertices[idx++] = v_out_R;
    vertices[idx++] = v_rid_L;
    vertices[idx++] = v_out_R;
    vertices[idx++] = v_out_L;

    // 3. Bottom Outer Face (Vertices 12 to 17)
    vertices[idx++] = v_out_L;
    vertices[idx++] = v_out_R;
    vertices[idx++] = v_bot_R;
    vertices[idx++] = v_out_L;
    vertices[idx++] = v_bot_R;
    vertices[idx++] = v_bot_L;

    // 4. Bottom Inner Face (Vertices 18 to 23)
    vertices[idx++] = v_bot_L;
    vertices[idx++] = v_bot_R;
    vertices[idx++] = v_in_R;
    vertices[idx++] = v_bot_L;
    vertices[idx++] = v_in_R;
    vertices[idx++] = v_in_L;
}

void create_hexagon() {}

void init()
{
    create_solid_segment(radius, 0.15f * log(num_segments));

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    program = InitShader("../shaders/vshader.glsl", "../shaders/fshader.glsl");
    glUseProgram(program);

    GLuint loc = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    colorLoc = glGetUniformLocation(program, "uFaceColor");

    modelViewProjLoc = glGetUniformLocation(program, "MVP");

    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);  // Dark gray background

    glEnable(GL_DEPTH_TEST);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float eye_x = camera_radius * sinf(camera_phi) * cosf(camera_theta);
    float eye_y = camera_radius * cosf(camera_phi);
    float eye_z = camera_radius * sinf(camera_phi) * sinf(camera_theta);

    vec3 eye(eye_x, eye_y, eye_z);
    vec3 at(0.0f, 0.0f, 0.0f);
    vec3 up(0.0f, 1.0f, 0.0f);
    mat4 view = LookAt(eye, at, up);

    float view_size = scale_factor + 0.5f;
    mat4 proj = Ortho(-view_size, view_size, -view_size, view_size, -100.0f, 100.0f);

    mat4 viewProj = proj * view;

    vec3 light(0.7f, 0.7f, 0.7f);
    vec3 med(0.4f, 0.4f, 0.4f);
    vec3 dark(0.1f, 0.1f, 0.1f);
    vec3 bottomColor(0.15f, 0.15f, 0.15f);

    vec3 colors[3] = {dark, med, light};
    int coloring_index = 0;

    int draw_order[num_segments];
    for (int i = 0; i < num_segments; i++)
    {
        draw_order[i] = i;
    }

    for (int i = 0; i < num_segments; i++)
    {
        int barIndex = draw_order[i];

        float angle = barIndex * (360 / num_segments);  // index * 2pi/n in degrees
        float zDepth = -barIndex * zStep +
                       (num_segments * zStep);  // The thing in parantheses makes sure the object is centered at z = 0.

        mat4 model = RotateZ(angle) * Translate(0.0f, radius, zDepth);
        mat4 mvp = viewProj * model;

        glUniformMatrix4fv(modelViewProjLoc, 1, GL_FALSE, &mvp.d[0][0]);

        // Draw the Top Outer Face (Vertices 6-11)
        glUniform3fv(colorLoc, 1, &colors[(coloring_index) % 3].x);
        glDrawArrays(GL_TRIANGLES, 6, 6);

        coloring_index++;

        // Draw the Top Inner Face (Vertices 0-5)
        glUniform3fv(colorLoc, 1, &colors[coloring_index % 3].x);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // NEW: Draw the Bottom Volume Faces (Vertices 12-23)
        glUniform3fv(colorLoc, 1, &bottomColor.x);
        glDrawArrays(GL_TRIANGLES, 12, 12);
    }

    glFinish();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch (key)
    {
        case GLFW_KEY_ESCAPE:
        case GLFW_KEY_Q:
            break;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            isDragging = true;
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        }
        else if (action == GLFW_RELEASE)
        {
            isDragging = false;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (isDragging)
    {
        double deltaX = xpos - lastMouseX;
        double deltaY = ypos - lastMouseY;

        lastMouseX = xpos;
        lastMouseY = ypos;

        camera_theta -= deltaX * 0.01f;
        camera_phi += deltaY * 0.01f;

        // Clamp the up/down angle so the camera doesn't flip upside down
        if (camera_phi < 0.01f) camera_phi = 0.01f;
        if (camera_phi > M_PI - 0.01f) camera_phi = M_PI - 0.01f;
    }
}

void update(void) {}

int main()
{
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(512, 512, "possible", NULL, NULL);
    glfwMakeContextCurrent(window);

    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    init();

    double frameRate = 30, currentTime, previousTime = 0.0;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        currentTime = glfwGetTime();
        if (currentTime - previousTime >= 1 / frameRate)
        {
            previousTime = currentTime;
            update();
        }

        display();
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
}
