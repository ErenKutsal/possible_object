//
//#include "includes.h"
//
//GLuint pm_base_shaderProgram;
//
//// vertex array + buffers
//GLuint pm_base_vao;
//GLuint pm_base_positionBuffer;
//GLuint pm_base_colorBuffer;
//
//// views
//GLuint pm_base_modelPos;
//GLuint pm_base_viewPos;
//GLuint pm_base_projectionPos;
//
//// one cuboid has 6 faces, each face is made of 2 tiangles, each triangle has 3 vertices
//const int verticesPerCuboid = 36;
//
//// I will have 3 cuboids
//const int totalVertices = verticesPerCuboid * 3;
//
//// arrays sent to GPU
//vec4 vertexPositions[totalVertices];
//vec4 vertexColors[totalVertices];
//
//// rotation angles
//float pm_base_angleX = -88.0;
//float pm_base_angleY = 90.0;
//float pm_base_angleZ = -2.5;
//
//// mouse control
//bool pm_base_isDragging = false;
//double pm_base_mouseX = 0.0;
//double pm_base_mouseY = 0.0;
//
//// ------------------------------------------------
//// color for each face
//// ------------------------------------------------
//vec4 getFaceColor(int faceIndex)
//{
//    if (faceIndex == 0) return vec4(0.73f, 0.58f, 0.62f, 1);  // pink front
//    if (faceIndex == 1) return vec4(0.35f, 0.38f, 0.58f, 1);  // blue dark
//    if (faceIndex == 2) return vec4(0.78f, 0.65f, 0.68f, 1);  // pink light top
//    if (faceIndex == 3) return vec4(0.28f, 0.30f, 0.45f, 1);  // dark bottom
//    if (faceIndex == 4) return vec4(0.45f, 0.47f, 0.65f, 1);  // blue medium
//    return vec4(0.35f, 0.38f, 0.58f, 1);                      // blue dark
//}
//
//// ------------------------------------------------
//// build one cuboid
//// ------------------------------------------------
//void buildCube(vec4* posArray, vec4* colArray)
//{
//    // 8 cuboid corners
//    vec4 vertices[8] = {vec4(-0.5, -0.5, 0.5, 1), vec4(-0.5, 0.5, 0.5, 1),   vec4(0.5, 0.5, 0.5, 1),
//                        vec4(0.5, -0.5, 0.5, 1),  vec4(-0.5, -0.5, -0.5, 1), vec4(-0.5, 0.5, -0.5, 1),
//                        vec4(0.5, 0.5, -0.5, 1),  vec4(0.5, -0.5, -0.5, 1)};
//
//    int index = 0;
//
//    // 6 faces; each face is defined by 4 vertices
//    int faces[6][4] = {{0, 1, 2, 3}, {3, 2, 6, 7}, {0, 1, 5, 4}, {2, 1, 5, 6}, {4, 5, 6, 7}, {0, 3, 7, 4}};
//
//    // build 2 triangles for each face
//    for (int i = 0; i < 6; i++)
//    {
//        vec4 c = getFaceColor(i);
//
//        posArray[index] = vertices[faces[i][0]];
//        colArray[index++] = c;
//        posArray[index] = vertices[faces[i][1]];
//        colArray[index++] = c;
//        posArray[index] = vertices[faces[i][2]];
//        colArray[index++] = c;
//
//        posArray[index] = vertices[faces[i][0]];
//        colArray[index++] = c;
//        posArray[index] = vertices[faces[i][2]];
//        colArray[index++] = c;
//        posArray[index] = vertices[faces[i][3]];
//        colArray[index++] = c;
//    }
//}
//
//// ------------------------------------------------
//// build 3 cuboids
//// ------------------------------------------------
//void buildScene()
//{
//    buildCube(vertexPositions + 0 * verticesPerCuboid, vertexColors + 0 * verticesPerCuboid);
//    buildCube(vertexPositions + 1 * verticesPerCuboid, vertexColors + 1 * verticesPerCuboid);
//    buildCube(vertexPositions + 2 * verticesPerCuboid, vertexColors + 2 * verticesPerCuboid);
//}
//
//// ------------------------------------------------
//// mouse + keyboard controls for penrose
//// ------------------------------------------------
//void pm_base_mouseButtonCallback(GLFWwindow* window, int button, int action, int)
//{
//    if (button == GLFW_MOUSE_BUTTON_LEFT)
//    {
//        pm_base_isDragging = (action == GLFW_PRESS);
//
//        if (pm_base_isDragging)
//        {
//            glfwGetCursorPos(window, &pm_base_mouseX, &pm_base_mouseY);
//        }
//    }
//}
//
//void pm_base_cursorPosCallback(GLFWwindow* window, double x, double y)
//{
//    if (!pm_base_isDragging) return;
//
//    pm_base_angleY += (float)(x - pm_base_mouseX) * 0.4;
//    pm_base_angleX += (float)(y - pm_base_mouseY) * 0.4;
//
//    pm_base_mouseX = x;
//    pm_base_mouseY = y;
//}
//
//void pm_base_scrollCallback(GLFWwindow*, double, double yoffset) { pm_base_angleZ += (float)yoffset * 2.0; }
//
//void pm_base_keyCallback(GLFWwindow* win, int key, int, int action, int)
//{
//    if (action == GLFW_PRESS || action == GLFW_REPEAT)
//    {
//        if (key == GLFW_KEY_LEFT) pm_base_angleY -= 3;
//        if (key == GLFW_KEY_RIGHT) pm_base_angleY += 3;
//        if (key == GLFW_KEY_UP) pm_base_angleX -= 3;
//        if (key == GLFW_KEY_DOWN) pm_base_angleX += 3;
//
//        // reset position
//        if (key == GLFW_KEY_R)
//        {
//            pm_base_angleX = 18;
//            pm_base_angleY = 0;
//            pm_base_angleZ = -25;
//        }
//
//        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, GL_TRUE);
//    }
//}
//
//// ------------------------------------------------
//// initialize GPU data for penrose
//// ------------------------------------------------
//void pm_base_init()
//{
//    buildScene();
//
//    glGenVertexArrays(1, &pm_base_vao);
//    glBindVertexArray(pm_base_vao);
//
//    // send positions
//    glGenBuffers(1, &pm_base_positionBuffer);
//    glBindBuffer(GL_ARRAY_BUFFER, pm_base_positionBuffer);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
//
//    pm_base_shaderProgram = InitShader(SHADER_DIR "vshader_simple.glsl", SHADER_DIR "fshader_simple.glsl");
//    glUseProgram(pm_base_shaderProgram);
//
//    GLuint posLoc = glGetAttribLocation(pm_base_shaderProgram, "vPosition");
//    glEnableVertexAttribArray(posLoc);
//    glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
//
//    // send colors
//    glGenBuffers(1, &pm_base_colorBuffer);
//    glBindBuffer(GL_ARRAY_BUFFER, pm_base_colorBuffer);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);
//
//    GLuint colLoc = glGetAttribLocation(pm_base_shaderProgram, "vColor");
//    glEnableVertexAttribArray(colLoc);
//    glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
//
//    glEnable(GL_DEPTH_TEST);
//    glClearColor(1.0, 1.0, 1.0, 1.0);
//
//    // get uniform locations/views
//    pm_base_modelPos = glGetUniformLocation(pm_base_shaderProgram, "model");
//    pm_base_viewPos = glGetUniformLocation(pm_base_shaderProgram, "view");
//    pm_base_projectionPos = glGetUniformLocation(pm_base_shaderProgram, "projection");
//}
//
//// ------------------------------------------------
//// draw penrose scene
//// ------------------------------------------------
//void pm_base_display()
//{
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//    glUseProgram(pm_base_shaderProgram);
//    glBindVertexArray(pm_base_vao);
//
//    // camera
//    vec3 eye(2.5, 2.5, 2.5);
//    vec3 at(0.0, 0.0, 0.0);  // <--- CHANGED: Was (0.75, 0.75, 0.35)
//    vec3 up(0, 1, 0);
//    mat4 view = LookAt(eye, at, up);
//
//    // perspective
//    mat4 projection = Perspective(30.0, 550.0/500.0, 0.1, 10.0);
//
//    glUniformMatrix4fv(pm_base_viewPos, 1, GL_FALSE, &view.d[0].x);
//    glUniformMatrix4fv(pm_base_projectionPos, 1, GL_FALSE, &projection.d[0].x);
//
//    // rotation
//    mat4 sceneRotation = RotateY(pm_base_angleY) * RotateX(pm_base_angleX) * RotateZ(pm_base_angleZ);
//
//    float length = 1.0;
//    float thickness = 0.2;
//
//    float cx = length * 0.5f;
//    float cy = length * 0.5f;
//    float cz = -length * 0.5f;
//    mat4 centerOffset = Translate(-cx, -cy, -cz);
//
//    // cuboid 1
//    mat4 model1 = sceneRotation * centerOffset * Translate(length * 0.5, 0, 0) * Scale(length, thickness, thickness);
//    glUniformMatrix4fv(pm_base_modelPos, 1, GL_FALSE, &model1.d[0].x);
//    glDrawArrays(GL_TRIANGLES, 0, verticesPerCuboid);
//
//    // cuboid 2
//    mat4 model2 = sceneRotation * centerOffset * Translate(length - thickness * 0.5, length * 0.5, 0) * RotateZ(90) * Scale(length, thickness, thickness);
//    glUniformMatrix4fv(pm_base_modelPos, 1, GL_FALSE, &model2.d[0].x);
//    glDrawArrays(GL_TRIANGLES, verticesPerCuboid, verticesPerCuboid);
//
//    // cuboid 3
//    mat4 model3 = sceneRotation * centerOffset * Translate(length - thickness * 0.5, length - thickness * 0.5, -length * 0.5) * RotateY(90) * Scale(length, thickness, thickness);
//    glUniformMatrix4fv(pm_base_modelPos, 1, GL_FALSE, &model3.d[0].x);
//    glDrawArrays(GL_TRIANGLES, 2 * verticesPerCuboid, verticesPerCuboid);
//
//    glFinish();
//}


//====================================================================================================================================


#include "includes.h"

GLuint pm_base_shaderProgram;
GLuint pm_base_vao;
GLuint pm_base_positionBuffer;
GLuint pm_base_colorBuffer;
GLuint pm_base_modelPos;
GLuint pm_base_viewPos;
GLuint pm_base_projectionPos;

const int verticesPerCuboid = 36;
const int totalVertices = verticesPerCuboid * 3;

vec4 vertexPositions[totalVertices];
vec4 vertexColors[totalVertices];

float pm_base_angleX = 0.0f;
float pm_base_angleY = 0.0f;
float pm_base_angleZ = 0.0f;

bool   pm_base_isDragging = false;
double pm_base_mouseX = 0.0;
double pm_base_mouseY = 0.0;

//vec4 getFaceColor(int faceIndex)
//{
//    if (faceIndex == 3)                   return vec4(0.60f, 0.68f, 0.78f, 1.0f);
//    if (faceIndex == 0 || faceIndex == 2) return vec4(0.38f, 0.45f, 0.58f, 1.0f);
//    return                                       vec4(0.20f, 0.25f, 0.38f, 1.0f);
//}

//vec4 getFaceColor(int faceIndex)
//{
//    if (faceIndex == 3)                   return vec4(0.95f, 0.75f, 0.10f, 1.0f);
//    if (faceIndex == 0 || faceIndex == 2) return vec4(0.90f, 0.35f, 0.05f, 1.0f);
//    return                                       vec4(0.60f, 0.08f, 0.02f, 1.0f);
//}

vec4 getFaceColor(int faceIndex)
{
    if (faceIndex == 3)                   return vec4(0.88f, 0.96f, 1.00f, 1.0f);
    if (faceIndex == 0 || faceIndex == 2) return vec4(0.45f, 0.78f, 0.95f, 1.0f);
    return                                       vec4(0.15f, 0.42f, 0.72f, 1.0f);
}

void buildCube(vec4* posArray, vec4* colArray)
{
    // NOTE: all w-components are 1.0f (not bare 1) to silence MSVC float warnings
    vec4 v[8] = {
        vec4(-0.5f, -0.5f,  0.5f, 1.0f),
        vec4(-0.5f,  0.5f,  0.5f, 1.0f),
        vec4(0.5f,  0.5f,  0.5f, 1.0f),
        vec4(0.5f, -0.5f,  0.5f, 1.0f),
        vec4(-0.5f, -0.5f, -0.5f, 1.0f),
        vec4(-0.5f,  0.5f, -0.5f, 1.0f),
        vec4(0.5f,  0.5f, -0.5f, 1.0f),
        vec4(0.5f, -0.5f, -0.5f, 1.0f)
    };

    int faces[6][4] = {
        {0, 1, 2, 3},  // 0  front (+Z)
        {3, 2, 6, 7},  // 1  right (+X)
        {4, 5, 1, 0},  // 2  left  (-X)
        {1, 5, 6, 2},  // 3  top   (+Y)
        {7, 6, 5, 4},  // 4  back  (-Z)
        {0, 3, 7, 4}   // 5  bottom(-Y)
    };

    int idx = 0;
    for (int i = 0; i < 6; i++)
    {
        vec4 c = getFaceColor(i);
        posArray[idx] = v[faces[i][0]]; colArray[idx++] = c;
        posArray[idx] = v[faces[i][1]]; colArray[idx++] = c;
        posArray[idx] = v[faces[i][2]]; colArray[idx++] = c;
        posArray[idx] = v[faces[i][0]]; colArray[idx++] = c;
        posArray[idx] = v[faces[i][2]]; colArray[idx++] = c;
        posArray[idx] = v[faces[i][3]]; colArray[idx++] = c;
    }
}

void buildScene()
{
    buildCube(vertexPositions + 0 * verticesPerCuboid, vertexColors + 0 * verticesPerCuboid);
    buildCube(vertexPositions + 1 * verticesPerCuboid, vertexColors + 1 * verticesPerCuboid);
    buildCube(vertexPositions + 2 * verticesPerCuboid, vertexColors + 2 * verticesPerCuboid);
}

void pm_base_mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        pm_base_isDragging = (action == GLFW_PRESS);
        if (pm_base_isDragging) glfwGetCursorPos(window, &pm_base_mouseX, &pm_base_mouseY);
    }
}

void pm_base_cursorPosCallback(GLFWwindow* window, double x, double y)
{
    if (!pm_base_isDragging) return;
    pm_base_angleY += (float)(x - pm_base_mouseX) * 0.4f;
    pm_base_angleX += (float)(y - pm_base_mouseY) * 0.4f;
    pm_base_mouseX = x;
    pm_base_mouseY = y;
}

void pm_base_scrollCallback(GLFWwindow*, double, double yoffset)
{
    pm_base_angleZ += (float)yoffset * 2.0f;
}

void pm_base_keyCallback(GLFWwindow* win, int key, int, int action, int)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_LEFT)   pm_base_angleY -= 3.0f;
        if (key == GLFW_KEY_RIGHT)  pm_base_angleY += 3.0f;
        if (key == GLFW_KEY_UP)     pm_base_angleX -= 3.0f;
        if (key == GLFW_KEY_DOWN)   pm_base_angleX += 3.0f;
        if (key == GLFW_KEY_R) { pm_base_angleX = 0.0f; pm_base_angleY = 0.0f; pm_base_angleZ = 0.0f; }
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, GL_TRUE);
    }
}

void pm_base_init()
{
    buildScene();

    glGenVertexArrays(1, &pm_base_vao);
    glBindVertexArray(pm_base_vao);

    glGenBuffers(1, &pm_base_positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, pm_base_positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);

    pm_base_shaderProgram = InitShader("../shaders/core/vshader_impossible.glsl", "../shaders/core/fshader_impossible.glsl");
    glUseProgram(pm_base_shaderProgram);

    GLuint posLoc = glGetAttribLocation(pm_base_shaderProgram, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glGenBuffers(1, &pm_base_colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, pm_base_colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);

    GLuint colLoc = glGetAttribLocation(pm_base_shaderProgram, "vColor");
    glEnableVertexAttribArray(colLoc);
    glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);

    pm_base_modelPos = glGetUniformLocation(pm_base_shaderProgram, "model");
    pm_base_viewPos = glGetUniformLocation(pm_base_shaderProgram, "view");
    pm_base_projectionPos = glGetUniformLocation(pm_base_shaderProgram, "projection");
}

void pm_base_display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(pm_base_shaderProgram);
    glBindVertexArray(pm_base_vao);

    // Using equal distance components creates the required isometric-like angle
    vec3 eye(3.0f, 3.0f, 3.0f);
    vec3 at(0.0f, 0.0f, 0.0f);
    vec3 up(0.0f, 1.0f, 0.0f);
    mat4 view = LookAt(eye, at, up);
    // Orthographic projection is CRITICAL for the Penrose illusion to align edges
    mat4 projection = Ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 20.0f);

    glUniformMatrix4fv(pm_base_viewPos, 1, GL_FALSE, &view.d[0].x);
    glUniformMatrix4fv(pm_base_projectionPos, 1, GL_FALSE, &projection.d[0].x);

    mat4 R = RotateY(pm_base_angleY) * RotateX(pm_base_angleX) * RotateZ(pm_base_angleZ);

    const float L = 1.0f;  // Length of the bars
    const float T = 0.25f; // Thickness of the bars

    // Centers the entire object structure nicely in the viewport
    mat4 centerOffset = Translate(-L * 0.5f, -L * 0.5f, -L * 0.15f);

    // arm1 — X Bar (Top horizontal bar extending along +X)
    mat4 model1 = R * centerOffset
        * Translate(L * 0.5f, L, 0.0f)
        * Scale(L+T, T, T);
    glUniformMatrix4fv(pm_base_modelPos, 1, GL_FALSE, &model1.d[0].x);
    glDrawArrays(GL_TRIANGLES, 0 * verticesPerCuboid, verticesPerCuboid);

    // arm2 — Y Bar (Left vertical bar extending along +Y)
    mat4 model2 = R * centerOffset
        * Translate(0.0f, L * 0.5f, 0.0f)
        * Scale(T, L, T);
    glUniformMatrix4fv(pm_base_modelPos, 1, GL_FALSE, &model2.d[0].x);
    glDrawArrays(GL_TRIANGLES, 1 * verticesPerCuboid, verticesPerCuboid);

    glClear(GL_DEPTH_BUFFER_BIT);

    // arm3 — Z Bar (The "impossible" gap closer extending along +Z)
    mat4 model3 = R * centerOffset
        * Translate(L, L, L * 0.5f + T * 0.5f)
        * Scale(T, T, L);
    glUniformMatrix4fv(pm_base_modelPos, 1, GL_FALSE, &model3.d[0].x);
    glDrawArrays(GL_TRIANGLES, 2 * verticesPerCuboid, verticesPerCuboid);

    glFinish();
}

