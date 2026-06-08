#include "includes.h"

GLuint impossible_shaderProgram;
GLuint impossible_vao;
GLuint impossible_positionBuffer;
GLuint impossible_colorBuffer;
GLuint impossible_modelPos, impossible_viewPos, impossible_projectionPos;

const int imp_verticesPerCuboid = 36;
const int imp_numCuboids = 12;
const int imp_totalVertices = imp_verticesPerCuboid * imp_numCuboids;

vec4 imp_positions[imp_totalVertices];
vec4 imp_colors[imp_totalVertices];

float imp_angleX = 20.0f;
float imp_angleY = 30.0f;
float imp_angleZ = 0.0f;

float imp_targetX = 20.0f;
float imp_targetY = 30.0f;
float imp_targetZ = 0.0f;
bool  imp_snapping = false;

bool   imp_isDragging = false;
double imp_mouseX = 0.0, imp_mouseY = 0.0;

const float ILLUSION_X = 25.26f;
const float ILLUSION_Y = 45.0f;
const float ILLUSION_Z = 0.0f;

// ------------------------------------------------
// face colors
// ------------------------------------------------
vec4 imp_greenFace(int f) {
    if (f == 0) return vec4(0.55f, 0.82f, 0.71f, 1.0f);
    if (f == 2) return vec4(0.72f, 0.91f, 0.82f, 1.0f);
    return             vec4(0.38f, 0.65f, 0.55f, 1.0f);
}
vec4 imp_pinkFace(int f) {
    if (f == 0) return vec4(0.95f, 0.60f, 0.58f, 1.0f);
    if (f == 2) return vec4(0.98f, 0.75f, 0.73f, 1.0f);
    return             vec4(0.80f, 0.42f, 0.40f, 1.0f);
}

// ------------------------------------------------
// build one cuboid
// ------------------------------------------------
void imp_buildCuboid(vec4* pos, vec4* col, vec4(*faceColor)(int))
{
    vec4 v[8] = {
        {-0.5f,-0.5f, 0.5f,1.0f},{-0.5f, 0.5f, 0.5f,1.0f},
        { 0.5f, 0.5f, 0.5f,1.0f},{ 0.5f,-0.5f, 0.5f,1.0f},
        {-0.5f,-0.5f,-0.5f,1.0f},{-0.5f, 0.5f,-0.5f,1.0f},
        { 0.5f, 0.5f,-0.5f,1.0f},{ 0.5f,-0.5f,-0.5f,1.0f}
    };
    int faces[6][4] = {
        {0,1,2,3},{3,2,6,7},{0,1,5,4},{2,1,5,6},{4,5,6,7},{0,3,7,4}
    };
    int idx = 0;
    for (int i = 0; i < 6; i++) {
        vec4 c = faceColor(i);
        pos[idx] = v[faces[i][0]]; col[idx++] = c;
        pos[idx] = v[faces[i][1]]; col[idx++] = c;
        pos[idx] = v[faces[i][2]]; col[idx++] = c;
        pos[idx] = v[faces[i][0]]; col[idx++] = c;
        pos[idx] = v[faces[i][2]]; col[idx++] = c;
        pos[idx] = v[faces[i][3]]; col[idx++] = c;
    }
}

void imp_buildScene() {
    for (int i = 0; i < 6; i++)
        imp_buildCuboid(imp_positions + i * imp_verticesPerCuboid,
            imp_colors + i * imp_verticesPerCuboid, imp_greenFace);
    for (int i = 6; i < 12; i++)
        imp_buildCuboid(imp_positions + i * imp_verticesPerCuboid,
            imp_colors + i * imp_verticesPerCuboid, imp_pinkFace);
}

// ------------------------------------------------
// callbacks  (named exactly as main.cpp expects)
// ------------------------------------------------
void impossible_mouseButtonCallback(GLFWwindow* w, int btn, int action, int) {
    if (btn == GLFW_MOUSE_BUTTON_LEFT) {
        imp_isDragging = (action == GLFW_PRESS);
        if (imp_isDragging) {
            imp_snapping = false;
            glfwGetCursorPos(w, &imp_mouseX, &imp_mouseY);
        }
    }
}
void impossible_cursorPosCallback(GLFWwindow*, double x, double y) {
    if (!imp_isDragging) return;
    imp_angleY += (float)(x - imp_mouseX) * 0.4f;
    imp_angleX += (float)(y - imp_mouseY) * 0.4f;
    imp_targetX = imp_angleX;
    imp_targetY = imp_angleY;
    imp_mouseX = x;
    imp_mouseY = y;
}
void impossible_scrollCallback(GLFWwindow*, double, double yo) {
    imp_angleZ += (float)yo * 2.0f;
    imp_targetZ = imp_angleZ;
}
void impossible_keyCallback(GLFWwindow* win, int key, int, int action, int) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_LEFT) { imp_angleY -= 3.0f; imp_targetY = imp_angleY; }
        if (key == GLFW_KEY_RIGHT) { imp_angleY += 3.0f; imp_targetY = imp_angleY; }
        if (key == GLFW_KEY_UP) { imp_angleX -= 3.0f; imp_targetX = imp_angleX; }
        if (key == GLFW_KEY_DOWN) { imp_angleX += 3.0f; imp_targetX = imp_angleX; }
        if (key == GLFW_KEY_R) {
            imp_angleX = 20.0f; imp_angleY = 30.0f; imp_angleZ = 0.0f;
            imp_targetX = 20.0f; imp_targetY = 30.0f; imp_targetZ = 0.0f;
            imp_snapping = false;
        }
        // S: smooth snap to perfect illusion angle
        if (key == GLFW_KEY_S) {
            imp_targetX = ILLUSION_X;
            imp_targetY = ILLUSION_Y;
            imp_targetZ = ILLUSION_Z;
            imp_snapping = true;
        }
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, GL_TRUE);
    }
}

// ------------------------------------------------
// impossible_init
// ------------------------------------------------
void impossible_init() {
    imp_buildScene();

    glGenVertexArrays(1, &impossible_vao);
    glBindVertexArray(impossible_vao);

    glGenBuffers(1, &impossible_positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, impossible_positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(imp_positions), imp_positions, GL_STATIC_DRAW);

    impossible_shaderProgram = InitShader(SHADER_DIR "vshader_i.glsl", SHADER_DIR "fshader_i.glsl");
    glUseProgram(impossible_shaderProgram);

    GLuint posLoc = glGetAttribLocation(impossible_shaderProgram, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glGenBuffers(1, &impossible_colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, impossible_colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(imp_colors), imp_colors, GL_STATIC_DRAW);

    GLuint colLoc = glGetAttribLocation(impossible_shaderProgram, "vColor");
    glEnableVertexAttribArray(colLoc);
    glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);

    impossible_modelPos = glGetUniformLocation(impossible_shaderProgram, "model");
    impossible_viewPos = glGetUniformLocation(impossible_shaderProgram, "view");
    impossible_projectionPos = glGetUniformLocation(impossible_shaderProgram, "projection");
}

// ------------------------------------------------
// impossible_display
// ------------------------------------------------
void impossible_display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(impossible_shaderProgram);
    glBindVertexArray(impossible_vao);

    mat4 view = LookAt(vec3(3.0f, 3.0f, 3.0f),
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 1.0f, 0.0f));

    mat4 projection = Ortho(-2, 2, -2, 2, -10, 10);

    glUniformMatrix4fv(impossible_viewPos, 1, GL_FALSE, &view.d[0].x);
    glUniformMatrix4fv(impossible_projectionPos, 1, GL_FALSE, &projection.d[0].x);

    mat4 R = RotateY(imp_angleY) * RotateX(imp_angleX) * RotateZ(imp_angleZ);

    //float L = 1.2f;
    //float T = 0.18f;

    //// helper
    //auto draw = [&](mat4 m, int idx) {
    //    glUniformMatrix4fv(impossible_modelPos, 1, GL_FALSE, &m.d[0].x);
    //    glDrawArrays(GL_TRIANGLES, idx * imp_verticesPerCuboid, imp_verticesPerCuboid);
    //    };

    //// =========================
    //// FIRST CORNER (same as before)
    //// =========================
    //mat4 m1 = R * Translate(0, 0, 0) * Scale(L, T, T);
    //mat4 m2 = R * Translate(L * 0.5f - T * 0.5f, L * 0.5f, 0) * Scale(T, L, T);
    //mat4 m3 = R * Translate(L * 0.5f - T * 0.5f,
    //    L - T * 0.5f,
    //    -L * 0.5f + T * 0.5f) * Scale(T, T, L);

    //// =========================
    //// SECOND CORNER (continue loop)
    //// =========================

    //// goes left (top horizontal)
    //mat4 m4 = R * Translate(0.0f,
    //    L - T * 0.5f,
    //    -L + T) * Scale(L, T, T);

    //// goes down (left vertical)
    //mat4 m5 = R * Translate(-L * 0.5f + T * 0.5f,
    //    L * 0.5f,
    //    -L + T) * Scale(T, L, T);

    //// =========================
    //// THIRD CONNECTION (bottom depth)
    //// =========================
    //mat4 m6 = R * Translate(-L * 0.5f + T * 0.5f,
    //    0.0f,
    //    -L * 0.5f + T * 0.5f) * Scale(T, T, L);

    //// draw all
    //draw(m1, 0);
    //draw(m2, 1);
    //draw(m3, 2);
    //draw(m4, 3);
    //draw(m5, 4);
    //draw(m6, 5);

    float L = 1.2f;
    float T = 0.18f;

    auto draw = [&](mat4 m, int idx) {
        glUniformMatrix4fv(impossible_modelPos, 1, GL_FALSE, &m.d[0].x);
        glDrawArrays(GL_TRIANGLES, idx * imp_verticesPerCuboid, imp_verticesPerCuboid);
        };

    // =========================
    // ARM 1 (GREEN)
    // =========================
    mat4 a1_1 = R * Translate(0, 0, 0) * Scale(L, T, T);
    mat4 a1_2 = R * Translate(L / 2 - T / 2, L / 2, 0) * Scale(T, L, T);
    mat4 a1_3 = R * Translate(L / 2 - T / 2, L - T / 2, -L / 2) * Scale(T, T, L);

    // =========================
    // ARM 2 (PINK)
    // shifted + rotated manually
    // =========================
    mat4 a2_1 = R * Translate(0.0f, 0.0f, 0.0f)
        * RotateZ(90.0f)
        * Scale(L, T, T);

    mat4 a2_2 = R * Translate(0.0f, L / 2, -T)
        * RotateZ(90.0f)
        * Scale(T, L, T);

    mat4 a2_3 = R * Translate(0.0f, L, -L / 2)
        * RotateZ(90.0f)
        * Scale(T, T, L);

    // =========================
    // ARM 3 (GREEN)
    // =========================
    mat4 a3_1 = R * Translate(-T, 0.0f, 0.0f)
        * RotateY(90.0f)
        * Scale(L, T, T);

    mat4 a3_2 = R * Translate(-T, L / 2, 0.0f)
        * RotateY(90.0f)
        * Scale(T, L, T);

    mat4 a3_3 = R * Translate(-L / 2, L, 0.0f)
        * RotateY(90.0f)
        * Scale(T, T, L);

    // =========================
    // DRAW
    // =========================
    draw(a1_1, 0);
    draw(a1_2, 1);
    draw(a1_3, 2);

    draw(a2_1, 3);
    draw(a2_2, 4);
    draw(a2_3, 5);

    draw(a3_1, 6);
    draw(a3_2, 7);
    draw(a3_3, 8);

    glFinish();
}