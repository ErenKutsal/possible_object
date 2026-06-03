#include "background.h"

// ---- All shared globals defined exactly once here ----
// int    screen_w = 512, screen_h = 512;
GLuint quad_vao, quad_vbo;

GLuint fbo_scene, tex_scene;
GLuint fbo_bright, tex_bright;
GLuint fbo_blur_h, tex_blur_h;
GLuint fbo_blur_v, tex_blur_v;

// Bloom pipeline (shared by all objects)
static GLuint bloom_program, bright_program, blur_program;
static GLint blur_horizontal_loc, bloom_scene_loc, bloom_blur_loc;

// Per-background programs
static GLuint escher_program;
static GLint escher_eye_loc, escher_right_loc, escher_up_loc;
static GLint escher_forward_loc, escher_view_size_loc, escher_time_loc;

static GLuint julia_program;
static GLint julia_time_loc, julia_aspect_loc, julia_c_loc, julia_zoom_loc;

// Tunnel
static GLuint tunnel_program;
static GLint tunnel_eye_loc, tunnel_right_loc, tunnel_up_loc, tunnel_forward_loc;
static GLint tunnel_time_loc, tunnel_aspect_loc;

// Gyroid
static GLuint gyroid_program;
static GLint gyroid_eye_loc, gyroid_right_loc, gyroid_up_loc, gyroid_forward_loc;
static GLint gyroid_time_loc, gyroid_aspect_loc;

// Lattice
static GLuint lattice_program;
static GLint lattice_eye_loc, lattice_right_loc, lattice_up_loc, lattice_forward_loc;
static GLint lattice_time_loc, lattice_aspect_loc;  // lattice_view_size_loc;

// ... one block per background ...

// -------------------------------------------------------

static void create_fbo(GLuint& fbo, GLuint& tex, int w, int h)
{
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Clamp so blur doesn't wrap around screen edges
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    // Depth buffer (only scene FBO needs it, others are just 2D passes)
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) fprintf(stderr, "FBO incomplete!\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void draw_quad_fullscreen()
{
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void bg_init_shared(GLFWwindow* window)
{
    glfwGetFramebufferSize(window, &screen_w, &screen_h);

    // Quad
    float quad[] = {
        -1, -1, 0, 0, 1, -1, 1, 0, 1, 1, 1, 1, -1, -1, 0, 0, 1, 1, 1, 1, -1, 1, 0, 1,
    };
    glGenVertexArrays(1, &quad_vao);
    glBindVertexArray(quad_vao);
    glGenBuffers(1, &quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // FBOs
    create_fbo(fbo_scene, tex_scene, screen_w, screen_h);
    create_fbo(fbo_bright, tex_bright, screen_w, screen_h);
    create_fbo(fbo_blur_h, tex_blur_h, screen_w, screen_h);
    create_fbo(fbo_blur_v, tex_blur_v, screen_w, screen_h);

    // Bloom shaders
    bright_program = InitShader("../shaders/vshader_quad.glsl", "../shaders/fshader_bright.glsl");
    blur_program = InitShader("../shaders/vshader_quad.glsl", "../shaders/fshader_blur.glsl");
    bloom_program = InitShader("../shaders/vshader_quad.glsl", "../shaders/fshader_bloom.glsl");

    blur_horizontal_loc = glGetUniformLocation(blur_program, "uHorizontal");
    bloom_scene_loc = glGetUniformLocation(bloom_program, "uScene");
    bloom_blur_loc = glGetUniformLocation(bloom_program, "uBloom");

    bg_init_escher();
    bg_init_julia();
    bg_init_tunnel();
    bg_init_gyroid();
    bg_init_lattice();
    // bg_init_godray();
    // bg_init_voronoi();
    // bg_init_sierpinski();
}

void bg_begin_scene()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_scene);
    glViewport(0, 0, screen_w, screen_h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
}

void bg_end_scene()
{
    // Pass 2: extract bright
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_bright);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(bright_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_scene);
    glUniform1i(glGetUniformLocation(bright_program, "uScene"), 0);
    glBindVertexArray(quad_vao);
    glDisable(GL_DEPTH_TEST);
    draw_quad_fullscreen();

    // Pass 3: horizontal blur
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_blur_h);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(blur_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_bright);
    glUniform1i(glGetUniformLocation(blur_program, "uScene"), 0);
    glUniform1i(blur_horizontal_loc, 1);
    draw_quad_fullscreen();

    // Pass 4: vertical blur
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_blur_v);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, tex_blur_h);
    glUniform1i(blur_horizontal_loc, 0);
    draw_quad_fullscreen();

    // Final: combine
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screen_w, screen_h);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(bloom_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_scene);
    glUniform1i(bloom_scene_loc, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex_blur_v);
    glUniform1i(bloom_blur_loc, 1);
    draw_quad_fullscreen();

    glEnable(GL_DEPTH_TEST);
}

void bg_init_julia()
{
    julia_program = InitShader("../shaders/vshader_quad.glsl", "../shaders/fshader_julia.glsl");
    julia_time_loc = glGetUniformLocation(julia_program, "uTime");
    julia_aspect_loc = glGetUniformLocation(julia_program, "uAspect");
    julia_c_loc = glGetUniformLocation(julia_program, "uC");
    julia_zoom_loc = glGetUniformLocation(julia_program, "uZoom");
}

void bg_draw_julia(float time)
{
    float t = time * 0.12f;
    float c_real = 0.7885f * cosf(t);
    float c_imag = 0.7885f * sinf(t);

    glUseProgram(julia_program);
    glUniform1f(julia_time_loc, time);
    glUniform1f(julia_aspect_loc, (float)screen_w / screen_h);
    glUniform2f(julia_c_loc, c_real, c_imag);
    glUniform1f(julia_zoom_loc, 1.5f);

    glBindVertexArray(quad_vao);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    draw_quad_fullscreen();
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void bg_init_escher()
{
    escher_program = InitShader("../shaders/vshader_quad.glsl", "../shaders/fshader_bg.glsl");
    escher_eye_loc = glGetUniformLocation(escher_program, "uEyePos");
    escher_right_loc = glGetUniformLocation(escher_program, "uCamRight");
    escher_up_loc = glGetUniformLocation(escher_program, "uCamUp");
    escher_forward_loc = glGetUniformLocation(escher_program, "uCamForward");
    escher_view_size_loc = glGetUniformLocation(escher_program, "uViewSize");
    escher_time_loc = glGetUniformLocation(escher_program, "uTime");
}

void bg_draw_escher(vec3 eye, vec3 right, vec3 up, vec3 forward, float view_size, float time)
{
    glUseProgram(escher_program);

    // Upload all camera info the shader needs to reconstruct world rays
    glUniform3fv(escher_eye_loc, 1, &eye.x);
    glUniform3fv(escher_right_loc, 1, &right.x);
    glUniform3fv(escher_up_loc, 1, &up.x);
    glUniform3fv(escher_forward_loc, 1, &forward.x);
    glUniform1f(escher_view_size_loc, view_size);
    glUniform1f(escher_time_loc, time);

    // Floor sits just below the polygon
    // float floor_y = -radius - 0.3f;
    // glUniform1f(bg_floor_y_loc, floor_y);

    glBindVertexArray(quad_vao);
    glDisable(GL_DEPTH_TEST);  // background never occludes anything
    glDepthMask(GL_FALSE);     // don't write to depth buffer
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    // Restore program for the bars
    // glUseProgram(program);
}

void bg_init_tunnel()
{
    tunnel_program = InitShader("../shaders/vshader_quad.glsl", "../shaders/fshader_tunnel.glsl");

    tunnel_eye_loc = glGetUniformLocation(tunnel_program, "uEyePos");
    tunnel_right_loc = glGetUniformLocation(tunnel_program, "uCamRight");
    tunnel_up_loc = glGetUniformLocation(tunnel_program, "uCamUp");
    tunnel_forward_loc = glGetUniformLocation(tunnel_program, "uCamForward");
    tunnel_time_loc = glGetUniformLocation(tunnel_program, "uTime");
    tunnel_aspect_loc = glGetUniformLocation(tunnel_program, "uAspect");
}

void bg_draw_tunnel(vec3 eye, vec3 right, vec3 up, vec3 forward, float time)
{
    glUseProgram(tunnel_program);

    // Pass the camera vectors to define the ray directions
    glUniform3fv(tunnel_eye_loc, 1, &eye.x);
    glUniform3fv(tunnel_right_loc, 1, &right.x);
    glUniform3fv(tunnel_up_loc, 1, &up.x);
    glUniform3fv(tunnel_forward_loc, 1, &forward.x);

    glUniform1f(tunnel_time_loc, time);
    glUniform1f(tunnel_aspect_loc, (float)screen_w / screen_h);

    glBindVertexArray(quad_vao);

    // Standard 2D background states (drawn behind 3D geometry)
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    draw_quad_fullscreen();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void bg_init_gyroid()
{
    gyroid_program = InitShader("../shaders/vshader_quad.glsl", "../shaders/fshader_gyroid.glsl");

    gyroid_eye_loc = glGetUniformLocation(gyroid_program, "uEyePos");
    gyroid_right_loc = glGetUniformLocation(gyroid_program, "uCamRight");
    gyroid_up_loc = glGetUniformLocation(gyroid_program, "uCamUp");
    gyroid_forward_loc = glGetUniformLocation(gyroid_program, "uCamForward");
    gyroid_time_loc = glGetUniformLocation(gyroid_program, "uTime");
    gyroid_aspect_loc = glGetUniformLocation(gyroid_program, "uAspect");
}

void bg_draw_gyroid(vec3 eye, vec3 right, vec3 up, vec3 forward, float time)
{
    glUseProgram(gyroid_program);

    glUniform3fv(gyroid_eye_loc, 1, &eye.x);
    glUniform3fv(gyroid_right_loc, 1, &right.x);
    glUniform3fv(gyroid_up_loc, 1, &up.x);
    glUniform3fv(gyroid_forward_loc, 1, &forward.x);

    glUniform1f(gyroid_time_loc, time);
    glUniform1f(gyroid_aspect_loc, (float)screen_w / screen_h);

    glBindVertexArray(quad_vao);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    draw_quad_fullscreen();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void bg_init_lattice()
{
    lattice_program = InitShader("../shaders/vshader_quad.glsl", "../shaders/fshader_lattice.glsl");

    lattice_eye_loc = glGetUniformLocation(lattice_program, "uEyePos");
    lattice_right_loc = glGetUniformLocation(lattice_program, "uCamRight");
    lattice_up_loc = glGetUniformLocation(lattice_program, "uCamUp");
    lattice_forward_loc = glGetUniformLocation(lattice_program, "uCamForward");
    lattice_time_loc = glGetUniformLocation(lattice_program, "uTime");
    lattice_aspect_loc = glGetUniformLocation(lattice_program, "uAspect");
}

void bg_draw_lattice(vec3 eye, vec3 right, vec3 up, vec3 forward, float view_size, float time)
{
    glUseProgram(lattice_program);

    glUniform3fv(lattice_eye_loc, 1, &eye.x);
    glUniform3fv(lattice_right_loc, 1, &right.x);
    glUniform3fv(lattice_up_loc, 1, &up.x);
    glUniform3fv(lattice_forward_loc, 1, &forward.x);

    glUniform1f(lattice_time_loc, time);
    glUniform1f(lattice_aspect_loc, (float)screen_w / screen_h);

    glBindVertexArray(quad_vao);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    draw_quad_fullscreen();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}