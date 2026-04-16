#include "includes.h"

GLuint stair_shaderProgram;

// vertex array + buffers
GLuint stair_vao;
GLuint stair_positionBuffer;
GLuint stair_colorBuffer;

// uniforms
GLuint stair_modelPos;
GLuint stair_viewPos;
GLuint stair_projectionPos;

// ------------------------------------------------
// Penrose Staircase
//
// An impossible staircase that appears to continuously
// ascend (or descend) yet forms a closed loop.
//
// From geometry_mesh_library.py lines 271-343:
//   Step spacing:
//     L1 = 2.0 * size/8   (horizontal step distance)
//     L2 = 0.5 * size/8   (ascending step height)
//     L3 = 0.25 * size/8  (descending step height)
//
//   Direction vectors per step:
//     Ai = (0, L1, L2)     ascend along +Y
//     Bi = (-L1, 0, L2)    ascend along -X
//     Ci = (0, -L1, L3)    descend along -Y
//     Di = (L1, 0, L3)     descend along +X
//
//   Sequence: 2xDi, 2xAi, 2xBi, 4xCi, 2xDi = 14 steps
//
//   Constraint (loop closure):
//     N1*(L1*y + L2*z) + N1*(-L1*x + L2*z)
//     + N2*(-L1*y + L3*z) + N2*(L1*x + L3*z) = (0,0)
//     where N1=2, N2=4
//
//   Origin = (L1, -L1, 0)
// ------------------------------------------------

// 13 cubes (12 steps + 1 origin position)
const int stair_cubeCount = 13;
const int stair_vertsPerCube = 36;
const int stair_totalVertices = stair_cubeCount * stair_vertsPerCube;

vec4 stair_positions[stair_cubeCount * stair_vertsPerCube];
vec4 stair_colors[stair_cubeCount * stair_vertsPerCube];

// rotation
float stair_angleX = 54.736f;
float stair_angleY = 0.0f;
float stair_angleZ = -45.0f;

// mouse
bool stair_isDragging = false;
double stair_mouseX = 0.0;
double stair_mouseY = 0.0;

// ------------------------------------------------
// face colors: warm stone tones
// ------------------------------------------------
vec4 stair_faceColor(int faceIndex)
{
    // Three visible faces in the isometric view get three distinct blue shades
    if (faceIndex == 4) return vec4(0.52f, 0.78f, 0.95f, 1.0f);  // Z+ top:   light blue
    if (faceIndex == 1) return vec4(0.20f, 0.46f, 0.80f, 1.0f);  // X+ right: medium blue
    if (faceIndex == 2) return vec4(0.08f, 0.22f, 0.56f, 1.0f);  // Y+ front: dark blue
    return vec4(0.06f, 0.16f, 0.42f, 1.0f);                       // hidden faces
}

// ------------------------------------------------
// build one step cube
// ------------------------------------------------
void stair_buildCube(vec4* pos, vec4* col)
{
    vec4 v[8] = {
        vec4(-0.5f, -0.5f, -0.5f, 1.0f),
        vec4(-0.5f, -0.5f,  0.5f, 1.0f),
        vec4(-0.5f,  0.5f, -0.5f, 1.0f),
        vec4(-0.5f,  0.5f,  0.5f, 1.0f),
        vec4( 0.5f, -0.5f, -0.5f, 1.0f),
        vec4( 0.5f, -0.5f,  0.5f, 1.0f),
        vec4( 0.5f,  0.5f, -0.5f, 1.0f),
        vec4( 0.5f,  0.5f,  0.5f, 1.0f)
    };

    int faces[6][4] = {
        {0, 1, 3, 2}, {6, 7, 5, 4},
        {2, 3, 7, 6}, {4, 5, 1, 0},
        {7, 3, 1, 5}, {2, 6, 4, 0}
    };

    int idx = 0;
    for (int i = 0; i < 6; i++)
    {
        vec4 c = stair_faceColor(i);
        pos[idx] = v[faces[i][0]]; col[idx++] = c;
        pos[idx] = v[faces[i][1]]; col[idx++] = c;
        pos[idx] = v[faces[i][2]]; col[idx++] = c;
        pos[idx] = v[faces[i][0]]; col[idx++] = c;
        pos[idx] = v[faces[i][2]]; col[idx++] = c;
        pos[idx] = v[faces[i][3]]; col[idx++] = c;
    }
}

void stair_buildScene()
{
    for (int i = 0; i < stair_cubeCount; i++)
        stair_buildCube(stair_positions + i * stair_vertsPerCube,
                        stair_colors    + i * stair_vertsPerCube);
}

// ------------------------------------------------
// mouse + keyboard
// ------------------------------------------------
void stair_mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        stair_isDragging = (action == GLFW_PRESS);
        if (stair_isDragging)
            glfwGetCursorPos(window, &stair_mouseX, &stair_mouseY);
    }
}

void stair_cursorPosCallback(GLFWwindow*, double x, double y)
{
    if (!stair_isDragging) return;
    stair_angleY += (float)(x - stair_mouseX) * 0.4f;
    stair_angleX += (float)(y - stair_mouseY) * 0.4f;
    stair_mouseX = x;
    stair_mouseY = y;
}

void stair_scrollCallback(GLFWwindow*, double, double yoffset)
{
    stair_angleZ += (float)yoffset * 2.0f;
}

void stair_keyCallback(GLFWwindow* win, int key, int, int action, int)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_LEFT)  stair_angleY -= 3.0f;
        if (key == GLFW_KEY_RIGHT) stair_angleY += 3.0f;
        if (key == GLFW_KEY_UP)    stair_angleX -= 3.0f;
        if (key == GLFW_KEY_DOWN)  stair_angleX += 3.0f;

        if (key == GLFW_KEY_R)
        {
            stair_angleX = 54.736f;
            stair_angleY = 0.0f;
            stair_angleZ = -45.0f;
        }
        if (key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(win, GL_TRUE);
    }
}

// ------------------------------------------------
// init
// ------------------------------------------------
void stair_init()
{
    stair_buildScene();

    glGenVertexArrays(1, &stair_vao);
    glBindVertexArray(stair_vao);

    glGenBuffers(1, &stair_positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, stair_positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(stair_positions), stair_positions, GL_STATIC_DRAW);

    stair_shaderProgram = InitShader(SHADER_DIR "vshader_simple.glsl", SHADER_DIR "fshader_simple.glsl");
    glUseProgram(stair_shaderProgram);

    GLuint posLoc = glGetAttribLocation(stair_shaderProgram, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glGenBuffers(1, &stair_colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, stair_colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(stair_colors), stair_colors, GL_STATIC_DRAW);

    GLuint colLoc = glGetAttribLocation(stair_shaderProgram, "vColor");
    glEnableVertexAttribArray(colLoc);
    glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.72f, 0.75f, 0.72f, 1.0f);

    stair_modelPos      = glGetUniformLocation(stair_shaderProgram, "model");
    stair_viewPos        = glGetUniformLocation(stair_shaderProgram, "view");
    stair_projectionPos  = glGetUniformLocation(stair_shaderProgram, "projection");
}

// ------------------------------------------------
// display
//
// Staircase with size=1.0:
//   L1 = 0.25 (horizontal), L2 = 0.0625 (up), L3 = 0.03125 (down)
//
//   Step directions:
//     Di = (+L1, 0, +L3)   ascend along +X
//     Ai = (0, +L1, +L2)   ascend along +Y
//     Bi = (-L1, 0, +L2)   ascend along -X
//     Ci = (0, -L1, +L3)   descend along -Y
//
//   Sequence: 2xDi, 2xAi, 2xBi, 4xCi, 2xDi
//   Origin = (L1, -L1, 0) = (0.25, -0.25, 0)
//
//   Each step is a small cube placed at the accumulated position.
// ------------------------------------------------
void stair_display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(stair_shaderProgram);
    glBindVertexArray(stair_vao);

    vec3 eye(1.5f, 1.5f, 1.5f);
    vec3 at(0.0f, 0.0f, 0.3f);
    vec3 up(0.0f, 0.0f, 1.0f);
    mat4 view = LookAt(eye, at, up);
    float aspect = 550.0f / 500.0f;
    float orthoS = 1.2f;
    mat4 projection = Ortho(-orthoS * aspect, orthoS * aspect, -orthoS, orthoS, 0.1f, 20.0f);

    glUniformMatrix4fv(stair_viewPos, 1, GL_FALSE, &view.d[0].x);
    glUniformMatrix4fv(stair_projectionPos, 1, GL_FALSE, &projection.d[0].x);

    mat4 sceneRot = RotateY(stair_angleY)
                  * RotateX(stair_angleX)
                  * RotateZ(stair_angleZ);

    float size = 1.0f;
    float L1 = 2.0f * size / 8.0f;    // 0.25  horizontal step
    float L2 = 0.5f * size / 8.0f;    // 0.0625 ascending step (Ai/Bi arms)
    float L3 = 0.25f * size / 8.0f;   // 0.03125 ascending step (Di/Ci arms)
    float cubeSize = L1;               // touching cubes, no gaps

    // Symmetric 4-arm sequence: 3xDi, 3xAi, 3xBi, 3xCi = 12 steps
    // Closes in X and Y (net displacement zero), ascends continuously in Z
    struct { float dx, dy, dz; } dirs[12] = {
        { L1, 0,    L3}, { L1, 0,    L3}, { L1, 0,    L3},  // 3x Di  (+X)
        {0,   L1,   L2}, {0,   L1,   L2}, {0,   L1,   L2},  // 3x Ai  (+Y)
        {-L1, 0,    L2}, {-L1, 0,    L2}, {-L1, 0,    L2},  // 3x Bi  (-X)
        {0,  -L1,   L3}, {0,  -L1,   L3}, {0,  -L1,   L3},  // 3x Ci  (-Y)
    };

    // Center the loop: origin at (-1.5*L1, -1.5*L1, 0)
    float cx = -1.5f * L1;
    float cy = -1.5f * L1;
    float cz = 0.0f;

    // Draw cube 0 at origin
    mat4 model0 = sceneRot * Translate(cx, cy, cz) * Scale(cubeSize, cubeSize, cubeSize);
    glUniformMatrix4fv(stair_modelPos, 1, GL_FALSE, &model0.d[0].x);
    glDrawArrays(GL_TRIANGLES, 0, stair_vertsPerCube);

    // Draw cubes 1-12 at accumulated positions
    for (int i = 0; i < 12; i++)
    {
        cx += dirs[i].dx;
        cy += dirs[i].dy;
        cz += dirs[i].dz;

        mat4 model = sceneRot * Translate(cx, cy, cz) * Scale(cubeSize, cubeSize, cubeSize);
        glUniformMatrix4fv(stair_modelPos, 1, GL_FALSE, &model.d[0].x);
        glDrawArrays(GL_TRIANGLES, (i + 1) * stair_vertsPerCube, stair_vertsPerCube);
    }

    glFinish();
}
