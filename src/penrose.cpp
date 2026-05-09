
#include "background.h"
#include "includes.h"

GLuint penrose_shaderProgram;

// vertex array + buffers
GLuint penrose_vao;
GLuint penrose_positionBuffer;
GLuint penrose_colorBuffer;

// views
GLuint penrose_modelPos;
GLuint penrose_viewPos;
GLuint penrose_projectionPos;

// one cuboid has 6 faces, each face is made of 2 tiangles, each triangle has 3 vertices
const int verticesPerCuboid = 36;

// I will have 3 cuboids
const int totalVertices = verticesPerCuboid * 3;

// arrays sent to GPU
vec4 vertexPositions[totalVertices];
vec4 vertexColors[totalVertices];

// rotation angles
float penrose_angleX = 18.0;
float penrose_angleY = 0.0;
float penrose_angleZ = -25.0;

// mouse control
bool penrose_isDragging = false;
double penrose_mouseX = 0.0;
double penrose_mouseY = 0.0;

// ------------------------------------------------
// color for each face
// ------------------------------------------------
vec4 getFaceColor(int faceIndex)
{
    if (faceIndex == 0) return vec4(0.73f, 0.58f, 0.62f, 1);  // pink front
    if (faceIndex == 1) return vec4(0.35f, 0.38f, 0.58f, 1);  // blue dark
    if (faceIndex == 2) return vec4(0.78f, 0.65f, 0.68f, 1);  // pink light top
    if (faceIndex == 3) return vec4(0.28f, 0.30f, 0.45f, 1);  // dark bottom
    if (faceIndex == 4) return vec4(0.45f, 0.47f, 0.65f, 1);  // blue medium
    return vec4(0.35f, 0.38f, 0.58f, 1);                      // blue dark
}

// ------------------------------------------------
// build one cuboid
// ------------------------------------------------
void buildCube(vec4* posArray, vec4* colArray)
{
    // 8 cuboid corners
    vec4 vertices[8] = {vec4(-0.5, -0.5, 0.5, 1), vec4(-0.5, 0.5, 0.5, 1),   vec4(0.5, 0.5, 0.5, 1),
                        vec4(0.5, -0.5, 0.5, 1),  vec4(-0.5, -0.5, -0.5, 1), vec4(-0.5, 0.5, -0.5, 1),
                        vec4(0.5, 0.5, -0.5, 1),  vec4(0.5, -0.5, -0.5, 1)};

    int index = 0;

    // 6 faces; each face is defined by 4 vertices
    int faces[6][4] = {{0, 1, 2, 3}, {3, 2, 6, 7}, {0, 1, 5, 4}, {2, 1, 5, 6}, {4, 5, 6, 7}, {0, 3, 7, 4}};

    // build 2 triangles for each face
    for (int i = 0; i < 6; i++)
    {
        vec4 c = getFaceColor(i);

        posArray[index] = vertices[faces[i][0]];
        colArray[index++] = c;
        posArray[index] = vertices[faces[i][1]];
        colArray[index++] = c;
        posArray[index] = vertices[faces[i][2]];
        colArray[index++] = c;

        posArray[index] = vertices[faces[i][0]];
        colArray[index++] = c;
        posArray[index] = vertices[faces[i][2]];
        colArray[index++] = c;
        posArray[index] = vertices[faces[i][3]];
        colArray[index++] = c;
    }
}

// ------------------------------------------------
// build 3 cuboids
// ------------------------------------------------
void buildScene()
{
    buildCube(vertexPositions + 0 * verticesPerCuboid, vertexColors + 0 * verticesPerCuboid);
    buildCube(vertexPositions + 1 * verticesPerCuboid, vertexColors + 1 * verticesPerCuboid);
    buildCube(vertexPositions + 2 * verticesPerCuboid, vertexColors + 2 * verticesPerCuboid);
}

// ------------------------------------------------
// mouse + keyboard controls for penrose
// ------------------------------------------------
void penrose_mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        penrose_isDragging = (action == GLFW_PRESS);

        if (penrose_isDragging)
        {
            glfwGetCursorPos(window, &penrose_mouseX, &penrose_mouseY);
        }
    }
}

void penrose_cursorPosCallback(GLFWwindow* window, double x, double y)
{
    if (!penrose_isDragging) return;

    penrose_angleY += (float)(x - penrose_mouseX) * 0.4;
    penrose_angleX += (float)(y - penrose_mouseY) * 0.4;

    penrose_mouseX = x;
    penrose_mouseY = y;
}

void penrose_scrollCallback(GLFWwindow*, double, double yoffset) { penrose_angleZ += (float)yoffset * 2.0; }

void penrose_keyCallback(GLFWwindow* win, int key, int, int action, int)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_LEFT) penrose_angleY -= 3;
        if (key == GLFW_KEY_RIGHT) penrose_angleY += 3;
        if (key == GLFW_KEY_UP) penrose_angleX -= 3;
        if (key == GLFW_KEY_DOWN) penrose_angleX += 3;

        // reset position
        if (key == GLFW_KEY_R)
        {
            penrose_angleX = 18;
            penrose_angleY = 0;
            penrose_angleZ = -25;
        }

        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, GL_TRUE);
    }
}

// ------------------------------------------------
// initialize GPU data for penrose
// ------------------------------------------------
void penrose_init()
{
    buildScene();

    glGenVertexArrays(1, &penrose_vao);
    glBindVertexArray(penrose_vao);

    // send positions
    glGenBuffers(1, &penrose_positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, penrose_positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);

    penrose_shaderProgram = InitShader("../shaders/vshader_simple.glsl", "../shaders/fshader_simple.glsl");
    glUseProgram(penrose_shaderProgram);

    GLuint posLoc = glGetAttribLocation(penrose_shaderProgram, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    // send colors
    glGenBuffers(1, &penrose_colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, penrose_colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);

    GLuint colLoc = glGetAttribLocation(penrose_shaderProgram, "vColor");
    glEnableVertexAttribArray(colLoc);
    glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0, 1.0, 1.0, 1.0);

    // get uniform locations/views
    penrose_modelPos = glGetUniformLocation(penrose_shaderProgram, "model");
    penrose_viewPos = glGetUniformLocation(penrose_shaderProgram, "view");
    penrose_projectionPos = glGetUniformLocation(penrose_shaderProgram, "projection");
}

// ------------------------------------------------
// draw penrose scene
// ------------------------------------------------
void penrose_display()
{
    // Replaces glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    bg_begin_scene();

    // 1. Calculate the model's rotation FIRST
    mat4 sceneRotation = RotateY(penrose_angleY) * RotateX(penrose_angleX) * RotateZ(penrose_angleZ);

    // 2. Calculate the inverse rotation for the background camera (Relativity Trick)
    mat4 invRot = RotateZ(-penrose_angleZ) * RotateX(-penrose_angleX) * RotateY(-penrose_angleY);

    // 3. The original static global camera for the Penrose illusion
    vec3 global_eye(2.5, 2.5, 2.5);
    vec3 global_at(0.0, 0.0, 0.0);
    vec3 global_up(0, 1, 0);

    // 4. Move the raymarching camera into the spinning model's local space
    vec4 local_eye4 = invRot * vec4(global_eye.x, global_eye.y, global_eye.z, 1.0f);
    vec4 local_at4 = invRot * vec4(global_at.x, global_at.y, global_at.z, 1.0f);
    vec4 local_up4 = invRot * vec4(global_up.x, global_up.y, global_up.z, 0.0f);

    vec3 local_eye = vec3(local_eye4.x, local_eye4.y, local_eye4.z);
    vec3 local_at = vec3(local_at4.x, local_at4.y, local_at4.z);
    vec3 local_up = normalize(vec3(local_up4.x, local_up4.y, local_up4.z));

    // 5. Calculate the ray vectors using our localized background camera
    vec3 cam_forward = normalize(local_at - local_eye);
    vec3 cam_right = normalize(cross(cam_forward, local_up));
    vec3 cam_up = cross(cam_right, cam_forward);
    float current_time = glfwGetTime();

    // 6. Draw the Tesseract Lattice background behind the scene
    bg_draw_lattice(local_eye, cam_right, cam_up, cam_forward, 2.0f, current_time);
    glClear(GL_DEPTH_BUFFER_BIT);

    // ------------------------------------------------
    // Setup and draw the Penrose Geometry
    // ------------------------------------------------
    glUseProgram(penrose_shaderProgram);
    glBindVertexArray(penrose_vao);

    // Use the static global camera for the objects to preserve the optical illusion
    mat4 view = LookAt(global_eye, global_at, global_up);
    mat4 projection = Perspective(50.0, 550.0 / 500.0, 0.1, 10.0);

    glUniformMatrix4fv(penrose_viewPos, 1, GL_FALSE, &view.d[0].x);
    glUniformMatrix4fv(penrose_projectionPos, 1, GL_FALSE, &projection.d[0].x);

    float length = 1.10;
    float thickness = 0.22;

    float cx = length * 0.5f;
    float cy = length * 0.5f;
    float cz = -length * 0.5f;
    mat4 centerOffset = Translate(-cx, -cy, -cz);

    // cuboid 1
    mat4 model1 = sceneRotation * centerOffset * Translate(length * 0.5, 0, 0) * Scale(length, thickness, thickness);
    glUniformMatrix4fv(penrose_modelPos, 1, GL_FALSE, &model1.d[0].x);
    glDrawArrays(GL_TRIANGLES, 0, verticesPerCuboid);

    // cuboid 2
    mat4 model2 = sceneRotation * centerOffset * Translate(length - thickness * 0.5, length * 0.5, 0) * RotateZ(90) *
                  Scale(length, thickness, thickness);
    glUniformMatrix4fv(penrose_modelPos, 1, GL_FALSE, &model2.d[0].x);
    glDrawArrays(GL_TRIANGLES, verticesPerCuboid, verticesPerCuboid);

    // cuboid 3
    mat4 model3 = sceneRotation * centerOffset *
                  Translate(length - thickness * 0.5, length - thickness * 0.5, -length * 0.5) * RotateY(90) *
                  Scale(length, thickness, thickness);
    glUniformMatrix4fv(penrose_modelPos, 1, GL_FALSE, &model3.d[0].x);
    glDrawArrays(GL_TRIANGLES, 2 * verticesPerCuboid, verticesPerCuboid);

    // Resolves any post-processing / FBOs attached to the background system
    bg_end_scene();

    glFinish();
}