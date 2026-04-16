#include "includes.h"

GLuint arch_shaderProgram;

// vertex array + buffers
GLuint arch_vao;
GLuint arch_positionBuffer;
GLuint arch_colorBuffer;

// uniforms
GLuint arch_modelPos;
GLuint arch_viewPos;
GLuint arch_projectionPos;

// ------------------------------------------------
// Impossible Arch
//
// Ported from Blender_ParadoxToolkit by Matias Garate
// https://github.com/matgarate/Blender_ParadoxToolkit
//
// An impossible arch formed by 5 bars in an open loop.
// The vertical side has an illusion break.
//
// From geometry_mesh_library.py lines 209-265:
//   Constraint: L1*x + L2*y + L3*z - L4*x - L5*z = (0,0)
//
//   Direction vectors (height=2.0, width_A=1.0, width_B=1.0):
//     L0_ia = (0, 0, -1.0)             illusion break top
//     L1    = (width_A, 0, 0)           right step along +X
//     L2    = (0, width_B, 0)           forward step along +Y
//     L3    = (0, 0, height-width_B)    rise along +Z
//     L4    = (-(width_A+width_B), 0, 0) return along -X
//     L5_ib = (0, 0, -1.0)             illusion break bottom
//   Origin = (-width_A/2, -(width_A+width_B)/2, height/2)
// ------------------------------------------------

// 5 bars = 5 cuboids
const int arch_vertsPerBar = 36;
const int arch_barCount = 5;
const int arch_totalVertices = arch_barCount * arch_vertsPerBar;

vec4 arch_positions[arch_barCount * arch_vertsPerBar];
vec4 arch_colors[arch_barCount * arch_vertsPerBar];

// rotation
float arch_angleX = 54.736f;
float arch_angleY = 0.0f;
float arch_angleZ = -45.0f;

// mouse
bool arch_isDragging = false;
double arch_mouseX = 0.0;
double arch_mouseY = 0.0;

// ------------------------------------------------
// face colors: purple tones
// ------------------------------------------------
vec4 arch_faceColor(int faceIndex)
{
    if (faceIndex == 0) return vec4(0.45f, 0.48f, 0.62f, 1.0f);
    if (faceIndex == 1) return vec4(0.38f, 0.40f, 0.55f, 1.0f);
    if (faceIndex == 2) return vec4(0.76f, 0.60f, 0.64f, 1.0f);
    if (faceIndex == 3) return vec4(0.35f, 0.37f, 0.52f, 1.0f);
    if (faceIndex == 4) return vec4(0.73f, 0.58f, 0.62f, 1.0f);
    return vec4(0.42f, 0.45f, 0.60f, 1.0f);
}

// ------------------------------------------------
// build one cuboid
// ------------------------------------------------
void arch_buildCube(vec4* pos, vec4* col)
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
        vec4 c = arch_faceColor(i);
        pos[idx] = v[faces[i][0]]; col[idx++] = c;
        pos[idx] = v[faces[i][1]]; col[idx++] = c;
        pos[idx] = v[faces[i][2]]; col[idx++] = c;
        pos[idx] = v[faces[i][0]]; col[idx++] = c;
        pos[idx] = v[faces[i][2]]; col[idx++] = c;
        pos[idx] = v[faces[i][3]]; col[idx++] = c;
    }
}

void arch_buildScene()
{
    for (int i = 0; i < arch_barCount; i++)
        arch_buildCube(arch_positions + i * arch_vertsPerBar,
                       arch_colors    + i * arch_vertsPerBar);
}

// ------------------------------------------------
// mouse + keyboard
// ------------------------------------------------
void arch_mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        arch_isDragging = (action == GLFW_PRESS);
        if (arch_isDragging)
            glfwGetCursorPos(window, &arch_mouseX, &arch_mouseY);
    }
}

void arch_cursorPosCallback(GLFWwindow*, double x, double y)
{
    if (!arch_isDragging) return;
    arch_angleY += (float)(x - arch_mouseX) * 0.4f;
    arch_angleX += (float)(y - arch_mouseY) * 0.4f;
    arch_mouseX = x;
    arch_mouseY = y;
}

void arch_scrollCallback(GLFWwindow*, double, double yoffset)
{
    arch_angleZ += (float)yoffset * 2.0f;
}

void arch_keyCallback(GLFWwindow* win, int key, int, int action, int)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_LEFT)  arch_angleY -= 3.0f;
        if (key == GLFW_KEY_RIGHT) arch_angleY += 3.0f;
        if (key == GLFW_KEY_UP)    arch_angleX -= 3.0f;
        if (key == GLFW_KEY_DOWN)  arch_angleX += 3.0f;

        if (key == GLFW_KEY_R)
        {
            arch_angleX = 54.736f;
            arch_angleY = 0.0f;
            arch_angleZ = -45.0f;
        }
        if (key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(win, GL_TRUE);
    }
}

// ------------------------------------------------
// init
// ------------------------------------------------
void arch_init()
{
    arch_buildScene();

    glGenVertexArrays(1, &arch_vao);
    glBindVertexArray(arch_vao);

    glGenBuffers(1, &arch_positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, arch_positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(arch_positions), arch_positions, GL_STATIC_DRAW);

    arch_shaderProgram = InitShader(SHADER_DIR "vshader_simple.glsl", SHADER_DIR "fshader_simple.glsl");
    glUseProgram(arch_shaderProgram);

    GLuint posLoc = glGetAttribLocation(arch_shaderProgram, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glGenBuffers(1, &arch_colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, arch_colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(arch_colors), arch_colors, GL_STATIC_DRAW);

    GLuint colLoc = glGetAttribLocation(arch_shaderProgram, "vColor");
    glEnableVertexAttribArray(colLoc);
    glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.72f, 0.75f, 0.72f, 1.0f);

    arch_modelPos      = glGetUniformLocation(arch_shaderProgram, "model");
    arch_viewPos        = glGetUniformLocation(arch_shaderProgram, "view");
    arch_projectionPos  = glGetUniformLocation(arch_shaderProgram, "projection");
}

// ------------------------------------------------
// display
//
// Arch geometry (height=2.0, width_A=1.0, width_B=1.0):
//   Origin = (-0.5, -1.0, 1.0)
//
//   Corner locations:
//     P0 = (-0.5, -1.0,  1.0)   origin
//     P1 = (-0.5, -1.0,  0.0)   after L0_ia
//     P2 = ( 0.5, -1.0,  0.0)   after L1 (+X)
//     P3 = ( 0.5,  0.0,  0.0)   after L2 (+Y)
//     P4 = ( 0.5,  0.0,  1.0)   after L3 (+Z, height-width_B=1.0)
//     P5 = (-1.5,  0.0,  1.0)   after L4 (-(wA+wB)=-2.0 in X)
//     P6 = (-1.5,  0.0,  0.0)   after L5_ib
//
// 5 bars:
//   Bar 1: P1->P2 along +X
//   Bar 2: P2->P3 along +Y
//   Bar 3: P3->P4 along +Z (vertical rise)
//   Bar 4: P4->P5 along -X (long top return)
//   Bar 5: illusion bar (P0->P1 + P5->P6)
// ------------------------------------------------
void arch_display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(arch_shaderProgram);
    glBindVertexArray(arch_vao);

    vec3 eye(3.0f, 3.0f, 3.0f);
    vec3 at(0.0f, 0.0f, 0.5f);
    vec3 up(0.0f, 1.0f, 0.0f);
    mat4 view = LookAt(eye, at, up);
    float aspect = 550.0f / 500.0f;
    float orthoS = 2.0f;
    mat4 projection = Ortho(-orthoS * aspect, orthoS * aspect, -orthoS, orthoS, 0.1f, 20.0f);

    glUniformMatrix4fv(arch_viewPos, 1, GL_FALSE, &view.d[0].x);
    glUniformMatrix4fv(arch_projectionPos, 1, GL_FALSE, &projection.d[0].x);

    mat4 sceneRot = RotateY(arch_angleY)
                  * RotateX(arch_angleX)
                  * RotateZ(arch_angleZ);

    float thickness = 0.2f;
    float width_A = 1.0f;
    float width_B = 1.0f;
    float height  = 2.0f;

    float t = thickness;

    // Bar 1: along +X (P1 to P2)
    // Extend both X-ends by t/2 so corners align with perpendicular bars
    mat4 model1 = sceneRot
                 * Translate(0.0f, -1.0f, 0.0f)
                 * Scale(width_A + t, t, t);
    glUniformMatrix4fv(arch_modelPos, 1, GL_FALSE, &model1.d[0].x);
    glDrawArrays(GL_TRIANGLES, 0, arch_vertsPerBar);

    // Bar 2: along +Y (P2 to P3)
    // Extend both Y-ends by t/2 so corners align with perpendicular bars
    mat4 model2 = sceneRot
                 * Translate(0.5f, -0.5f, 0.0f)
                 * Scale(t, width_B + t, t);
    glUniformMatrix4fv(arch_modelPos, 1, GL_FALSE, &model2.d[0].x);
    glDrawArrays(GL_TRIANGLES, arch_vertsPerBar, arch_vertsPerBar);

    // Bar 3: along +Z (P3 to P4)
    // Extend both Z-ends by t/2 so corners align with perpendicular bars
    mat4 model3 = sceneRot
                 * Translate(0.5f, 0.0f, 0.5f)
                 * Scale(t, t, (height - width_B) + t);
    glUniformMatrix4fv(arch_modelPos, 1, GL_FALSE, &model3.d[0].x);
    glDrawArrays(GL_TRIANGLES, 2 * arch_vertsPerBar, arch_vertsPerBar);

    // Bar 4: along -X (P4 to P5)
    // Extend both X-ends by t/2 so corners align with perpendicular bars
    mat4 model4 = sceneRot
                 * Translate(-0.5f, 0.0f, 1.0f)
                 * Scale((width_A + width_B) + t, t, t);
    glUniformMatrix4fv(arch_modelPos, 1, GL_FALSE, &model4.d[0].x);
    glDrawArrays(GL_TRIANGLES, 3 * arch_vertsPerBar, arch_vertsPerBar);

    // Bar 5a: illusion top half (P0 to P1)
    // Extend only the P1 (connecting) end by t/2; P0 is a free end
    // New Z range: -t/2 to 1.0, length = 1.0 + t/2, center shifts down
    mat4 model5a = sceneRot
                  * Translate(-0.5f, -1.0f, 0.5f - t * 0.25f)
                  * Scale(t, t, 1.0f + t * 0.5f);
    glUniformMatrix4fv(arch_modelPos, 1, GL_FALSE, &model5a.d[0].x);
    glDrawArrays(GL_TRIANGLES, 4 * arch_vertsPerBar, arch_vertsPerBar);

    // Bar 5b: illusion bottom half (P5 to P6)
    // Extend only the P5 (connecting) end by t/2; P6 is a free end
    // New Z range: 0.0 to 1.0 + t/2, length = 1.0 + t/2, center shifts up
    mat4 model5b = sceneRot
                  * Translate(-1.5f, 0.0f, 0.5f + t * 0.25f)
                  * Scale(t, t, 1.0f + t * 0.5f);
    glUniformMatrix4fv(arch_modelPos, 1, GL_FALSE, &model5b.d[0].x);
    glDrawArrays(GL_TRIANGLES, 4 * arch_vertsPerBar, arch_vertsPerBar);

    glFinish();
}
