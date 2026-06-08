#include "penrose_mirror.h"
#include <vector>
#include <cmath>


int penrose_ballStyle = 5;   // 0=purple glass, 5=rainbow

GLuint penrose_mirrorStyleLoc;
GLuint penrose_mirrorTimeLoc;

// ============================================================
// Shared geometry from penrose_1.cpp
// ============================================================
extern GLuint penrose_shaderProgram;
extern GLuint penrose_vao;
extern GLuint penrose_modelPos;
extern GLuint penrose_viewPos;
extern GLuint penrose_projectionPos;

// Use the original scene angles so the mirror version matches
extern float penrose_angleX;
extern float penrose_angleY;
extern float penrose_angleZ;

const int verticesPerCuboid = 36;

// ============================================================
// ball GPU objects
// ============================================================
GLuint penrose_sphereVAO;
GLuint penrose_spherePositionBuffer;
GLuint penrose_sphereColorBuffer;
GLsizei penrose_sphereVertexCount = 0;

GLuint penrose_mirrorProgram;
GLuint penrose_mirrorModelPos;
GLuint penrose_mirrorViewPos;
GLuint penrose_mirrorProjectionPos;
GLuint penrose_mirrorCameraPosLoc;

GLuint splitVAO = 0;
GLuint splitPosBuffer = 0;
GLuint splitColorBuffer = 0;
GLsizei splitVertexCount = 0;

std::vector<vec4> spherePositions;
std::vector<vec4> sphereColors;

// ============================================================
// Ball animation 
// ============================================================
static bool   pm_isDragging = false;
static double pm_mouseX = 0.0;
static double pm_mouseY = 0.0;

static float pm_t = 0.0f;   // path parameter [0, 1)

// Ball settings
static const float BALL_RADIUS = 0.05f;  
static const float BALL_SPEED = 0.00025f;

// ============================================================
// Build a UV sphere
// ============================================================
void buildSphere(float radius, int stacks, int slices, const vec4& color)
{
    spherePositions.clear();
    sphereColors.clear();

    const float PI = 3.14159265358979323846f;

    for (int i = 0; i < stacks; ++i)
    {
        float phi0 = PI * i / (float)stacks;
        float phi1 = PI * (i + 1) / (float)stacks;

        for (int j = 0; j < slices; ++j)
        {
            float theta0 = 2.0f * PI * j / (float)slices;
            float theta1 = 2.0f * PI * (j + 1) / (float)slices;

            vec4 p0(radius * sinf(phi0) * cosf(theta0), radius * cosf(phi0), radius * sinf(phi0) * sinf(theta0), 1.0f);
            vec4 p1(radius * sinf(phi1) * cosf(theta0), radius * cosf(phi1), radius * sinf(phi1) * sinf(theta0), 1.0f);
            vec4 p2(radius * sinf(phi1) * cosf(theta1), radius * cosf(phi1), radius * sinf(phi1) * sinf(theta1), 1.0f);
            vec4 p3(radius * sinf(phi0) * cosf(theta1), radius * cosf(phi0), radius * sinf(phi0) * sinf(theta1), 1.0f);

            spherePositions.push_back(p0); spherePositions.push_back(p1); spherePositions.push_back(p2);
            spherePositions.push_back(p0); spherePositions.push_back(p2); spherePositions.push_back(p3);

            for (int k = 0; k < 6; ++k) sphereColors.push_back(color);
        }
    }

    penrose_sphereVertexCount = (GLsizei)spherePositions.size();
}

// ============================================================
// Split-square geometry
// ============================================================
static void buildSplitCornerFace()
{
    const float s = 0.125f;   // half-size (matches T/2 when T = 0.25)
    const float eps = 0.0005f;

    std::vector<vec4> positions;
    std::vector<vec4> colors;

    vec4 A(-s, eps, -s, 1.0f);
    vec4 B(s, eps, -s, 1.0f);
    vec4 C(s, eps, s, 1.0f);
    vec4 D(-s, eps, s, 1.0f);

    //vec4 color1(0.20f, 0.25f, 0.38f, 1.0f);
    //vec4 color2(0.38f, 0.45f, 0.58f, 1.0f);

    vec4 color1(0.15f, 0.42f, 0.72f, 1.0f);
    vec4 color2(0.45f, 0.78f, 0.95f, 1.0f);

    // Triangle 1
    positions.push_back(A);
    positions.push_back(B);
    positions.push_back(C);
    colors.push_back(color1);
    colors.push_back(color1);
    colors.push_back(color1);

    // Triangle 2
    positions.push_back(A);
    positions.push_back(C);
    positions.push_back(D);
    colors.push_back(color2);
    colors.push_back(color2);
    colors.push_back(color2);

    splitVertexCount = (GLsizei)positions.size();

    glGenVertexArrays(1, &splitVAO);
    glBindVertexArray(splitVAO);

    glGenBuffers(1, &splitPosBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, splitPosBuffer);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(vec4), positions.data(), GL_STATIC_DRAW);

    glUseProgram(penrose_shaderProgram);
    GLuint posLoc = glGetAttribLocation(penrose_shaderProgram, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glGenBuffers(1, &splitColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, splitColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(vec4), colors.data(), GL_STATIC_DRAW);

    GLuint colLoc = glGetAttribLocation(penrose_shaderProgram, "vColor");
    glEnableVertexAttribArray(colLoc);
    glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glBindVertexArray(0);
}

// ============================================================
// Ball path configuration
// ============================================================
struct BallPathSegment {
    vec3 start;
    vec3 end;
    int renderPass; // 1 , 2
};

static const int NUM_SEGMENTS = 10;
static BallPathSegment pathSegments[NUM_SEGMENTS];
static float pathLen[NUM_SEGMENTS];
static float pathTotal = 0.0f;
static bool  pathReady = false;

static void buildBallPath()
{
    if (pathReady) return;

    const float L = 1.0f;
    const float T = 0.25f;
    const float H = T * 0.5f;
    const float R = BALL_RADIUS;

    // Coordinate Boundaries
    const float y_start = (L + H) + R;
    const float y_end = -5.0f;
    const float x_start = 0.0f;
    const float x_end = 1.0f;
    const float z_start = 0.0f;
    const float z_end = 1.125f;

    const float offset = 0.175f;
    const float y_bot = L - offset;

    // Path 1: 
    pathSegments[0] = { vec3(x_start, y_start, z_start), vec3(x_end, y_start, z_start), 1 };

    // Path 2: 
    pathSegments[1] = { vec3(x_end, y_start, z_start), vec3(x_end, y_start, z_end), 1 };

    // Path 3: 
    pathSegments[2] = {};

    // Path 4:
    pathSegments[3] = {};

    // Path 5: 
    pathSegments[4] = { vec3(x_end + T * 0.5f + R, L, z_start), vec3(x_end + T * 0.5f + R, L, z_end), 1 };

    // Path 6: 
    pathSegments[5] = { vec3(0.15f, z_start, 0.0f), vec3(0.15f, 0.65f, 0.0f), 2 };

    // Path 7: 
    pathSegments[6] = {};

    // Path 8: 
    pathSegments[7] = {};

    // Path 9: 
    pathSegments[8] = { vec3(x_start, 0.0f, T * 0.5f + R), vec3(x_start, L, T * 0.5f + R), 2 };
    
    // Path 10: 
    pathSegments[9] = { vec3(x_start, L, T * 0.5f + R), vec3(x_end, L, T * 0.5f + R), 1 };

    // Compute total sequence distance
    pathTotal = 0.0f;
    for (int i = 0; i < NUM_SEGMENTS; ++i)
    {
        vec3 d = pathSegments[i].end - pathSegments[i].start;
        pathLen[i] = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
        pathTotal += pathLen[i];
    }

    pathReady = true;
}

static vec3 sampleBallPath(float t, int& passOut)
{
    if (!pathReady) buildBallPath();

    if (pathTotal <= 0.0f)
    {
        passOut = 1;
        return pathSegments[0].start;
    }

    float dist = t * pathTotal;
    float accum = 0.0f;
    int seg = 0;

    for (int i = 0; i < NUM_SEGMENTS; ++i)
    {
        if (dist <= accum + pathLen[i])
        {
            seg = i;
            break;
        }
        accum += pathLen[i];
    }

    float localT = 0.0f;
    if (pathLen[seg] > 1e-6f)
        localT = (dist - accum) / pathLen[seg];

    passOut = pathSegments[seg].renderPass;
    return pathSegments[seg].start + (pathSegments[seg].end - pathSegments[seg].start) * localT;
}

// ============================================================
// Initialize scene geometry
// ============================================================
void penrose_m_init()
{
    penrose_mirrorProgram = InitShader(SHADER_DIR "core/vshader_mirror.glsl", SHADER_DIR "core/fshader_mirror.glsl");

    penrose_mirrorStyleLoc = glGetUniformLocation(penrose_mirrorProgram, "u_ballStyle");
    penrose_mirrorTimeLoc = glGetUniformLocation(penrose_mirrorProgram, "u_time");

    penrose_mirrorModelPos = glGetUniformLocation(penrose_mirrorProgram, "model");
    penrose_mirrorViewPos = glGetUniformLocation(penrose_mirrorProgram, "view");
    penrose_mirrorProjectionPos = glGetUniformLocation(penrose_mirrorProgram, "projection");
    penrose_mirrorCameraPosLoc = glGetUniformLocation(penrose_mirrorProgram, "cameraPos");

    //buildSphere(1.0f, 24, 24, vec4(0.95f, 0.70f, 0.20f, 1.0f));
    buildSphere(1.0f, 48, 48, vec4(0.95f, 0.70f, 0.20f, 1.0f));

    glGenVertexArrays(1, &penrose_sphereVAO);
    glBindVertexArray(penrose_sphereVAO);

    glGenBuffers(1, &penrose_spherePositionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, penrose_spherePositionBuffer);
    glBufferData(GL_ARRAY_BUFFER, spherePositions.size() * sizeof(vec4), spherePositions.data(), GL_STATIC_DRAW);

    glUseProgram(penrose_mirrorProgram);
    GLuint posLoc = glGetAttribLocation(penrose_mirrorProgram, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glGenBuffers(1, &penrose_sphereColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, penrose_sphereColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sphereColors.size() * sizeof(vec4), sphereColors.data(), GL_STATIC_DRAW);

    GLuint colLoc = glGetAttribLocation(penrose_mirrorProgram, "vColor");
    glEnableVertexAttribArray(colLoc);
    glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    buildSplitCornerFace();

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
}

// ============================================================
// Display Loop Pass
// ============================================================
void penrose_m_display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vec3 eye(3.0f, 3.0f, 3.0f);
    vec3 at(0.0f, 0.0f, 0.0f);
    vec3 up(0.0f, 1.0f, 0.0f);

    mat4 view = LookAt(eye, at, up);
    mat4 projection = Ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 20.0f);

    glUseProgram(penrose_shaderProgram);
    glBindVertexArray(penrose_vao);
    glUniformMatrix4fv(penrose_viewPos, 1, GL_FALSE, &view.d[0].x);
    glUniformMatrix4fv(penrose_projectionPos, 1, GL_FALSE, &projection.d[0].x);

    mat4 R = RotateY(penrose_angleY) * RotateX(penrose_angleX) * RotateZ(penrose_angleZ);

    const float L = 1.0f;
    const float T = 0.25f;
    mat4 CO = Translate(-L * 0.5f, -L * 0.5f, -L * 0.15f);

    // Progress timeline variable
    pm_t += BALL_SPEED;
    if (pm_t >= 1.0f) pm_t -= 1.0f;

    int currentBallPass = 1;
    vec3 ballPos = sampleBallPath(pm_t, currentBallPass);

    GLint overrideLoc = glGetUniformLocation(penrose_shaderProgram, "u_overrideColor");
    if (overrideLoc != -1) glUniform1i(overrideLoc, 0);

    // ============================================================
    // PASS 1: Draw Base 3D Shape & Background Path 
    // ============================================================

    // arm1 
    mat4 m1 = R * CO * Translate(L * 0.5f, L, 0.0f) * Scale(L + T, T, T);
    glUniformMatrix4fv(penrose_modelPos, 1, GL_FALSE, &m1.d[0].x);
    glDrawArrays(GL_TRIANGLES, 0, verticesPerCuboid);

    // arm2 
    mat4 m2 = R * CO * Translate(0.0f, L * 0.5f, 0.0f) * Scale(T, L, T);
    glUniformMatrix4fv(penrose_modelPos, 1, GL_FALSE, &m2.d[0].x);
    glDrawArrays(GL_TRIANGLES, verticesPerCuboid, verticesPerCuboid);

    // arm3 
    mat4 m3 = R * CO * Translate(L, L, L * 0.5f + T * 0.5f) * Scale(T, T, L);
    glUniformMatrix4fv(penrose_modelPos, 1, GL_FALSE, &m3.d[0].x);
    glDrawArrays(GL_TRIANGLES, 2 * verticesPerCuboid, verticesPerCuboid);

    // Render ball 
    if (currentBallPass == 1)
    {
        glUseProgram(penrose_mirrorProgram);
        glUniform1i(penrose_mirrorStyleLoc, penrose_ballStyle);
        glUniform1f(penrose_mirrorTimeLoc, (float)glfwGetTime());
        glBindVertexArray(penrose_sphereVAO);
        glUniformMatrix4fv(penrose_mirrorViewPos, 1, GL_FALSE, &view.d[0].x);
        glUniformMatrix4fv(penrose_mirrorProjectionPos, 1, GL_FALSE, &projection.d[0].x);
        glUniform3f(penrose_mirrorCameraPosLoc, eye.x, eye.y, eye.z);

        mat4 ballModel = R * CO * Translate(ballPos.x, ballPos.y, ballPos.z) * Scale(BALL_RADIUS, BALL_RADIUS, BALL_RADIUS);
        glUniformMatrix4fv(penrose_mirrorModelPos, 1, GL_FALSE, &ballModel.d[0].x);
        glDrawArrays(GL_TRIANGLES, 0, penrose_sphereVertexCount);
    }

    // ============================================================
    // PASS2: 
    // ============================================================
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(penrose_shaderProgram);
    glBindVertexArray(penrose_vao);
    glUniformMatrix4fv(penrose_viewPos, 1, GL_FALSE, &view.d[0].x);
    glUniformMatrix4fv(penrose_projectionPos, 1, GL_FALSE, &projection.d[0].x);

    float armUpDistance = 0.125f;
    mat4 m2_patch = R * CO * Translate(0.0f, L * 0.25f + armUpDistance, 0.0f) * Scale(T, L * 0.5f, T);
    glUniformMatrix4fv(penrose_modelPos, 1, GL_FALSE, &m2_patch.d[0].x);

    if (overrideLoc != -1) glUniform1i(overrideLoc, 1);
    glDrawArrays(GL_TRIANGLES, verticesPerCuboid, verticesPerCuboid);
    if (overrideLoc != -1) glUniform1i(overrideLoc, 0);

    // Draw the split color patch
    glBindVertexArray(splitVAO);
    mat4 splitModel = R * CO * Translate(0.0f, L * 0.4996f + armUpDistance, 0.0f);
    glUniformMatrix4fv(penrose_modelPos, 1, GL_FALSE, &splitModel.d[0].x);
    glDrawArrays(GL_TRIANGLES, 0, splitVertexCount);

    glBindVertexArray(penrose_vao);

    // Render ball
    if (currentBallPass == 2)
    {
        glUseProgram(penrose_mirrorProgram);
        glUniform1i(penrose_mirrorStyleLoc, penrose_ballStyle);
        glUniform1f(penrose_mirrorTimeLoc, (float)glfwGetTime());
        glBindVertexArray(penrose_sphereVAO);
        glUniformMatrix4fv(penrose_mirrorViewPos, 1, GL_FALSE, &view.d[0].x);
        glUniformMatrix4fv(penrose_mirrorProjectionPos, 1, GL_FALSE, &projection.d[0].x);
        glUniform3f(penrose_mirrorCameraPosLoc, eye.x, eye.y, eye.z);

        mat4 ballModel = R * CO * Translate(ballPos.x, ballPos.y, ballPos.z) * Scale(BALL_RADIUS, BALL_RADIUS, BALL_RADIUS);
        glUniformMatrix4fv(penrose_mirrorModelPos, 1, GL_FALSE, &ballModel.d[0].x);
        glDrawArrays(GL_TRIANGLES, 0, penrose_sphereVertexCount);
    }

    glFinish();
}

// ============================================================
// Input Callbacks
// ============================================================
void penrose_m_keyCallback(GLFWwindow* win, int key, int, int action, int)
{
    //if (action == GLFW_PRESS)
    //{
    //    if (key == GLFW_KEY_1) penrose_ballStyle = 0; // purple glass
    //    if (key == GLFW_KEY_2) penrose_ballStyle = 1; // dark glossy
    //    if (key == GLFW_KEY_3) penrose_ballStyle = 2; // chrome
    //    if (key == GLFW_KEY_4) penrose_ballStyle = 3; // clear glass
    //    if (key == GLFW_KEY_0) penrose_ballStyle = (penrose_ballStyle + 1) % 4;
    //    if (key == GLFW_KEY_5) penrose_ballStyle = 5; // rainbow
    //    if (key == GLFW_KEY_9) penrose_ballStyle = 9; // original
    //}

    //if (action == GLFW_PRESS || action == GLFW_REPEAT)
    //{
    //    if (key == GLFW_KEY_LEFT)  penrose_angleY -= 3.0f;
    //    if (key == GLFW_KEY_RIGHT) penrose_angleY += 3.0f;
    //    if (key == GLFW_KEY_UP)    penrose_angleX -= 3.0f;
    //    if (key == GLFW_KEY_DOWN)  penrose_angleX += 3.0f;

    //    if (key == GLFW_KEY_R)
    //    {
    //        penrose_angleX = 0.0f;
    //        penrose_angleY = 0.0f;
    //        penrose_angleZ = 0.0f;
    //    }

    //    if (key == GLFW_KEY_ESCAPE)
    //        glfwSetWindowShouldClose(win, GL_TRUE);
    //}
}

void penrose_m_mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    //if (button == GLFW_MOUSE_BUTTON_LEFT)
    //{
    //    pm_isDragging = (action == GLFW_PRESS);
    //    if (pm_isDragging)
    //        glfwGetCursorPos(window, &pm_mouseX, &pm_mouseY);
    //}
}

void penrose_m_cursorPosCallback(GLFWwindow*, double x, double y)
{
    //if (!pm_isDragging) return;

    //penrose_angleY += (float)(x - pm_mouseX) * 0.4f;
    //penrose_angleX += (float)(y - pm_mouseY) * 0.4f;

    //pm_mouseX = x;
    //pm_mouseY = y;
}

void penrose_m_scrollCallback(GLFWwindow*, double, double yoffset)
{
    //penrose_angleZ += (float)yoffset * 2.0f;
}
