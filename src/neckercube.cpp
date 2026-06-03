#include "background.h"
#include "obj_shape.h"

// Impossible Cube — loaded from models/impossible_cube.obj
// (Paradox Toolkit primitive). Replaces the previous hand-built 12-bar version.

static ObjShape g_shape;

// Distinct cube palette
static ObjColorPalette cube_palette()
{
    ObjColorPalette p;
    p.xp = vec4(0.67f, 0.75f, 0.92f, 1.0f);
    p.xn = vec4(0.50f, 0.56f, 0.75f, 1.0f);
    p.yp = vec4(0.88f, 0.88f, 0.70f, 1.0f);
    p.yn = vec4(0.75f, 0.75f, 0.58f, 1.0f);
    p.zp = vec4(0.66f, 0.56f, 0.63f, 1.0f);
    p.zn = vec4(0.52f, 0.43f, 0.50f, 1.0f);
    p.generic = vec4(0.55f, 0.55f, 0.58f, 1.0f);
    return p;
}

void cube_init()
{
    g_shape.init("../models/impossible_cube.obj", cube_palette());

    // This Paradox cube has a larger bounding box than the other figures —
    // pull the ortho frustum out a bit so it appears at a similar on-screen
    // size as slots 2, 3, 5–8.
    g_shape.orthoSize = 12.5f;  // was 9.5 (the shared default)

    // Initialize tunnel background
    bg_init_tunnel();
}
void cube_display()
{
    // Begin background rendering
    bg_begin_scene();

    // Get camera info and draw tunnel background FIRST (so it appears behind)
    double now = glfwGetTime();
    vec3 eye = g_shape.cameraEye;
    vec3 at(0.0f, 0.0f, 0.0f);
    vec3 cam_forward = normalize(eye - at);
    vec3 cam_right = normalize(cross(vec3(0, 1, 0), cam_forward));
    vec3 cam_up = normalize(cross(cam_forward, cam_right));

    bg_draw_tunnel(eye, cam_right, cam_up, cam_forward, (float)now);

    // Render the cube on top
    g_shape.display();

    // End background rendering (applies bloom and outputs to screen)
    bg_end_scene();
}
void cube_mouseButtonCallback(GLFWwindow* w, int b, int a, int) { g_shape.mouseButton(w, b, a); }
void cube_cursorPosCallback(GLFWwindow*, double x, double y) { g_shape.cursorPos(x, y); }
void cube_scrollCallback(GLFWwindow*, double, double y) { g_shape.scroll(y); }
void cube_keyCallback(GLFWwindow* w, int k, int, int a, int) { g_shape.key(w, k, a); }
