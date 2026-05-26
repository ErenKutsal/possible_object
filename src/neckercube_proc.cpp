#include "includes.h"

GLuint nckp_shaderProgram;
GLuint nckp_vao;
GLuint nckp_positionBuffer;
GLuint nckp_colorBuffer;
GLuint nckp_modelPos;
GLuint nckp_viewPos;
GLuint nckp_projectionPos;

// Clean 12-bar cube frame
const int nckp_barCount = 12;
const int nckp_vertsPerBar = 36;

vec4 nckp_positions[nckp_barCount * nckp_vertsPerBar];
vec4 nckp_colors[nckp_barCount * nckp_vertsPerBar];

float nckp_angleX = 35.264f;
float nckp_angleY = 0.0f;
float nckp_angleZ = -45.0f;

bool nckp_isDragging = false;
double nckp_mouseX = 0.0;
double nckp_mouseY = 0.0;

// Camera globals — defaults set to canonical isometric view
// (θ_horizontal = 45°, θ_vertical ≈ 54.74°), so +X, +Y, +Z project to
// three screen-space vectors that sum to zero. This is the projection
// the Bridges paper requires for impossible figures.
float camera_radius_nckp = 5.0f;
float camera_theta_nckp = M_PI / 4.0f;             // 45° around Y
float camera_phi_nckp = 0.9553166f;                // arccos(1/sqrt(3)) ≈ 54.74°

bool is_dragging_nckp = false;
double last_mouse_x_nckp = 0.0;
double last_mouse_y_nckp = 0.0;

vec4 nckpBlue() { return vec4(0.67f, 0.75f, 0.92f, 1.0f); }
vec4 nckpBlueDark() { return vec4(0.50f, 0.56f, 0.75f, 1.0f); }
vec4 nckpCream() { return vec4(0.88f, 0.88f, 0.70f, 1.0f); }
vec4 nckpCreamDark() { return vec4(0.75f, 0.75f, 0.58f, 1.0f); }
vec4 nckpMauve() { return vec4(0.66f, 0.56f, 0.63f, 1.0f); }
vec4 nckpMauveDark() { return vec4(0.52f, 0.43f, 0.50f, 1.0f); }
vec4 nckpBlack() { return vec4(0.15f, 0.15f, 0.18f, 1.0f); }

void nckp_buildBar(vec4* pos, vec4* col, vec4 colors[6])
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

void nckp_buildScene()
{
    vec4 xColors[6] = {nckpBlue(), nckpMauve(), nckpCream(), nckpBlack(), nckpBlue(), nckpMauveDark()};
    vec4 yColors[6] = {nckpBlue(), nckpMauve(), nckpCreamDark(), nckpCreamDark(), nckpBlue(), nckpMauveDark()};
    vec4 zColors[6] = {nckpBlue(), nckpMauve(), nckpCream(), nckpBlack(), nckpBlueDark(), nckpMauveDark()};

    // Bars 0-1: bottom X, 2-3: bottom Z, 4-5: top X, 6-7: top Z, 8-11: verticals
    for (int i = 0; i < 2; i++)
        nckp_buildBar(nckp_positions + i * nckp_vertsPerBar, nckp_colors + i * nckp_vertsPerBar, xColors);
    for (int i = 2; i < 4; i++)
        nckp_buildBar(nckp_positions + i * nckp_vertsPerBar, nckp_colors + i * nckp_vertsPerBar, zColors);
    for (int i = 4; i < 6; i++)
        nckp_buildBar(nckp_positions + i * nckp_vertsPerBar, nckp_colors + i * nckp_vertsPerBar, xColors);
    for (int i = 6; i < 8; i++)
        nckp_buildBar(nckp_positions + i * nckp_vertsPerBar, nckp_colors + i * nckp_vertsPerBar, zColors);
    for (int i = 8; i < 12; i++)
        nckp_buildBar(nckp_positions + i * nckp_vertsPerBar, nckp_colors + i * nckp_vertsPerBar, yColors);
}

void nckp_mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    /*
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        nckp_isDragging = (action == GLFW_PRESS);
        if (nckp_isDragging) glfwGetCursorPos(window, &nckp_mouseX, &nckp_mouseY);
    }
    */
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            is_dragging_nckp = true;
            glfwGetCursorPos(window, &last_mouse_x_nckp, &last_mouse_y_nckp);
        }
        else if (action == GLFW_RELEASE)
        {
            is_dragging_nckp = false;
        }
    }
}

void nckp_cursorPosCallback(GLFWwindow*, double x, double y)
{
    /*
    if (!nckp_isDragging) return;
    nckp_angleY += (float)(x - nckp_mouseX) * 0.4f;
    nckp_angleX += (float)(y - nckp_mouseY) * 0.4f;
    nckp_mouseX = x;
    nckp_mouseY = y;
    */
    if (is_dragging_nckp)
    {
        double deltaX = x - last_mouse_x_nckp;
        double deltaY = y - last_mouse_y_nckp;

        last_mouse_x_nckp = x;
        last_mouse_y_nckp = y;

        camera_theta_nckp -= deltaX * 0.01f;
        camera_phi_nckp += deltaY * 0.01f;

        if (camera_phi_nckp < 0.01f) camera_phi_nckp = 0.01f;
        if (camera_phi_nckp > M_PI - 0.01f) camera_phi_nckp = M_PI - 0.01f;
    }
}

void nckp_scrollCallback(GLFWwindow*, double, double yoffset) { nckp_angleZ += (float)yoffset * 2.0f; }

void nckp_keyCallback(GLFWwindow* win, int key, int, int action, int)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_LEFT) nckp_angleY -= 3.0f;
        if (key == GLFW_KEY_RIGHT) nckp_angleY += 3.0f;
        if (key == GLFW_KEY_UP) nckp_angleX -= 3.0f;
        if (key == GLFW_KEY_DOWN) nckp_angleX += 3.0f;
        if (key == GLFW_KEY_R)
        {
            nckp_angleX = 35.264f;
            nckp_angleY = 0.0f;
            nckp_angleZ = -45.0f;
        }
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, GL_TRUE);
    }
}

void nckp_init()
{
    nckp_buildScene();

    glGenVertexArrays(1, &nckp_vao);
    glBindVertexArray(nckp_vao);

    glGenBuffers(1, &nckp_positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, nckp_positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, nckp_barCount * nckp_vertsPerBar * sizeof(vec4), nckp_positions, GL_STATIC_DRAW);

    nckp_shaderProgram = InitShader("../shaders/vshader_impossible.glsl", "../shaders/fshader_impossible.glsl");
    glUseProgram(nckp_shaderProgram);

    GLuint posLoc = glGetAttribLocation(nckp_shaderProgram, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glGenBuffers(1, &nckp_colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, nckp_colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, nckp_barCount * nckp_vertsPerBar * sizeof(vec4), nckp_colors, GL_STATIC_DRAW);

    GLuint colLoc = glGetAttribLocation(nckp_shaderProgram, "vColor");
    glEnableVertexAttribArray(colLoc);
    glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glEnable(GL_DEPTH_TEST);

    nckp_modelPos = glGetUniformLocation(nckp_shaderProgram, "model");
    nckp_viewPos = glGetUniformLocation(nckp_shaderProgram, "view");
    nckp_projectionPos = glGetUniformLocation(nckp_shaderProgram, "projection");
}

static void nckp_drawBar(int barIdx, mat4 m, GLuint modelLoc)
{
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &m.d[0].x);
    glDrawArrays(GL_TRIANGLES, barIdx * nckp_vertsPerBar, nckp_vertsPerBar);
}

void nckp_display()
{
    float eye_x = camera_radius_nckp * sinf(camera_phi_nckp) * cosf(camera_theta_nckp);
    float eye_y = camera_radius_nckp * cosf(camera_phi_nckp);
    float eye_z = camera_radius_nckp * sinf(camera_phi_nckp) * sinf(camera_theta_nckp);

    vec3 eye(eye_x, eye_y, eye_z);
    vec3 at(0.0f, 0.0f, 0.0f);
    vec3 up(0.0f, 1.0f, 0.0f);

    glUseProgram(nckp_shaderProgram);
    glBindVertexArray(nckp_vao);

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

    glUniformMatrix4fv(nckp_viewPos, 1, GL_FALSE, &view.d[0].x);
    glUniformMatrix4fv(nckp_projectionPos, 1, GL_FALSE, &projection.d[0].x);

    // --- 4 new uniforms, looked up inline, no new globals ---
    GLuint p = nckp_shaderProgram;
    vec3 lightPos(3.0f, 5.0f, 3.0f);
    glUniform3fv(glGetUniformLocation(p, "uLightPos"), 1, &lightPos.x);
    glUniform3fv(glGetUniformLocation(p, "uEyePos"), 1, &eye.x);
    glUniform1f(glGetUniformLocation(p, "uTime"), glfwGetTime());
    // uObjHeight = total world-space height of cube = S + T
    glUniform1f(glGetUniformLocation(p, "uObjHeight"), 0.8f + 0.14f);

    mat4 sceneRot = RotateY(nckp_angleY) * RotateX(nckp_angleX) * RotateZ(nckp_angleZ);

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
    nckp_drawBar(idx++, sceneRot * swap * Translate(0, -h, -h) * Scale(S + T, T, T), nckp_modelPos);  // 0: back-corner
    nckp_drawBar(idx++, sceneRot *        Translate(0, -h,  h) * Scale(S + T, T, T), nckp_modelPos);  // 1
    nckp_drawBar(idx++, sceneRot * swap * Translate(-h, -h, 0) * Scale(T, T, S + T), nckp_modelPos);  // 2: back-corner
    nckp_drawBar(idx++, sceneRot *        Translate( h, -h, 0) * Scale(T, T, S + T), nckp_modelPos);  // 3

    // Top face: 4=back-X, 5=front-X (front-corner), 6=back-Z, 7=front-Z (front-corner)
    nckp_drawBar(idx++, sceneRot *        Translate(0,  h, -h) * Scale(S + T, T, T), nckp_modelPos);  // 4
    nckp_drawBar(idx++, sceneRot *        Translate(0,  h,  h) * Scale(S + T, T, T), nckp_modelPos);  // 5: front-corner
    nckp_drawBar(idx++, sceneRot *        Translate(-h, h, 0) * Scale(T, T, S + T), nckp_modelPos);   // 6
    nckp_drawBar(idx++, sceneRot *        Translate( h, h, 0) * Scale(T, T, S + T), nckp_modelPos);   // 7: front-corner

    // Verticals: 8=back-corner, 9=front-corner, 10/11=mid
    nckp_drawBar(idx++, sceneRot * swap * Translate(-h, 0, -h) * Scale(T, S + T, T), nckp_modelPos);  // 8: back-corner
    nckp_drawBar(idx++, sceneRot *        Translate( h, 0,  h) * Scale(T, S + T, T), nckp_modelPos);  // 9: front-corner
    nckp_drawBar(idx++, sceneRot *        Translate( h, 0, -h) * Scale(T, S + T, T), nckp_modelPos);  // 10
    nckp_drawBar(idx++, sceneRot *        Translate(-h, 0,  h) * Scale(T, S + T, T), nckp_modelPos);  // 11

    glFinish();
}
