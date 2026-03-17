#include "InitShader.h"
#include "my_math.h"

const int num_vertices = 36;
vec3 vertices[num_vertices];

vec3 cube_vertices[8] = {vec3(-0.5, -0.5, 0.5),  vec3(-0.5, 0.5, 0.5),
                         vec3(0.5, 0.5, 0.5),    vec3(0.5, -0.5, 0.5),
                         vec3(-0.5, -0.5, -0.5), vec3(-0.5, 0.5, -0.5),
                         vec3(0.5, 0.5, -0.5),   vec3(0.5, -0.5, -0.5)};

GLuint vao, vbo;
GLuint program;
GLint modelViewProjLoc;

float camera_radius = 0.5f;
float camera_theta = M_PI / 2.0f;
float camera_phi = M_PI / 2.0f;

bool isDragging = false;
double lastMouseX = 0.0;
double lastMouseY = 0.0;

int Index = 0;

void quad(int a, int b, int c, int d)
{
    vertices[Index] = cube_vertices[a];
    Index++;
    vertices[Index] = cube_vertices[b];
    Index++;
    vertices[Index] = cube_vertices[c];
    Index++;
    vertices[Index] = cube_vertices[a];
    Index++;
    vertices[Index] = cube_vertices[c];
    Index++;
    vertices[Index] = cube_vertices[d];
    Index++;
}

void cube()
{
    quad(1, 0, 3, 2);
    quad(2, 3, 7, 6);
    quad(3, 0, 4, 7);
    quad(6, 5, 1, 2);
    quad(4, 5, 6, 7);
    quad(5, 4, 0, 1);
}

void create_hexagon() {}

void init()
{
    cube();
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

    glEnable(GL_DEPTH_TEST);
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

    mat4 proj = Ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

    mat4 viewProj = proj * view;

    float radius = 0.4f;
    float barLength = 0.5f;
    float thickness = 0.05f;
    float zStep = 0.08f;

    int drawOrder[6] = {1, 2, 3, 4, 5, 0};

    for (int i = 0; i < 6; i++)
    {
        int barIndex = drawOrder[i];

        if (barIndex == 0)
        {
            glClear(GL_DEPTH_BUFFER_BIT);
        }

        float zHeight = barIndex * zStep - (3.0f * zStep);
        float angle = barIndex * 60.0f;

        mat4 model = RotateZ(angle) * Translate(0.0f, radius, zHeight) *
                     Scale(barLength, thickness, thickness);

        mat4 mvp = viewProj * model;

        // Send to the shader
        glUniformMatrix4fv(modelViewProjLoc, 1, GL_FALSE, &mvp.d[0][0]);

        // Draw the standard cube (Assumes you have bound a VAO with a 36-vertex
        // generic cube)
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

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
