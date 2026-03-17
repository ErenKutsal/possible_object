#include "InitShader.h"
#include "my_math.h"

const int num_vertices = 7;
vec3 vertices[num_vertices];

GLuint vao, vbo;
GLuint program;
GLint modelViewProjLoc;

float camera_radius = 0.5f;
float camera_theta = M_PI / 2.0f;
float camera_phi = M_PI / 2.0f;

bool isDragging = false;
double lastMouseX = 0.0;
double lastMouseY = 0.0;

void create_hexagon()
{
    float pi_over_3 = M_PI / 3;
    float radius = 0.5f;
    float height_step = 0.1f;

    for (int i = 0; i < num_vertices; i++)
    {
        vertices[i] =
            vec3(radius * cosf(i * pi_over_3), radius * sinf(i * pi_over_3),
                 i * height_step - 0.25f);  // To make its center at z = 0
    }
}

void init()
{
    create_hexagon();

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

    modelViewProjLoc = glGetUniformLocation(program, "MVP");

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);  // Dark gray background
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

    mat4 proj = Ortho(-1, 1, -1, 1, -1, 1);

    mat4 model;

    mat4 mvp = proj * view * model;

    // Send the matrix to the shader
    glUniformMatrix4fv(modelViewProjLoc, 1, GL_FALSE, &mvp.d[0][0]);

    // Draw the helix as a line strip
    glBindVertexArray(vao);
    glDrawArrays(GL_LINE_STRIP, 0, num_vertices);

    // Draw points so you can see the vertices clearly
    glPointSize(5.0f);
    glDrawArrays(GL_POINTS, 0, num_vertices);

    glFinish();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mods)
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
        glfwPollEvents();  // Handles queued events
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
