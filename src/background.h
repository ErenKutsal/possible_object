#ifndef BACKGROUND_H
#define BACKGROUND_H

#include "includes.h"

// ---- Shared state — defined in background.cpp, used everywhere ----
// extern int    screen_w, screen_h; // I inited those in main.cpp
extern GLuint quad_vao, quad_vbo;

// FBOs — moved out of polygon.cpp, owned here
extern GLuint fbo_scene, tex_scene;
extern GLuint fbo_bright, tex_bright;
extern GLuint fbo_blur_h, tex_blur_h;
extern GLuint fbo_blur_v, tex_blur_v;

// ---- Lifecycle ----
// Call once at startup after GL is initialized
void bg_init_shared(GLFWwindow* window);

// Call at the start of every display function (binds fbo_scene, clears it)
void bg_begin_scene();

// Call at the end of every display function (runs bloom, outputs to screen)
void bg_end_scene();

// ---- Per-background inits (call once per object's init) ----
void bg_init_escher();
void bg_init_julia();
void bg_init_tunnel();
void bg_init_gyroid();

void bg_init_sierpinski();
void bg_init_godray();
void bg_init_voronoi();
void bg_init_truchet();
void bg_init_plasma();
void bg_init_lattice();

// ---- Per-background draws (call inside bg_begin/end, before your geometry) ----
// Escher: needs camera info since it's spherical
void bg_draw_escher(vec3 eye, vec3 cam_right, vec3 cam_up, vec3 cam_forward, float view_size, float time);
void bg_draw_julia(float time);
void bg_draw_tunnel(vec3 eye, vec3 right, vec3 up, vec3 forward, float time);
void bg_draw_gyroid(vec3 eye, vec3 right, vec3 up, vec3 forward, float time);
void bg_draw_lattice(vec3 eye, vec3 right, vec3 up, vec3 forward, float view_size, float time);

void bg_draw_sierpinski(float time);
void bg_draw_voronoi(float time);
void bg_draw_truchet(float time);
void bg_draw_plasma(float time);

// God rays also needs the light screen position
void bg_draw_godray(float time, vec2 light_screen_pos);

#endif