#include "includes.h"

GLuint penrose_block_shaderProgram;

// vertex array + buffers
GLuint penrose_block_vao;
GLuint penrose_block_positionBuffer;
GLuint penrose_block_colorBuffer;

// uniforms
GLuint penrose_block_modelPos;
GLuint penrose_block_viewPos;
GLuint penrose_block_projectionPos;

// ------------------------------------------------
// Penrose Triangle (Block Variant)
//
// Instead of continuous bars, this variant uses discrete
// cubes arranged along the triangular path.
//
// From the source (geometry_mesh_library.py lines 83-138):
//   L = 0.25 * size  (spacing between cubes)
//   Directions: 2x Ci, 4x Ai, 4x Bi, 2x Ci
//     Ai = (0, L, 0)    along +Y
//     Bi = (-L, 0, 0)   along -X
//     Ci = (0, 0, -L)   along -Z
//   Origin = (size/2, -size/2, size/2)
//
// This gives 13 cube positions (12 steps + 1 origin).
// The first and last cubes are "illusion cubes" where
// 4 of their 8 vertices are displaced to align with
// the illusion plane.
// ------------------------------------------------

// 13 cubes, each = 36 vertices
const int penrose_block_cubeCount = 13;
const int penrose_block_vertsPerCube = 36;
const int penrose_block_totalVertices = penrose_block_cubeCount * penrose_block_vertsPerCube;

vec4 penrose_block_positions[penrose_block_cubeCount * penrose_block_vertsPerCube];
vec4 penrose_block_colors[penrose_block_cubeCount * penrose_block_vertsPerCube];

// rotation (default axonometric view for illusion)
float penrose_block_angleX = 54.736f;
float penrose_block_angleY = 0.0f;
float penrose_block_angleZ = -45.0f;

// mouse
bool penrose_block_isDragging = false;
double penrose_block_mouseX = 0.0;
double penrose_block_mouseY = 0.0;

// ------------------------------------------------
// face colors per cube face
// ------------------------------------------------
vec4 penrose_block_faceColor(int faceIndex)
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
void penrose_block_buildCube(vec4* pos, vec4* col)
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
        vec4 c = penrose_block_faceColor(i);
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
void penrose_block_buildScene()
{
    for (int i = 0; i < penrose_block_cubeCount; i++)
    {
        penrose_block_buildCube(penrose_block_positions + i * penrose_block_vertsPerCube,
                                penrose_block_colors + i * penrose_block_vertsPerCube);
    }
}

// ------------------------------------------------
// mouse + keyboard
// ------------------------------------------------
void penrose_block_mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        penrose_block_isDragging = (action == GLFW_PRESS);
        if (penrose_block_isDragging) glfwGetCursorPos(window, &penrose_block_mouseX, &penrose_block_mouseY);
    }
}

void penrose_block_cursorPosCallback(GLFWwindow*, double x, double y)
{
    if (!penrose_block_isDragging) return;
    penrose_block_angleY += (float)(x - penrose_block_mouseX) * 0.4f;
    penrose_block_angleX += (float)(y - penrose_block_mouseY) * 0.4f;
    penrose_block_mouseX = x;
    penrose_block_mouseY = y;
}

void penrose_block_scrollCallback(GLFWwindow*, double, double yoffset)
{
    penrose_block_angleZ += (float)yoffset * 2.0f;
}

void penrose_block_keyCallback(GLFWwindow* win, int key, int, int action, int)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_LEFT) penrose_block_angleY -= 3.0f;
        if (key == GLFW_KEY_RIGHT) penrose_block_angleY += 3.0f;
        if (key == GLFW_KEY_UP) penrose_block_angleX -= 3.0f;
        if (key == GLFW_KEY_DOWN) penrose_block_angleX += 3.0f;

        if (key == GLFW_KEY_R)
        {
            penrose_block_angleX = 54.736f;
            penrose_block_angleY = 0.0f;
            penrose_block_angleZ = -45.0f;
        }
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, GL_TRUE);
    }
}

// ------------------------------------------------
// init
// ------------------------------------------------
void penrose_block_init()
{
    penrose_block_buildScene();

    glGenVertexArrays(1, &penrose_block_vao);
    glBindVertexArray(penrose_block_vao);

    glGenBuffers(1, &penrose_block_positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, penrose_block_positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(penrose_block_positions), penrose_block_positions, GL_STATIC_DRAW);

    penrose_block_shaderProgram = InitShader(SHADER_DIR "vshader_simple.glsl", SHADER_DIR "fshader_simple.glsl");
    glUseProgram(penrose_block_shaderProgram);

    GLuint posLoc = glGetAttribLocation(penrose_block_shaderProgram, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glGenBuffers(1, &penrose_block_colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, penrose_block_colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(penrose_block_colors), penrose_block_colors, GL_STATIC_DRAW);

    GLuint colLoc = glGetAttribLocation(penrose_block_shaderProgram, "vColor");
    glEnableVertexAttribArray(colLoc);
    glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.72f, 0.75f, 0.72f, 1.0f);

    penrose_block_modelPos = glGetUniformLocation(penrose_block_shaderProgram, "model");
    penrose_block_viewPos = glGetUniformLocation(penrose_block_shaderProgram, "view");
    penrose_block_projectionPos = glGetUniformLocation(penrose_block_shaderProgram, "projection");
}

// ------------------------------------------------
// display
// ------------------------------------------------
void penrose_block_display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(penrose_block_shaderProgram);
    glBindVertexArray(penrose_block_vao);

    vec3 eye(2.5f, 2.5f, 2.5f);
    vec3 at(0.0f, 0.0f, 0.0f);
    vec3 up(0.0f, 1.0f, 0.0f);
    mat4 view = LookAt(eye, at, up);
    float aspect = 550.0f / 500.0f;
    float orthoS = 1.5f;
    mat4 projection = Ortho(-orthoS * aspect, orthoS * aspect, -orthoS, orthoS, 0.1f, 20.0f);

    glUniformMatrix4fv(penrose_block_viewPos, 1, GL_FALSE, &view.d[0].x);
    glUniformMatrix4fv(penrose_block_projectionPos, 1, GL_FALSE, &projection.d[0].x);

    mat4 sceneRot = RotateY(penrose_block_angleY) * RotateX(penrose_block_angleX) * RotateZ(penrose_block_angleZ);

    float size = 1.0f;
    float L = 0.25f * size;
    float cubeSize = L * 0.85f;  // slightly smaller than spacing for gaps

    // Cube positions: accumulate direction vectors from origin
    // Origin = (size/2, -size/2, size/2) = (0.5, -0.5, 0.5)
    float cx = size / 2.0f;
    float cy = -size / 2.0f;
    float cz = size / 2.0f;

    // Direction pattern: 2xCi, 4xAi, 4xBi, 2xCi
    // Ci = (0, 0, -L), Ai = (0, L, 0), Bi = (-L, 0, 0)
    struct
    {
        float dx, dy, dz;
    } dirs[12] = {
        {0, 0, -L}, {0, 0, -L},                          // 2x down
        {0, L, 0},  {0, L, 0},  {0, L, 0},  {0, L, 0},   // 4x +Y
        {-L, 0, 0}, {-L, 0, 0}, {-L, 0, 0}, {-L, 0, 0},  // 4x -X
        {0, 0, -L}, {0, 0, -L}                           // 2x down (close)
    };

    // Draw cube 0 at origin
    mat4 model0 = sceneRot * Translate(cx, cy, cz) * Scale(cubeSize, cubeSize, cubeSize);
    glUniformMatrix4fv(penrose_block_modelPos, 1, GL_FALSE, &model0.d[0].x);
    glDrawArrays(GL_TRIANGLES, 0, penrose_block_vertsPerCube);

    // Draw cubes 1-12 at accumulated positions
    for (int i = 0; i < 12; i++)
    {
        cx += dirs[i].dx;
        cy += dirs[i].dy;
        cz += dirs[i].dz;

        mat4 model = sceneRot * Translate(cx, cy, cz) * Scale(cubeSize, cubeSize, cubeSize);
        glUniformMatrix4fv(penrose_block_modelPos, 1, GL_FALSE, &model.d[0].x);
        glDrawArrays(GL_TRIANGLES, (i + 1) * penrose_block_vertsPerCube, penrose_block_vertsPerCube);
    }

    glFinish();
}
