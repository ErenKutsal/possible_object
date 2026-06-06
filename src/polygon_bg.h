#pragma once
#include "includes.h"

// ─────────────────────────────────────────────────────────────────────────────
// polygon_bg — bloom post-processing pipeline + interactive backgrounds
//              for the Impossible Polygon (slot 0).
//
// Call order each frame inside polygon_display():
//   1. polygon_bg_begin_scene()          — bind scene FBO, clear
//   2. polygon_bg_draw(...)              — draw the chosen background
//   3. <draw polygon normally>
//   4. polygon_bg_end_scene()            — bloom passes → blit to screen
//
// Background selection is automatic based on num_segments:
//   (num_segments - 3) % 4
//   0 → Escher corridor   (triangle)
//   1 → Julia fractal     (square)
//   2 → Gyroid SDF        (pentagon)
//   3 → Neon N-gon tunnel (hexagon+)
// ─────────────────────────────────────────────────────────────────────────────

// Call once after the GL context is current and screen_w / screen_h are set.
void polygon_bg_init();

// Bind the scene FBO and clear it.  Must be called before any polygon draws.
void polygon_bg_begin_scene();

// Draw the background for the given polygon size into the currently-bound FBO.
// eye / right / up / forward — camera basis from polygon_display().
// aspect — viewport width / height.
// time   — glfwGetTime().
// nSides — current num_segments (selects the background).
void polygon_bg_draw(vec3 eye, vec3 right, vec3 up, vec3 forward,
                     float aspect, float time, int nSides);

// Run bloom passes and composite the result to the default framebuffer.
void polygon_bg_end_scene();
