#include "impossible_polygon.h"
#include "includes.h"
#include "neckercube.h"
#include "penrose.h"
#include "ui.h"

int screen_w, screen_h;  // Screen Attributes

// ─── Forward decls — modules without their own headers ──────────────────────

// OBJ-loaded modules
void arch_init();
void arch_display();
void arch_mouseButtonCallback(GLFWwindow*, int, int, int);
void arch_cursorPosCallback(GLFWwindow*, double, double);
void arch_scrollCallback(GLFWwindow*, double, double);
void arch_keyCallback(GLFWwindow*, int, int, int, int);

void penrose_block_init();
void penrose_block_display();
void penrose_block_mouseButtonCallback(GLFWwindow*, int, int, int);
void penrose_block_cursorPosCallback(GLFWwindow*, double, double);
void penrose_block_scrollCallback(GLFWwindow*, double, double);
void penrose_block_keyCallback(GLFWwindow*, int, int, int, int);

void reutersvard_init();
void reutersvard_display();
void reutersvard_mouseButtonCallback(GLFWwindow*, int, int, int);
void reutersvard_cursorPosCallback(GLFWwindow*, double, double);
void reutersvard_scrollCallback(GLFWwindow*, double, double);
void reutersvard_keyCallback(GLFWwindow*, int, int, int, int);

// Procedural modules
//   pbp_*   — Blocked Penrose (Paradox block variant)    (penrose_blocks_proc.cpp)
//   archp_* — Impossible Arch wide-squat variant         (arch_proc.cpp)
void pbp_init();
void pbp_display();
void pbp_mouseButtonCallback(GLFWwindow*, int, int, int);
void pbp_cursorPosCallback(GLFWwindow*, double, double);
void pbp_scrollCallback(GLFWwindow*, double, double);
void pbp_keyCallback(GLFWwindow*, int, int, int, int);

void archp_init();
void archp_display();
void archp_mouseButtonCallback(GLFWwindow*, int, int, int);
void archp_cursorPosCallback(GLFWwindow*, double, double);
void archp_scrollCallback(GLFWwindow*, double, double);
void archp_keyCallback(GLFWwindow*, int, int, int, int);

// ─── Shape registry ─────────────────────────────────────────────────────────
const int NUM_OBJECTS = 8;
const char* object_names[NUM_OBJECTS] = {
    "Impossible Polygon",         // 0 — procedural (parametric n-gon)
    "Penrose Triangle",           // 1 — OBJ
    "Blocked Penrose (Blender)",  // 2 — OBJ (Paradox block variant)
    "Impossible Cube",            // 3 — OBJ
    "Impossible Arch",            // 4 — OBJ (tall narrow variant)
    "Impossible Arch (round)",    // 5 — Paradox arch sphere-cast to bend bars
    "Penrose Stair",              // 6 — OBJ
    "Reutersvard Rectangle",      // 7 — OBJ
};

int current_object = 0;
AppState app_state = AppState::TITLE;

// ─── Per-slot callback dispatch ────────────────────────────────────────────
static void key_for(int slot, GLFWwindow* w, int k, int s, int a, int m)
{
    switch (slot)
    {
        case 0: polygon_key_callback(w, k, s, a, m);    break;
        case 1: penrose_keyCallback(w, k, s, a, m);     break;
        case 2: pbp_keyCallback(w, k, s, a, m);         break;
        case 3: cube_keyCallback(w, k, s, a, m);        break;
        case 4: arch_keyCallback(w, k, s, a, m);        break;
        case 5: archp_keyCallback(w, k, s, a, m);       break;
        case 6: penrose_block_keyCallback(w, k, s, a, m); break;
        case 7: reutersvard_keyCallback(w, k, s, a, m);   break;
    }
}
static void mouse_for(int slot, GLFWwindow* w, int b, int a, int m)
{
    switch (slot)
    {
        case 0: polygon_mouseButtonCallback(w, b, a, m); break;
        case 1: penrose_mouseButtonCallback(w, b, a, m); break;
        case 2: pbp_mouseButtonCallback(w, b, a, m);     break;
        case 3: cube_mouseButtonCallback(w, b, a, m);    break;
        case 4: arch_mouseButtonCallback(w, b, a, m);    break;
        case 5: archp_mouseButtonCallback(w, b, a, m);   break;
        case 6: penrose_block_mouseButtonCallback(w, b, a, m); break;
        case 7: reutersvard_mouseButtonCallback(w, b, a, m);   break;
    }
}
static void cursor_for(int slot, GLFWwindow* w, double x, double y)
{
    switch (slot)
    {
        case 0: polygon_cursorPosCallback(w, x, y); break;
        case 1: penrose_cursorPosCallback(w, x, y); break;
        case 2: pbp_cursorPosCallback(w, x, y);     break;
        case 3: cube_cursorPosCallback(w, x, y);    break;
        case 4: arch_cursorPosCallback(w, x, y);    break;
        case 5: archp_cursorPosCallback(w, x, y);   break;
        case 6: penrose_block_cursorPosCallback(w, x, y); break;
        case 7: reutersvard_cursorPosCallback(w, x, y);   break;
    }
}
static void scroll_for(int slot, GLFWwindow* w, double x, double y)
{
    switch (slot)
    {
        case 1: penrose_scrollCallback(w, x, y);     break;
        case 2: pbp_scrollCallback(w, x, y);         break;
        case 3: cube_scrollCallback(w, x, y);        break;
        case 4: arch_scrollCallback(w, x, y);        break;
        case 5: archp_scrollCallback(w, x, y);       break;
        case 6: penrose_block_scrollCallback(w, x, y); break;
        case 7: reutersvard_scrollCallback(w, x, y);   break;
        default: break;
    }
}
static void display_for(int slot)
{
    switch (slot)
    {
        case 0: polygon_display(); break;
        case 1: penrose_display(); break;
        case 2: pbp_display();     break;
        case 3: cube_display();    break;
        case 4: arch_display();    break;
        case 5: archp_display();   break;
        case 6: penrose_block_display(); break;
        case 7: reutersvard_display();   break;
    }
}

// =============================================
// Unified GLFW callbacks
// =============================================
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (ui_wants_keyboard()) return;

    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
    {
        if (app_state == AppState::IN_SHAPE)
        {
            app_state = AppState::SHRINE_SELECT;
            return;
        }
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }

    if (app_state != AppState::IN_SHAPE) return;

    if (action == GLFW_PRESS)
    {
        int target = -1;
        if      (key == GLFW_KEY_TAB) target = (current_object + 1) % NUM_OBJECTS;
        else if (key == GLFW_KEY_1)   target = 0;
        else if (key == GLFW_KEY_2)   target = 1;
        else if (key == GLFW_KEY_3)   target = 2;
        else if (key == GLFW_KEY_4)   target = 3;
        else if (key == GLFW_KEY_5)   target = 4;
        else if (key == GLFW_KEY_6)   target = 5;
        else if (key == GLFW_KEY_7)   target = 6;
        else if (key == GLFW_KEY_8)   target = 7;

        if (target != -1)
        {
            current_object = target;
            std::cout << object_names[current_object] << std::endl;
            return;
        }
    }

    key_for(current_object, window, key, scancode, action, mods);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (ui_wants_mouse()) return;
    if (app_state != AppState::IN_SHAPE) return;
    mouse_for(current_object, window, button, action, mods);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (ui_wants_mouse()) return;
    if (app_state != AppState::IN_SHAPE) return;
    cursor_for(current_object, window, xpos, ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (ui_wants_mouse()) return;
    if (app_state != AppState::IN_SHAPE) return;
    scroll_for(current_object, window, xoffset, yoffset);
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

    // Frame matches the aspect ratio of the landing UI background image
    // renders/31d_colorwindows_4k.png (3840 × 3598, aspect ≈ 1.0673). 1280×1199
    // keeps the width close to the previous 1280 and brings the height up to
    // match the image so the background can fill the window without distortion.
    GLFWwindow* window = glfwCreateWindow(1280, 1199, "Impossible Objects", NULL, NULL);
    if (!window) { glfwTerminate(); exit(EXIT_FAILURE); }

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
    reutersvard_init();
    pbp_init();
    archp_init();

    ui_init(window);

    glClearColor(0.75f, 0.78f, 0.80f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    std::cout << "Press TAB to cycle objects, or 1-8 to jump directly" << std::endl;
    for (int i = 0; i < NUM_OBJECTS; i++)
        std::cout << "  " << (i + 1) << ". " << object_names[i] << std::endl;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.75f, 0.78f, 0.80f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ui_begin_frame();

        if (app_state == AppState::IN_SHAPE)
            display_for(current_object);
        else
            ui_draw_menu(app_state, current_object, window);

        ui_end_frame();
        glfwSwapBuffers(window);
    }

    ui_shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
