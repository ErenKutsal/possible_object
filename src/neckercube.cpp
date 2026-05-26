#include "includes.h"

GLuint cube_shaderProgram;
GLuint cube_vao;
GLuint cube_positionBuffer;
GLuint cube_colorBuffer;
GLuint cube_modelPos;
GLuint cube_viewPos;
GLuint cube_projectionPos;

// Clean 12-bar cube frame
const int cube_barCount = 12;
const int cube_vertsPerBar = 36;

vec4 cube_positions[cube_barCount * cube_vertsPerBar];
vec4 cube_colors[cube_barCount * cube_vertsPerBar];

float cube_angleX = 35.264f;
float cube_angleY = 0.0f;
float cube_angleZ = -45.0f;

bool cube_isDragging = false;
double cube_mouseX = 0.0;
double cube_mouseY = 0.0;

// Camera globals — defaults set to canonical isometric view
// (θ_horizontal = 45°, θ_vertical ≈ 54.74°), so +X, +Y, +Z project to
// three screen-space vectors that sum to zero. This is the projection
// the Bridges paper requires for impossible figures.
float camera_radius_cube = 5.0f;
float camera_theta_cube = M_PI / 4.0f;             // 45° around Y
float camera_phi_cube = 0.9553166f;                // arccos(1/sqrt(3)) ≈ 54.74°

bool is_dragging_cube = false;
double last_mouse_x_cube = 0.0;
double last_mouse_y_cube = 0.0;

vec4 cubeBlue() { return vec4(0.67f, 0.75f, 0.92f, 1.0f); }
vec4 cubeBlueDark() { return vec4(0.50f, 0.56f, 0.75f, 1.0f); }
vec4 cubeCream() { return vec4(0.88f, 0.88f, 0.70f, 1.0f); }
vec4 cubeCreamDark() { return vec4(0.75f, 0.75f, 0.58f, 1.0f); }
vec4 cubeMauve() { return vec4(0.66f, 0.56f, 0.63f, 1.0f); }
vec4 cubeMauveDark() { return vec4(0.52f, 0.43f, 0.50f, 1.0f); }
vec4 cubeBlack() { return vec4(0.15f, 0.15f, 0.18f, 1.0f); }

void cube_buildBar(vec4* pos, vec4* col, vec4 colors[6])
{
    vec4 v[8] = {vec4(-0.5f, -0.5f, -0.5f, 1.0f), vec4(-0.5f, -0.5f, 0.5f, 1.0f), vec4(-0.5f, 0.5f, -0.5f, 1.0f),
                 vec4(-0.5f, 0.5f, 0.5f, 1.0f),   vec4(0.5f, -0.5f, -0.5f, 1.0f), vec4(0.5f, -0.5f, 0.5f, 1.0f),
                 vec4(0.5f, 0.5f, -0.5f, 1.0f),   vec4(0.5f, 0.5f, 0.5f, 1.0f)};

    int faces[6][4] = {{0, 1, 3, 2}, {4, 6, 7, 5}, {2, 3, 7, 6}, {0, 4, 5, 1}, {1, 5, 7, 3}, {0, 2, 6, 4}};

    int idx = 0;
    for (int i = 0; i < 6; i++)
    {
        vec4 c = colors[i];
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

void cube_buildScene()
{
    vec4 xColors[6] = {cubeBlue(), cubeMauve(), cubeCream(), cubeBlack(), cubeBlue(), cubeMauveDark()};
    vec4 yColors[6] = {cubeBlue(), cubeMauve(), cubeCreamDark(), cubeCreamDark(), cubeBlue(), cubeMauveDark()};
    vec4 zColors[6] = {cubeBlue(), cubeMauve(), cubeCream(), cubeBlack(), cubeBlueDark(), cubeMauveDark()};

    // Bars 0-1: bottom X, 2-3: bottom Z, 4-5: top X, 6-7: top Z, 8-11: verticals
    for (int i = 0; i < 2; i++)
        cube_buildBar(cube_positions + i * cube_vertsPerBar, cube_colors + i * cube_vertsPerBar, xColors);
    for (int i = 2; i < 4; i++)
        cube_buildBar(cube_positions + i * cube_vertsPerBar, cube_colors + i * cube_vertsPerBar, zColors);
    for (int i = 4; i < 6; i++)
        cube_buildBar(cube_positions + i * cube_vertsPerBar, cube_colors + i * cube_vertsPerBar, xColors);
    for (int i = 6; i < 8; i++)
        cube_buildBar(cube_positions + i * cube_vertsPerBar, cube_colors + i * cube_vertsPerBar, zColors);
    for (int i = 8; i < 12; i++)
        cube_buildBar(cube_positions + i * cube_vertsPerBar, cube_colors + i * cube_vertsPerBar, yColors);
}

void cube_mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    /*
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        cube_isDragging = (action == GLFW_PRESS);
        if (cube_isDragging) glfwGetCursorPos(window, &cube_mouseX, &cube_mouseY);
    }
    */
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            is_dragging_cube = true;
            glfwGetCursorPos(window, &last_mouse_x_cube, &last_mouse_y_cube);
        }
        else if (action == GLFW_RELEASE)
        {
            is_dragging_cube = false;
        }
    }
}

void cube_cursorPosCallback(GLFWwindow*, double x, double y)
{
    /*
    if (!cube_isDragging) return;
    cube_angleY += (float)(x - cube_mouseX) * 0.4f;
    cube_angleX += (float)(y - cube_mouseY) * 0.4f;
    cube_mouseX = x;
    cube_mouseY = y;
    */
    if (is_dragging_cube)
    {
        double deltaX = x - last_mouse_x_cube;
        double deltaY = y - last_mouse_y_cube;

        last_mouse_x_cube = x;
        last_mouse_y_cube = y;

        camera_theta_cube -= deltaX * 0.01f;
        camera_phi_cube += deltaY * 0.01f;

        if (camera_phi_cube < 0.01f) camera_phi_cube = 0.01f;
        if (camera_phi_cube > M_PI - 0.01f) camera_phi_cube = M_PI - 0.01f;
    }
}

void cube_scrollCallback(GLFWwindow*, double, double yoffset) { cube_angleZ += (float)yoffset * 2.0f; }

void cube_keyCallback(GLFWwindow* win, int key, int, int action, int)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_LEFT) cube_angleY -= 3.0f;
        if (key == GLFW_KEY_RIGHT) cube_angleY += 3.0f;
        if (key == GLFW_KEY_UP) cube_angleX -= 3.0f;
        if (key == GLFW_KEY_DOWN) cube_angleX += 3.0f;
        if (key == GLFW_KEY_R)
        {
            cube_angleX = 35.264f;
            cube_angleY = 0.0f;
            cube_angleZ = -45.0f;
        }
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, GL_TRUE);
    }
}

void cube_init()
{
    cube_buildScene();

    glGenVertexArrays(1, &cube_vao);
    glBindVertexArray(cube_vao);

    glGenBuffers(1, &cube_positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, cube_positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, cube_barCount * cube_vertsPerBar * sizeof(vec4), cube_positions, GL_STATIC_DRAW);

    cube_shaderProgram = InitShader("../shaders/vshader_impossible.glsl", "../shaders/fshader_impossible.glsl");
    glUseProgram(cube_shaderProgram);

    GLuint posLoc = glGetAttribLocation(cube_shaderProgram, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glGenBuffers(1, &cube_colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, cube_colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, cube_barCount * cube_vertsPerBar * sizeof(vec4), cube_colors, GL_STATIC_DRAW);

    GLuint colLoc = glGetAttribLocation(cube_shaderProgram, "vColor");
    glEnableVertexAttribArray(colLoc);
    glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glEnable(GL_DEPTH_TEST);

    cube_modelPos = glGetUniformLocation(cube_shaderProgram, "model");
    cube_viewPos = glGetUniformLocation(cube_shaderProgram, "view");
    cube_projectionPos = glGetUniformLocation(cube_shaderProgram, "projection");
}

static void cube_drawBar(int barIdx, mat4 m, GLuint modelLoc)
{
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &m.d[0].x);
    glDrawArrays(GL_TRIANGLES, barIdx * cube_vertsPerBar, cube_vertsPerBar);
}

void cube_display()
{
    float eye_x = camera_radius_cube * sinf(camera_phi_cube) * cosf(camera_theta_cube);
    float eye_y = camera_radius_cube * cosf(camera_phi_cube);
    float eye_z = camera_radius_cube * sinf(camera_phi_cube) * sinf(camera_theta_cube);

    vec3 eye(eye_x, eye_y, eye_z);
    vec3 at(0.0f, 0.0f, 0.0f);
    vec3 up(0.0f, 1.0f, 0.0f);

    glUseProgram(cube_shaderProgram);
    glBindVertexArray(cube_vao);

    mat4 view = LookAt(eye, at, up);
    // Orthographic projection: required for the impossible-cube
    // illusion. Under parallel projection, +X, +Y, +Z project to
    // three coplanar vectors that sum to zero, so the back corner
    // (-h,-h,-h) and front corner (h,h,h) land on the SAME screen
    // pixel — only depth distinguishes them. That is what lets us
    // swap them below.
    float aspect = (float)screen_w / (float)screen_h;
    float orthoS = 1.2f;
    mat4 projection = Ortho(-orthoS * aspect, orthoS * aspect, -orthoS, orthoS, 0.1f, 50.0f);

    glUniformMatrix4fv(cube_viewPos, 1, GL_FALSE, &view.d[0].x);
    glUniformMatrix4fv(cube_projectionPos, 1, GL_FALSE, &projection.d[0].x);

    // --- 4 new uniforms, looked up inline, no new globals ---
    GLuint p = cube_shaderProgram;
    vec3 lightPos(3.0f, 5.0f, 3.0f);
    glUniform3fv(glGetUniformLocation(p, "uLightPos"), 1, &lightPos.x);
    glUniform3fv(glGetUniformLocation(p, "uEyePos"), 1, &eye.x);
    glUniform1f(glGetUniformLocation(p, "uTime"), glfwGetTime());
    // uObjHeight = total world-space height of cube = S + T
    glUniform1f(glGetUniformLocation(p, "uObjHeight"), 0.8f + 0.14f);

    mat4 sceneRot = RotateY(cube_angleY) * RotateX(cube_angleX) * RotateZ(cube_angleZ);

    float S = 0.8f;
    float T = 0.14f;
    float h = S / 2.0f;

    // Impossible-cube swap: translate the 3 bars meeting at the
    // BACK corner (-h,-h,-h) toward the camera along (1,1,1)/sqrt(3)
    // by an amount > cube diagonal. Under orthographic projection
    // this changes only depth, not screen position — so those bars
    // are drawn IN FRONT of the 3 front-corner bars at the center
    // vertex, producing the Necker-reversed impossible reading.
    float k = 1.6f;                                // shift magnitude
    float s = k / 1.7320508f;                      // k / sqrt(3)
    mat4 swap = Translate(s, s, s);                // along +(1,1,1)

    int idx = 0;

    // Bottom face: 0=back-X (back-corner bar), 1=front-X, 2=back-Z (back-corner bar), 3=front-Z
    cube_drawBar(idx++, sceneRot * swap * Translate(0, -h, -h) * Scale(S + T, T, T), cube_modelPos);  // 0: back-corner
    cube_drawBar(idx++, sceneRot *        Translate(0, -h,  h) * Scale(S + T, T, T), cube_modelPos);  // 1
    cube_drawBar(idx++, sceneRot * swap * Translate(-h, -h, 0) * Scale(T, T, S + T), cube_modelPos);  // 2: back-corner
    cube_drawBar(idx++, sceneRot *        Translate( h, -h, 0) * Scale(T, T, S + T), cube_modelPos);  // 3

    // Top face: 4=back-X, 5=front-X (front-corner), 6=back-Z, 7=front-Z (front-corner)
    cube_drawBar(idx++, sceneRot *        Translate(0,  h, -h) * Scale(S + T, T, T), cube_modelPos);  // 4
    cube_drawBar(idx++, sceneRot *        Translate(0,  h,  h) * Scale(S + T, T, T), cube_modelPos);  // 5: front-corner
    cube_drawBar(idx++, sceneRot *        Translate(-h, h, 0) * Scale(T, T, S + T), cube_modelPos);   // 6
    cube_drawBar(idx++, sceneRot *        Translate( h, h, 0) * Scale(T, T, S + T), cube_modelPos);   // 7: front-corner

    // Verticals: 8=back-corner, 9=front-corner, 10/11=mid
    cube_drawBar(idx++, sceneRot * swap * Translate(-h, 0, -h) * Scale(T, S + T, T), cube_modelPos);  // 8: back-corner
    cube_drawBar(idx++, sceneRot *        Translate( h, 0,  h) * Scale(T, S + T, T), cube_modelPos);  // 9: front-corner
    cube_drawBar(idx++, sceneRot *        Translate( h, 0, -h) * Scale(T, S + T, T), cube_modelPos);  // 10
    cube_drawBar(idx++, sceneRot *        Translate(-h, 0,  h) * Scale(T, S + T, T), cube_modelPos);  // 11

    glFinish();
}
