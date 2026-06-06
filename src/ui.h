#pragma once
#include "includes.h"

enum class AppState
{
    TITLE,
    LEVEL_SELECT,
    IN_SHAPE
};

void ui_init(GLFWwindow* window);
void ui_shutdown();
void ui_begin_frame();
void ui_end_frame();

// Mutates state and selected_shape based on user clicks.
void ui_draw_menu(AppState& state, int& selected_shape, GLFWwindow* window);

// True while the mouse is over an ImGui widget — callers should skip
// scene mouse handling when this is set.
bool ui_wants_mouse();
bool ui_wants_keyboard();
