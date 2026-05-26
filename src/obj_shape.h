#pragma once
#include "includes.h"
#include "obj_loader.h"

// ─────────────────────────────────────────────────────────────────────────────
// ObjShape — bundles per-shape OpenGL state + interaction for an OBJ figure.
//
// Each impossible figure that comes from a .obj file in models/ owns one
// ObjShape instance. The per-shape .cpp files (penrose.cpp, neckercube.cpp,
// ...) are now thin wrappers that delegate their init/display/callback
// functions to a static ObjShape inside that file.
//
// This struct exists so the boilerplate (load OBJ → make VAO/VBO → compile
// shader → render with axonometric matrix → handle mouse/key rotation) lives
// in exactly one place.
// ─────────────────────────────────────────────────────────────────────────────

struct ObjShape
{
    // OpenGL handles
    GLuint shaderProgram = 0;
    GLuint vao = 0;
    GLuint positionBuffer = 0;
    GLuint colorBuffer = 0;
    GLuint modelLoc = 0, viewLoc = 0, projLoc = 0;
    GLint  lightLoc = -1, eyeLoc = -1, timeLoc = -1, heightLoc = -1;

    // CPU-side mesh data
    std::vector<vec4> positions;
    std::vector<vec4> colors;

    // PUZZLE STARTING POSITION (the "unsolved" pose) — clearly off the magic
    // axonometric so the illusion is broken and the player has to rotate to
    // find the angle where the cut sides "touch". The actual solved angle is
    // (54.736°, 0°, -45°) — that's what S-key snaps to.
    //
    // Per-slot init() can override defaultAngle* with a different starting
    // pose; R-key resets to whichever defaults that slot set.
    float angleX = 40.0f;
    float angleY = 25.0f;
    float angleZ = -30.0f;
    float defaultAngleX = 40.0f;
    float defaultAngleY = 25.0f;
    float defaultAngleZ = -30.0f;

    // Mouse drag state
    bool   isDragging = false;
    double mouseX = 0.0;
    double mouseY = 0.0;

    // Camera placement (orthographic, looking from eye at origin)
    vec3   cameraEye = vec3(25.0f, 25.0f, 25.0f);
    float  orthoSize = 12.0f;
    float  objHeight = 12.0f;

    // ──────────────────────────────────────────────────────────────────────
    // init: load the OBJ file at `path` with `palette` colors, compile the
    // shared impossible-figure shader, upload mesh to GPU.
    // ──────────────────────────────────────────────────────────────────────
    void init(const char* objPath, const ObjColorPalette& palette)
    {
        if (!obj_load(objPath, palette, positions, colors))
        {
            fprintf(stderr, "ObjShape::init: failed to load %s\n", objPath);
        }

        shaderProgram = InitShader("../shaders/vshader_impossible.glsl",
                                   "../shaders/fshader_impossible.glsl");

        lightLoc  = glGetUniformLocation(shaderProgram, "uLightPos");
        eyeLoc    = glGetUniformLocation(shaderProgram, "uEyePos");
        timeLoc   = glGetUniformLocation(shaderProgram, "uTime");
        heightLoc = glGetUniformLocation(shaderProgram, "uObjHeight");
        modelLoc  = glGetUniformLocation(shaderProgram, "model");
        viewLoc   = glGetUniformLocation(shaderProgram, "view");
        projLoc   = glGetUniformLocation(shaderProgram, "projection");

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &positionBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(vec4),
                     positions.data(), GL_STATIC_DRAW);
        GLuint posLoc = glGetAttribLocation(shaderProgram, "vPosition");
        glEnableVertexAttribArray(posLoc);
        glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

        glGenBuffers(1, &colorBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(vec4),
                     colors.data(), GL_STATIC_DRAW);
        GLuint colLoc = glGetAttribLocation(shaderProgram, "vColor");
        glEnableVertexAttribArray(colLoc);
        glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

        glEnable(GL_DEPTH_TEST);
    }

    // ──────────────────────────────────────────────────────────────────────
    // display: render the shape with current rotation against an ortho cam.
    // ──────────────────────────────────────────────────────────────────────
    void display()
    {
        glUseProgram(shaderProgram);
        glBindVertexArray(vao);

        vec3 eye(cameraEye);
        vec3 at(0.0f, 0.0f, 0.0f);
        vec3 up(0.0f, 1.0f, 0.0f);
        mat4 view = LookAt(eye, at, up);

        float aspect = (screen_w > 0 && screen_h > 0)
                           ? (float)screen_w / (float)screen_h
                           : 1.0f;
        mat4 projection = Ortho(-orthoSize * aspect, orthoSize * aspect,
                                -orthoSize, orthoSize, 0.1f, 200.0f);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view.d[0].x);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection.d[0].x);

        mat4 model = RotateY(angleY) * RotateX(angleX) * RotateZ(angleZ);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model.d[0].x);

        vec3 lightPos(eye.x * 0.8f, eye.y * 1.6f, eye.z * 1.2f);
        glUniform3fv(lightLoc, 1, &lightPos.x);
        glUniform3fv(eyeLoc, 1, &eye.x);
        glUniform1f(timeLoc, (float)glfwGetTime());
        glUniform1f(heightLoc, objHeight);

        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)positions.size());
        glFinish();
    }

    // ──────────────────────────────────────────────────────────────────────
    // Mouse / keyboard handlers
    // ──────────────────────────────────────────────────────────────────────
    void mouseButton(GLFWwindow* window, int button, int action)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            isDragging = (action == GLFW_PRESS);
            if (isDragging) glfwGetCursorPos(window, &mouseX, &mouseY);
        }
    }

    void cursorPos(double x, double y)
    {
        if (!isDragging) return;
        angleY += (float)(x - mouseX) * 0.4f;
        angleX += (float)(y - mouseY) * 0.4f;
        mouseX = x;
        mouseY = y;
    }

    void scroll(double yoffset)
    {
        angleZ += (float)yoffset * 2.0f;
    }

    void key(GLFWwindow* win, int keyCode, int action)
    {
        if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
        if (keyCode == GLFW_KEY_LEFT)  angleY -= 3.0f;
        if (keyCode == GLFW_KEY_RIGHT) angleY += 3.0f;
        if (keyCode == GLFW_KEY_UP)    angleX -= 3.0f;
        if (keyCode == GLFW_KEY_DOWN)  angleX += 3.0f;
        if (keyCode == GLFW_KEY_R)
        {
            // R = reset to this slot's defaults (may include per-slot tweaks
            // like the Y-rotated Penrose Stair or Z-rotated Impossible Arch).
            angleX = defaultAngleX;
            angleY = defaultAngleY;
            angleZ = defaultAngleZ;
        }
        if (keyCode == GLFW_KEY_S)
        {
            // S = "solved" pose.
            //
            // The OBJ files come out of the Paradox Toolkit Blender addon
            // already laid out in iso-aligned coordinates: Blender's axonometric
            // camera at (14.43, -14.43, 14.43) maps — after the Wavefront
            // axis remap (Blender Y -> OBJ -Z, Blender Z -> OBJ Y) — to the
            // OBJ-space direction (1, 1, 1)/sqrt(3). The bisect cuts that
            // Paradox bakes into each figure are perpendicular to that
            // direction.
            //
            // Our OpenGL camera lives at (25, 25, 25) looking at the origin
            // with up = (0, 1, 0), so its view direction is also (1, 1, 1)/
            // sqrt(3) (same line, opposite sign — equivalent for orthographic
            // projection). The bisect cuts therefore project to lines and the
            // illusion clicks into place WITHOUT any figure rotation.
            //
            // Hence the solved pose is identity rotation.
            angleX = 0.0f;
            angleY = 0.0f;
            angleZ = 0.0f;
        }
        if (keyCode == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, GL_TRUE);
    }
};

// Default palette that gives axis-aligned faces distinct shades.
inline ObjColorPalette default_palette()
{
    ObjColorPalette p;
    p.xp = vec4(0.45f, 0.48f, 0.62f, 1.0f);
    p.xn = vec4(0.38f, 0.40f, 0.55f, 1.0f);
    p.yp = vec4(0.76f, 0.60f, 0.64f, 1.0f);
    p.yn = vec4(0.35f, 0.37f, 0.52f, 1.0f);
    p.zp = vec4(0.73f, 0.58f, 0.62f, 1.0f);
    p.zn = vec4(0.42f, 0.45f, 0.60f, 1.0f);
    p.generic = vec4(0.55f, 0.55f, 0.60f, 1.0f);
    return p;
}
