#include "impossible_polygon.h"
#include "includes.h"
#include "neckercube.h"
#include "penrose.h"

int screen_w, screen_h;  // Screen Attributes

const int NUM_OBJECTS = 5;
const char* object_names[NUM_OBJECTS] = {"Impossible Polygon", "Penrose Triangle", "Impossible Cube", "Penrose Blocks",
                                         "Impossible Arch"};

// =============================================
// Which object to show: 0 = impossible polygon, 1 = penrose triangle
// =============================================
int current_object = 0;

// =============================================
// Unified callbacks - dispatch based on current_object
// =============================================
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Tab cycles, number keys jump directly
    if (action == GLFW_PRESS)
    {
        int target = -1;
        if (key == GLFW_KEY_TAB)
            target = (current_object + 1) % NUM_OBJECTS;
        else if (key == GLFW_KEY_1)
            target = 0;
        else if (key == GLFW_KEY_2)
            target = 1;
        else if (key == GLFW_KEY_3)
            target = 2;
        else if (key == GLFW_KEY_4)
            target = 3;
        else if (key == GLFW_KEY_5)
            target = 4;

        if (target != -1)
        {
            current_object = target;
            std::cout << object_names[current_object] << std::endl;
            return;
        }
    }

    if (current_object == 0)
    {
        // Impossible polygon controls
        polygon_key_callback(window, key, scancode, action, mods);
    }
    else if (current_object == 1)
    {
        penrose_keyCallback(window, key, scancode, action, mods);
    }
    else if (current_object == 2)
    {
        cube_keyCallback(window, key, scancode, action, mods);
    }
    else if (current_object == 3)
    {
        penrose_block_keyCallback(window, key, scancode, action, mods);
    }
    else if (current_object == 4)
    {
        arch_keyCallback(window, key, scancode, action, mods);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (current_object == 0)
        polygon_mouseButtonCallback(window, button, action, mods);
    else if (current_object == 1)
        penrose_mouseButtonCallback(window, button, action, mods);
    else if (current_object == 2)
        cube_mouseButtonCallback(window, button, action, mods);
    else if (current_object == 3)
        penrose_block_mouseButtonCallback(window, button, action, mods);
    else if (current_object == 4)
        arch_mouseButtonCallback(window, button, action, mods);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (current_object == 0)
        polygon_cursorPosCallback(window, xpos, ypos);
    else if (current_object == 1)
        penrose_cursorPosCallback(window, xpos, ypos);
    else if (current_object == 2)
        cube_cursorPosCallback(window, xpos, ypos);
    else if (current_object == 3)
        penrose_block_cursorPosCallback(window, xpos, ypos);
    else if (current_object == 4)
        arch_cursorPosCallback(window, xpos, ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (current_object == 1)
        penrose_scrollCallback(window, xoffset, yoffset);
    else if (current_object == 2)
        cube_scrollCallback(window, xoffset, yoffset);
    else if (current_object == 3)
        penrose_block_scrollCallback(window, xoffset, yoffset);
    else if (current_object == 4)
        arch_scrollCallback(window, xoffset, yoffset);
}

// =============================================
// Main
// =============================================
int main()
{
    if (!glfwInit()) exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(512, 512, "Impossible Objects", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwGetFramebufferSize(window, &screen_w, &screen_h);

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

#ifndef __APPLE__
    glewInit();
#endif

    // Initialize all objects
    polygon_init();
    penrose_init();
    cube_init();
    penrose_block_init();
    arch_init();

    glClearColor(0.75f, 0.78f, 0.80f, 1.0f);  // light teal background
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    std::cout << "Press TAB to cycle objects, or 1-5 to jump directly" << std::endl;
    std::cout << "  1. Impossible Polygon: SPACE to spin, D/A to add/remove edges, mouse drag to rotate" << std::endl;
    std::cout << "  2. Penrose Triangle: arrow keys/mouse drag to rotate, R to reset, scroll to tilt" << std::endl;
    std::cout << "  3. Impossible Cube: arrow keys/mouse drag to rotate, R to reset, scroll to tilt" << std::endl;
    std::cout << "  4. Penrose Blocks: arrow keys/mouse drag to rotate, R to reset, scroll to tilt" << std::endl;
    std::cout << "  5. Impossible Arch: arrow keys/mouse drag to rotate, R to reset, scroll to tilt" << std::endl;

    double frameRate = 30, currentTime, previousTime = 0.0;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        currentTime = glfwGetTime();
        if (currentTime - previousTime >= 1 / frameRate)
        {
            previousTime = currentTime;
        }

        // Light teal background matching the image
        glClearColor(0.75f, 0.78f, 0.80f, 1.0f);

        if (current_object == 0)
            polygon_display();
        else if (current_object == 1)
            penrose_display();
        else if (current_object == 2)
            cube_display();
        else if (current_object == 3)
            penrose_block_display();
        else if (current_object == 4)
            arch_display();

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
