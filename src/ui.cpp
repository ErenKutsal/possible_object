#include "ui.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#define SHRINE_COUNT 9
static const char* SHRINE_THEMES[SHRINE_COUNT] = {
    "Floral", "Stone", "Water", "Fire", "Cosmic", "Forge",
    "Glass",  "Earth", "Iron",
};

static const char* SHRINE_NAMES[SHRINE_COUNT] = {
    "Impossible Polygon",      // 0
    "Penrose Triangle",        // 1 — OBJ
    "Blocked Penrose (Blender)", // 2 — OBJ pair of #1 (Paradox block variant)
    "Impossible Cube",         // 3 — OBJ
    "Necker Cube",             // 4 — procedural pair of #3
    "Impossible Arch",         // 5 — OBJ
    "Impossible Arch (proc)",  // 6 — procedural pair of #5
    "Penrose Stair",           // 7 — OBJ
    "Reutersvard Rectangle",   // 8 — OBJ
};

static const ImVec4 SHRINE_TINTS[SHRINE_COUNT] = {
    ImVec4(0.55f, 0.72f, 0.45f, 1.0f),  // Floral — mossy green
    ImVec4(0.62f, 0.62f, 0.60f, 1.0f),  // Stone — neutral grey
    ImVec4(0.40f, 0.62f, 0.78f, 1.0f),  // Water — slate blue
    ImVec4(0.85f, 0.50f, 0.30f, 1.0f),  // Fire — ember orange
    ImVec4(0.55f, 0.45f, 0.78f, 1.0f),  // Cosmic — violet
    ImVec4(0.80f, 0.70f, 0.45f, 1.0f),  // Forge — brass / warm gold
    ImVec4(0.65f, 0.78f, 0.82f, 1.0f),  // Glass — pale cyan
    ImVec4(0.62f, 0.48f, 0.36f, 1.0f),  // Earth — warm sienna
    ImVec4(0.55f, 0.55f, 0.60f, 1.0f),  // Iron — steel grey
};

void ui_init(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // don't write imgui.ini

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.FramePadding = ImVec2(10, 8);
    style.ItemSpacing = ImVec2(10, 10);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
}

void ui_shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ui_begin_frame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ui_end_frame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool ui_wants_mouse() { return ImGui::GetIO().WantCaptureMouse; }
bool ui_wants_keyboard() { return ImGui::GetIO().WantCaptureKeyboard; }

// ------------------------------------------------
// Helpers
// ------------------------------------------------
static void centered_text(const char* text, float scale)
{
    ImGui::SetWindowFontScale(scale);
    ImVec2 size = ImGui::CalcTextSize(text);
    float win_width = ImGui::GetWindowSize().x;
    ImGui::SetCursorPosX((win_width - size.x) * 0.5f);
    ImGui::TextUnformatted(text);
    ImGui::SetWindowFontScale(1.0f);
}

static bool centered_button(const char* label, ImVec2 size)
{
    float win_width = ImGui::GetWindowSize().x;
    ImGui::SetCursorPosX((win_width - size.x) * 0.5f);
    return ImGui::Button(label, size);
}

static ImGuiWindowFlags fullscreen_window_flags()
{
    return ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground |
           ImGuiWindowFlags_NoScrollbar;
}

// ------------------------------------------------
// Title screen
// ------------------------------------------------
static void draw_title(AppState& state, GLFWwindow* window)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);

    ImGui::Begin("##title", nullptr, fullscreen_window_flags());

    float h = ImGui::GetWindowSize().y;
    ImGui::Dummy(ImVec2(0, h * 0.18f));

    centered_text("Impossible Shapes", 3.2f);
    ImGui::Dummy(ImVec2(0, 8));
    centered_text("An interactive showcase of optical illusions", 1.0f);

    ImGui::Dummy(ImVec2(0, h * 0.12f));

    ImVec2 btn(240, 48);
    if (centered_button("Begin Journey", btn))
    {
        state = AppState::IN_SHAPE;
    }
    if (centered_button("Shrine Select", btn))
    {
        state = AppState::SHRINE_SELECT;
    }
    if (centered_button("Quit", btn))
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    ImGui::End();
}

// ------------------------------------------------
// Shrine select
// ------------------------------------------------
static void draw_shrine_select(AppState& state, int& selected_shape)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);

    ImGui::Begin("##select", nullptr, fullscreen_window_flags());

    ImGui::Dummy(ImVec2(0, 32));
    centered_text("Shrine Select", 2.5f);
    ImGui::Dummy(ImVec2(0, 24));

    // Card layout: SHRINE_COUNT cards in a row, fall back to multiple rows if narrow.
    const float card_w = 150.0f;
    const float card_h = 200.0f;
    const float gap = 14.0f;
    float win_width = ImGui::GetWindowSize().x;
    int per_row = (int)((win_width + gap) / (card_w + gap));
    if (per_row < 1) per_row = 1;
    if (per_row > SHRINE_COUNT) per_row = SHRINE_COUNT;

    int rows = (SHRINE_COUNT + per_row - 1) / per_row;
    for (int r = 0; r < rows; r++)
    {
        int first = r * per_row;
        int last = (first + per_row > SHRINE_COUNT) ? SHRINE_COUNT : first + per_row;
        int count = last - first;
        float row_width = count * card_w + (count - 1) * gap;
        ImGui::SetCursorPosX((win_width - row_width) * 0.5f);

        for (int i = first; i < last; i++)
        {
            if (i > first) ImGui::SameLine(0.0f, gap);

            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(SHRINE_TINTS[i].x * 0.35f, SHRINE_TINTS[i].y * 0.35f,
                                                          SHRINE_TINTS[i].z * 0.35f, 0.55f));
            ImGui::BeginChild("##card", ImVec2(card_w, card_h), true,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            ImGui::PushStyleColor(ImGuiCol_Text, SHRINE_TINTS[i]);
            ImGui::SetWindowFontScale(1.6f);
            ImGui::TextUnformatted(SHRINE_THEMES[i]);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::TextDisabled("Shape %d", i + 1);
            ImGui::Spacing();
            ImGui::TextWrapped("%s", SHRINE_NAMES[i]);

            // Push the button to the bottom of the card.
            float remaining = ImGui::GetContentRegionAvail().y - 40;
            if (remaining > 0) ImGui::Dummy(ImVec2(0, remaining));

            if (ImGui::Button("Enter", ImVec2(-FLT_MIN, 36)))
            {
                selected_shape = i;
                state = AppState::IN_SHAPE;
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
    }

    ImGui::Dummy(ImVec2(0, 24));
    if (centered_button("Back", ImVec2(160, 40)))
    {
        state = AppState::TITLE;
    }

    ImGui::End();
}

void ui_draw_menu(AppState& state, int& selected_shape, GLFWwindow* window)
{
    if (state == AppState::TITLE)
        draw_title(state, window);
    else if (state == AppState::SHRINE_SELECT)
        draw_shrine_select(state, selected_shape);
}
