// polygon_bg.cpp
// Bloom post-processing pipeline and interactive backgrounds for slot 0
// (Impossible Polygon).  See polygon_bg.h for the public API.

#include "polygon_bg.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

// ── Shared GL objects ────────────────────────────────────────────────────────
static GLuint quad_vao, quad_vbo;

// Four FBOs in the bloom pipeline
static GLuint fbo_scene,  tex_scene;   // raw render
static GLuint fbo_bright, tex_bright;  // bright-pass extraction
static GLuint fbo_blur_h, tex_blur_h;  // horizontal Gaussian
static GLuint fbo_blur_v, tex_blur_v;  // vertical Gaussian   (= final bloom)

// Shared depth renderbuffer for the scene FBO
static GLuint rbo_depth;

// ── Programs ─────────────────────────────────────────────────────────────────
static GLuint prog_bright, prog_blur, prog_bloom;
static GLint  blur_horiz_loc;    // uHorizontal
static GLint  bloom_scene_loc;   // uScene
static GLint  bloom_blur_loc;    // uBloom

static GLuint prog_escher;
static GLint  esch_eye_loc, esch_right_loc, esch_up_loc, esch_fwd_loc;
static GLint  esch_time_loc, esch_aspect_loc;

static GLuint prog_mandelbulb;
static GLint  bulb_eye_loc, bulb_right_loc, bulb_up_loc, bulb_fwd_loc;
static GLint  bulb_time_loc, bulb_aspect_loc;

static GLuint prog_gyroid;
static GLint  gyro_eye_loc, gyro_right_loc, gyro_up_loc, gyro_fwd_loc;
static GLint  gyro_time_loc, gyro_aspect_loc;

static GLuint prog_menger;
static GLint  mng_eye_loc, mng_right_loc, mng_up_loc, mng_fwd_loc;
static GLint  mng_time_loc, mng_aspect_loc;

// ── Helpers ──────────────────────────────────────────────────────────────────

// Read a text file into a string.
static std::string readFile(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        fprintf(stderr, "[polygon_bg] Cannot open shader: %s\n", path);
        return "";
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Compile a single shader stage.
static GLuint compileStage(GLenum type, const std::string& src)
{
    const char* s = src.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &s, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        fprintf(stderr, "[polygon_bg] Shader compile error:\n%s\n", log);
    }
    return shader;
}

// Build a program, binding aPos=0 and aUV=1 before the link.
static GLuint makeProgram(const char* vpath, const char* fpath)
{
    std::string vSrc = readFile(vpath);
    std::string fSrc = readFile(fpath);

    GLuint vs   = compileStage(GL_VERTEX_SHADER,   vSrc);
    GLuint fs   = compileStage(GL_FRAGMENT_SHADER, fSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);

    // Fix attribute locations BEFORE link so the shared quad VAO works for
    // every background program without rebuilding the bindings.
    glBindAttribLocation(prog, 0, "aPos");
    glBindAttribLocation(prog, 1, "aUV");

    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        fprintf(stderr, "[polygon_bg] Link error:\n%s\n", log);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// Create a 16-bit floating-point colour texture + depth renderbuffer FBO.
static void makeFBO(int w, int h, GLuint* fbo, GLuint* tex,
                    bool needDepth = false, GLuint* rbo = nullptr)
{
    glGenFramebuffers(1, fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);

    if (needDepth && rbo) {
        glGenRenderbuffers(1, rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, *rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, *rbo);
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        fprintf(stderr, "[polygon_bg] FBO incomplete: 0x%x\n", status);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Draw the pre-built fullscreen quad.
static void drawQuad()
{
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ── Public API ───────────────────────────────────────────────────────────────

void polygon_bg_init()
{
    int w = screen_w, h = screen_h;
    if (w <= 0) w = 512;
    if (h <= 0) h = 512;

    // ── Fullscreen quad geometry ─────────────────────────────────────────────
    // Interleaved: x y u v  (two triangles covering NDC)
    float quad[] = {
        -1.f, -1.f,  0.f, 0.f,
         1.f, -1.f,  1.f, 0.f,
         1.f,  1.f,  1.f, 1.f,
        -1.f, -1.f,  0.f, 0.f,
         1.f,  1.f,  1.f, 1.f,
        -1.f,  1.f,  0.f, 1.f,
    };

    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);
    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    // aPos @ location 0 (stride 16 bytes, offset 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)0);
    // aUV  @ location 1 (stride 16 bytes, offset 8)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)(2 * sizeof(float)));

    glBindVertexArray(0);

    // ── FBOs ─────────────────────────────────────────────────────────────────
    makeFBO(w, h, &fbo_scene,  &tex_scene,  true, &rbo_depth);
    makeFBO(w, h, &fbo_bright, &tex_bright, false, nullptr);
    makeFBO(w, h, &fbo_blur_h, &tex_blur_h, false, nullptr);
    makeFBO(w, h, &fbo_blur_v, &tex_blur_v, false, nullptr);

    // ── Bloom pipeline programs ───────────────────────────────────────────────
    prog_bright = makeProgram("../shaders/vshader_quad.glsl",
                              "../shaders/fshader_bright.glsl");
    prog_blur   = makeProgram("../shaders/vshader_quad.glsl",
                              "../shaders/fshader_blur.glsl");
    prog_bloom  = makeProgram("../shaders/vshader_quad.glsl",
                              "../shaders/fshader_bloom.glsl");

    blur_horiz_loc = glGetUniformLocation(prog_blur,  "uHorizontal");
    bloom_scene_loc= glGetUniformLocation(prog_bloom, "uScene");
    bloom_blur_loc = glGetUniformLocation(prog_bloom, "uBloom");

    // ── Background programs ───────────────────────────────────────────────────
    prog_escher = makeProgram("../shaders/vshader_quad.glsl",
                              "../shaders/fshader_poly_bg_escher.glsl");
    esch_eye_loc    = glGetUniformLocation(prog_escher, "uEyePos");
    esch_right_loc  = glGetUniformLocation(prog_escher, "uCamRight");
    esch_up_loc     = glGetUniformLocation(prog_escher, "uCamUp");
    esch_fwd_loc    = glGetUniformLocation(prog_escher, "uCamForward");
    esch_time_loc   = glGetUniformLocation(prog_escher, "uTime");
    esch_aspect_loc = glGetUniformLocation(prog_escher, "uAspect");

    prog_mandelbulb = makeProgram("../shaders/vshader_quad.glsl",
                                  "../shaders/fshader_poly_bg_mandelbulb.glsl");
    bulb_eye_loc    = glGetUniformLocation(prog_mandelbulb, "uEyePos");
    bulb_right_loc  = glGetUniformLocation(prog_mandelbulb, "uCamRight");
    bulb_up_loc     = glGetUniformLocation(prog_mandelbulb, "uCamUp");
    bulb_fwd_loc    = glGetUniformLocation(prog_mandelbulb, "uCamForward");
    bulb_time_loc   = glGetUniformLocation(prog_mandelbulb, "uTime");
    bulb_aspect_loc = glGetUniformLocation(prog_mandelbulb, "uAspect");

    prog_gyroid = makeProgram("../shaders/vshader_quad.glsl",
                              "../shaders/fshader_poly_bg_gyroid.glsl");
    gyro_eye_loc    = glGetUniformLocation(prog_gyroid, "uEyePos");
    gyro_right_loc  = glGetUniformLocation(prog_gyroid, "uCamRight");
    gyro_up_loc     = glGetUniformLocation(prog_gyroid, "uCamUp");
    gyro_fwd_loc    = glGetUniformLocation(prog_gyroid, "uCamForward");
    gyro_time_loc   = glGetUniformLocation(prog_gyroid, "uTime");
    gyro_aspect_loc = glGetUniformLocation(prog_gyroid, "uAspect");

    prog_menger = makeProgram("../shaders/vshader_quad.glsl",
                              "../shaders/fshader_poly_bg_menger.glsl");
    mng_eye_loc    = glGetUniformLocation(prog_menger, "uEyePos");
    mng_right_loc  = glGetUniformLocation(prog_menger, "uCamRight");
    mng_up_loc     = glGetUniformLocation(prog_menger, "uCamUp");
    mng_fwd_loc    = glGetUniformLocation(prog_menger, "uCamForward");
    mng_time_loc   = glGetUniformLocation(prog_menger, "uTime");
    mng_aspect_loc = glGetUniformLocation(prog_menger, "uAspect");
}

void polygon_bg_begin_scene()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_scene);
    glViewport(0, 0, screen_w, screen_h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
}

void polygon_bg_draw(vec3 eye, vec3 right, vec3 up, vec3 forward,
                     float aspect, float time, int nSides)
{
    // Select background by n-gon index (cycles every 4)
    int bg = (nSides - 3) % 4;
    if (bg < 0) bg = 0;

    // Background draws over the entire screen with no depth write so the
    // polygon (drawn after this call) can occlude it correctly.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    switch (bg)
    {
    case 0:  // ── Escher corridor ──────────────────────────────────────────
        glUseProgram(prog_escher);
        glUniform3fv(esch_eye_loc,    1, &eye.x);
        glUniform3fv(esch_right_loc,  1, &right.x);
        glUniform3fv(esch_up_loc,     1, &up.x);
        glUniform3fv(esch_fwd_loc,    1, &forward.x);
        glUniform1f (esch_time_loc,   time);
        glUniform1f (esch_aspect_loc, aspect);
        break;

    case 1:  // ── Mandelbulb 3-D fractal ─────────────────────────────
        glUseProgram(prog_mandelbulb);
        glUniform3fv(bulb_eye_loc,    1, &eye.x);
        glUniform3fv(bulb_right_loc,  1, &right.x);
        glUniform3fv(bulb_up_loc,     1, &up.x);
        glUniform3fv(bulb_fwd_loc,    1, &forward.x);
        glUniform1f (bulb_time_loc,   time);
        glUniform1f (bulb_aspect_loc, aspect);
        break;

    case 2:  // ── Gyroid SDF ───────────────────────────────────────────────
        glUseProgram(prog_gyroid);
        glUniform3fv(gyro_eye_loc,    1, &eye.x);
        glUniform3fv(gyro_right_loc,  1, &right.x);
        glUniform3fv(gyro_up_loc,     1, &up.x);
        glUniform3fv(gyro_fwd_loc,    1, &forward.x);
        glUniform1f (gyro_time_loc,   time);
        glUniform1f (gyro_aspect_loc, aspect);
        break;

    default: // ── Menger Sponge 3-D fractal ────────────────────────────────
        glUseProgram(prog_menger);
        glUniform3fv(mng_eye_loc,    1, &eye.x);
        glUniform3fv(mng_right_loc,  1, &right.x);
        glUniform3fv(mng_up_loc,     1, &up.x);
        glUniform3fv(mng_fwd_loc,    1, &forward.x);
        glUniform1f (mng_time_loc,   time);
        glUniform1f (mng_aspect_loc, aspect);
        break;
    }

    drawQuad();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void polygon_bg_end_scene()
{
    // ── 1. Bright-pass extraction ────────────────────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_bright);
    glViewport(0, 0, screen_w, screen_h);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(prog_bright);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_scene);
    glUniform1i(glGetUniformLocation(prog_bright, "uScene"), 0);
    drawQuad();

    // ── 2. Horizontal Gaussian blur ──────────────────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_blur_h);
    glViewport(0, 0, screen_w, screen_h);

    glUseProgram(prog_blur);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_bright);
    glUniform1i(glGetUniformLocation(prog_blur, "uScene"), 0);
    glUniform1i(blur_horiz_loc, 1);
    drawQuad();

    // ── 3. Vertical Gaussian blur ────────────────────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_blur_v);
    glViewport(0, 0, screen_w, screen_h);

    glBindTexture(GL_TEXTURE_2D, tex_blur_h);
    glUniform1i(blur_horiz_loc, 0);
    drawQuad();

    // ── 4. Composite to default framebuffer ──────────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screen_w, screen_h);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(prog_bloom);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_scene);
    glUniform1i(bloom_scene_loc, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex_blur_v);
    glUniform1i(bloom_blur_loc, 1);

    drawQuad();

    // Restore GL state
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_DEPTH_TEST);
}
