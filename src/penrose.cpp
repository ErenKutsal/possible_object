#include "includes.h"

GLuint penrose_shaderProgram;

// vertex array + buffers
GLuint penrose_vao;
GLuint penrose_positionBuffer;
GLuint penrose_colorBuffer;

// uniforms
GLuint penrose_modelPos;
GLuint penrose_viewPos;
GLuint penrose_projectionPos;

// ------------------------------------------------
// Penrose Triangle (Block Variant)
//
// Per Inglis (Bridges 2014): three legs along world +X, +Y, +Z,
// each L*N long. Under our isometric view (camera at (1,1,1)),
// the three axes project to screen-space vectors that sum to
// zero — so the path closes in 2D. In 3D, the chain ends at
// (L*N, L*N, L*N), which lies along the view ray through the
// origin. The last cube therefore projects to the same screen
// position as the first cube, but at a closer depth, so it
// hides cube 0 — producing the impossible-corner illusion.
//
// 13 cubes total: 4 per leg + 1 shared corner, with corners at
//   cube 0  (Z↔X corner — the impossible one)
//   cube 4  (X↔Y corner — a real 3D corner)
//   cube 8  (Y↔Z corner — a real 3D corner)
//   cube 12 (overlaps cube 0 in screen space)
// ------------------------------------------------

// 13 cubes, each = 36 vertices
const int penrose_cubeCount = 13;
const int penrose_vertsPerCube = 36;
const int penrose_totalVertices = penrose_cubeCount * penrose_vertsPerCube;

vec4 penrose_positions[penrose_cubeCount * penrose_vertsPerCube];
vec4 penrose_colors[penrose_cubeCount * penrose_vertsPerCube];

// rotation (default axonometric view for illusion)
float penrose_angleX = 54.736f;
float penrose_angleY = 0.0f;
float penrose_angleZ = -45.0f;

// mouse
bool penrose_isDragging = false;
double penrose_mouseX = 0.0;
double penrose_mouseY = 0.0;

// Some new globals
GLint penrose_light_loc;
GLint penrose_eye_loc;
GLint penrose_time_loc;
GLint penrose_height_loc;

// ------------------------------------------------
// face colors per cube face
// ------------------------------------------------
vec4 penrose_faceColor(int faceIndex)
{
    if (faceIndex == 0) return vec4(0.45f, 0.48f, 0.62f, 1.0f);  // X-
    if (faceIndex == 1) return vec4(0.38f, 0.40f, 0.55f, 1.0f);  // X+
    if (faceIndex == 2) return vec4(0.76f, 0.60f, 0.64f, 1.0f);  // Y+
    if (faceIndex == 3) return vec4(0.35f, 0.37f, 0.52f, 1.0f);  // Y-
    if (faceIndex == 4) return vec4(0.73f, 0.58f, 0.62f, 1.0f);  // Z+
    return vec4(0.42f, 0.45f, 0.60f, 1.0f);                      // Z-
}

// ------------------------------------------------
// build one cube
// ------------------------------------------------
void penrose_buildCube(vec4* pos, vec4* col)
{
    vec4 v[8] = {vec4(-0.5f, -0.5f, -0.5f, 1.0f), vec4(-0.5f, -0.5f, 0.5f, 1.0f), vec4(-0.5f, 0.5f, -0.5f, 1.0f),
                 vec4(-0.5f, 0.5f, 0.5f, 1.0f),   vec4(0.5f, -0.5f, -0.5f, 1.0f), vec4(0.5f, -0.5f, 0.5f, 1.0f),
                 vec4(0.5f, 0.5f, -0.5f, 1.0f),   vec4(0.5f, 0.5f, 0.5f, 1.0f)};

    // face winding from Blender addon CubePoint class
    int faces[6][4] = {
        {0, 1, 3, 2},  // X-
        {6, 7, 5, 4},  // X+
        {2, 3, 7, 6},  // Y+
        {4, 5, 1, 0},  // Y-
        {7, 3, 1, 5},  // Z+
        {2, 6, 4, 0}   // Z-
    };

    int idx = 0;
    for (int i = 0; i < 6; i++)
    {
        vec4 c = penrose_faceColor(i);
        pos[idx] = v[faces[i][0]];
        col[idx++] = c;
        pos[idx] = v[faces[i][1]];
        col[idx++] = c;
        pos[idx] = v[faces[i][2]];
        col[idx++] = c;

        pos[idx] = v[faces[i][0]];
        col[idx++] = c;
        pos[idx] = v[faces[i][2]];
        col[idx++] = c;
        pos[idx] = v[faces[i][3]];
        col[idx++] = c;
    }
}

// ------------------------------------------------
// Build the block triangle scene
//
// Cube positions derived from the direction vectors:
//   Starting at origin = (size/2, -size/2, size/2)
//   L = 0.25 * size
//
//   Step 0:  origin
//   Step 1:  + (0, 0, -L)       2x down in Z
//   Step 2:  + (0, 0, -L)
//   Step 3:  + (0, L, 0)        4x along +Y
//   Step 4:  + (0, L, 0)
//   Step 5:  + (0, L, 0)
//   Step 6:  + (0, L, 0)
//   Step 7:  + (-L, 0, 0)       4x along -X
//   Step 8:  + (-L, 0, 0)
//   Step 9:  + (-L, 0, 0)
//   Step 10: + (-L, 0, 0)
//   Step 11: + (0, 0, -L)       2x down in Z (closes loop)
//   Step 12: + (0, 0, -L)
// ------------------------------------------------
void penrose_buildScene()
{
    for (int i = 0; i < penrose_cubeCount; i++)
    {
        penrose_buildCube(penrose_positions + i * penrose_vertsPerCube,
                                penrose_colors + i * penrose_vertsPerCube);
    }
}

// ------------------------------------------------
// mouse + keyboard
// ------------------------------------------------
void penrose_mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        penrose_isDragging = (action == GLFW_PRESS);
        if (penrose_isDragging) glfwGetCursorPos(window, &penrose_mouseX, &penrose_mouseY);
    }
}

void penrose_cursorPosCallback(GLFWwindow*, double x, double y)
{
    if (!penrose_isDragging) return;
    penrose_angleY += (float)(x - penrose_mouseX) * 0.4f;
    penrose_angleX += (float)(y - penrose_mouseY) * 0.4f;
    penrose_mouseX = x;
    penrose_mouseY = y;
}

void penrose_scrollCallback(GLFWwindow*, double, double yoffset)
{
    penrose_angleZ += (float)yoffset * 2.0f;
}

void penrose_keyCallback(GLFWwindow* win, int key, int, int action, int)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_LEFT) penrose_angleY -= 3.0f;
        if (key == GLFW_KEY_RIGHT) penrose_angleY += 3.0f;
        if (key == GLFW_KEY_UP) penrose_angleX -= 3.0f;
        if (key == GLFW_KEY_DOWN) penrose_angleX += 3.0f;

        if (key == GLFW_KEY_R)
        {
            penrose_angleX = 54.736f;
            penrose_angleY = 0.0f;
            penrose_angleZ = -45.0f;
        }
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, GL_TRUE);
    }
}

// ------------------------------------------------
// init
// ------------------------------------------------
void penrose_init()
{
    penrose_buildScene();

    glGenVertexArrays(1, &penrose_vao);
    glBindVertexArray(penrose_vao);

    glGenBuffers(1, &penrose_positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, penrose_positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(penrose_positions), penrose_positions, GL_STATIC_DRAW);

    /*
    penrose_shaderProgram = InitShader("../shaders/vshader_simple.glsl", "../shaders/fshader_simple.glsl");
    glUseProgram(penrose_shaderProgram);
    */

    // Change the shader:
    penrose_shaderProgram =
        InitShader("../shaders/vshader_impossible.glsl", "../shaders/fshader_impossible.glsl");

    // Add these uniform locations alongside the existing ones:
    penrose_light_loc = glGetUniformLocation(penrose_shaderProgram, "uLightPos");
    penrose_eye_loc = glGetUniformLocation(penrose_shaderProgram, "uEyePos");
    penrose_time_loc = glGetUniformLocation(penrose_shaderProgram, "uTime");
    penrose_height_loc = glGetUniformLocation(penrose_shaderProgram, "uObjHeight");

    GLuint posLoc = glGetAttribLocation(penrose_shaderProgram, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glGenBuffers(1, &penrose_colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, penrose_colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(penrose_colors), penrose_colors, GL_STATIC_DRAW);

    GLuint colLoc = glGetAttribLocation(penrose_shaderProgram, "vColor");
    glEnableVertexAttribArray(colLoc);
    glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glEnable(GL_DEPTH_TEST);
    // glClearColor(0.72f, 0.75f, 0.72f, 1.0f); // No need if theres a bg

    penrose_modelPos = glGetUniformLocation(penrose_shaderProgram, "model");
    penrose_viewPos = glGetUniformLocation(penrose_shaderProgram, "view");
    penrose_projectionPos = glGetUniformLocation(penrose_shaderProgram, "projection");
}

// ------------------------------------------------
// display
// ------------------------------------------------
void penrose_display()
{
    glUseProgram(penrose_shaderProgram);
    glBindVertexArray(penrose_vao);

    vec3 eye(2.5f, 2.5f, 2.5f);
    vec3 at(0.0f, 0.0f, 0.0f);
    vec3 up(0.0f, 1.0f, 0.0f);
    mat4 view = LookAt(eye, at, up);
    float aspect = 550.0f / 500.0f;
    float orthoS = 1.5f;
    mat4 projection = Ortho(-orthoS * aspect, orthoS * aspect, -orthoS, orthoS, 0.1f, 20.0f);

    glUniformMatrix4fv(penrose_viewPos, 1, GL_FALSE, &view.d[0].x);
    glUniformMatrix4fv(penrose_projectionPos, 1, GL_FALSE, &projection.d[0].x);

    mat4 sceneRot = RotateY(penrose_angleY) * RotateX(penrose_angleX) * RotateZ(penrose_angleZ);

    vec3 lightPos(2.0f, 4.0f, 3.0f);
    glUniform3fv(penrose_light_loc, 1, &lightPos.x);
    glUniform3fv(penrose_eye_loc, 1, &eye.x);  // eye already computed for view matrix
    glUniform1f(penrose_time_loc, glfwGetTime());
    glUniform1f(penrose_height_loc, 1.0f);  // tune to match object's world height

    float L = 0.25f;            // cube edge / spacing along each leg
    const int N = 4;            // steps per leg → 1 + 3N = 13 cubes

    // Center the triangle on screen. The triangle has 3D corners
    //   O = origin, A = (LN,0,0), B = (LN,LN,0)
    // whose screen-space centroid is shifted; offset the origin by
    // (-LN/3, 0, LN/3) so the centroid lands at (0,0) on screen.
    float ox = -L * N / 3.0f;
    float oy = 0.0f;
    float oz = L * N / 3.0f;

    // Direction sequence: 4×+X, 4×+Y, 4×+Z. Sum projects to 0 under
    // isometric view but is (LN,LN,LN) in 3D — the impossible figure.
    struct { float dx, dy, dz; } dirs[12] = {
        { L, 0, 0}, { L, 0, 0}, { L, 0, 0}, { L, 0, 0},  // +X leg
        { 0, L, 0}, { 0, L, 0}, { 0, L, 0}, { 0, L, 0},  // +Y leg
        { 0, 0, L}, { 0, 0, L}, { 0, 0, L}, { 0, 0, L},  // +Z leg
    };

    float cx = ox, cy = oy, cz = oz;

    // Draw cube 0 at origin (farthest from camera — drawn first so
    // cube 12 will cover it cleanly at the impossible corner).
    mat4 model0 = sceneRot * Translate(cx, cy, cz) * Scale(L, L, L);
    glUniformMatrix4fv(penrose_modelPos, 1, GL_FALSE, &model0.d[0].x);
    glDrawArrays(GL_TRIANGLES, 0, penrose_vertsPerCube);

    for (int i = 0; i < 12; i++)
    {
        cx += dirs[i].dx;
        cy += dirs[i].dy;
        cz += dirs[i].dz;

        mat4 model = sceneRot * Translate(cx, cy, cz) * Scale(L, L, L);
        glUniformMatrix4fv(penrose_modelPos, 1, GL_FALSE, &model.d[0].x);
        glDrawArrays(GL_TRIANGLES, (i + 1) * penrose_vertsPerCube, penrose_vertsPerCube);
    }

    glFinish();
}
