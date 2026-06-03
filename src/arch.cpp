#include "background.h"
#include "obj_shape.h"

// Impossible Arch — loaded from models/impossible_arch.obj
// (Paradox Toolkit primitive).

static ObjShape g_shape;

void arch_init()
{
    g_shape.init("../models/impossible_arch.obj", default_palette());

    // Rotate 90° CCW around Z (screen-plane CCW from the default iso view).
    // Default angleZ was -45; -45 + 90 = +45 lays the arch on its side.
    g_shape.angleZ = 45.0f;
    g_shape.defaultAngleZ = 45.0f;

    // Initialize gyroid background
    bg_init_gyroid();
}
void arch_display()
{
    // Begin background rendering
    bg_begin_scene();

    // Get camera info and draw gyroid background FIRST (so it appears behind)
    double now = glfwGetTime();
    vec3 eye = g_shape.cameraEye;
    vec3 at(0.0f, 0.0f, 0.0f);
    vec3 cam_forward = normalize(eye - at);
    vec3 cam_right = normalize(cross(vec3(0, 1, 0), cam_forward));
    vec3 cam_up = normalize(cross(cam_forward, cam_right));

    bg_draw_gyroid(eye, cam_right, cam_up, cam_forward, (float)now);

    // Render the arch on top
    g_shape.display();

    // End background rendering (applies bloom and outputs to screen)
    bg_end_scene();
}
void arch_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void arch_cursorPosCallback(GLFWwindow*, double x, double y) { g_shape.cursorPos(x, y); }
void arch_scrollCallback(GLFWwindow*, double, double y) { g_shape.scroll(y); }
void arch_keyCallback(GLFWwindow* w, int k, int, int a, int) { g_shape.key(w, k, a); }
