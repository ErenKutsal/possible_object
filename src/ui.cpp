#include "ui.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// stb_image — vendored single-header PNG/JPEG loader (FetchContent in CMake).
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstdio>

#define SHRINE_COUNT 8
static const char* SHRINE_THEMES[SHRINE_COUNT] = {
    "Floral", "Stone", "Water", "Fire", "Cosmic", "Forge", "Glass", "Iron",
};

static const char* SHRINE_NAMES[SHRINE_COUNT] = {
    "Impossible Polygon",         // 0
    "Penrose Triangle",           // 1 — OBJ
    "Blocked Penrose (Blender)",  // 2 — OBJ pair of #1
    "Impossible Cube",            // 3 — OBJ
    "Impossible Arch",            // 4 — OBJ (tall narrow)
    "Impossible Arch (round)",    // 5 — sphere-cast pair of #4
    "Penrose Stair",              // 6 — OBJ
    "Reutersvard Rectangle",      // 7 — OBJ
};

static const ImVec4 SHRINE_TINTS[SHRINE_COUNT] = {
    ImVec4(0.55f, 0.72f, 0.45f, 1.0f),  // Floral — mossy green
    ImVec4(0.62f, 0.62f, 0.60f, 1.0f),  // Stone — neutral grey
    ImVec4(0.40f, 0.62f, 0.78f, 1.0f),  // Water — slate blue
    ImVec4(0.85f, 0.50f, 0.30f, 1.0f),  // Fire — ember orange
    ImVec4(0.55f, 0.45f, 0.78f, 1.0f),  // Cosmic — violet
    ImVec4(0.80f, 0.70f, 0.45f, 1.0f),  // Forge — brass / warm gold
    ImVec4(0.65f, 0.78f, 0.82f, 1.0f),  // Glass — pale cyan
    ImVec4(0.55f, 0.55f, 0.60f, 1.0f),  // Iron — steel grey
};

// ── Landing background image (Escher-style impossible architecture) ──────
// Loaded once at ui_init from renders/landing_bg.jpg, drawn full-bleed
// behind the title menu via ImGui's background draw list.
static GLuint g_landing_tex = 0;
static int    g_landing_w   = 0;
static int    g_landing_h   = 0;

static void load_landing_texture(const char* path)
{
    int channels = 0;
    unsigned char* data = stbi_load(path, &g_landing_w, &g_landing_h, &channels, 4);
    if (!data) {
        fprintf(stderr, "[ui] failed to load landing image: %s (%s)\n",
                path, stbi_failure_reason());
        return;
    }
    glGenTextures(1, &g_landing_tex);
    glBindTexture(GL_TEXTURE_2D, g_landing_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_landing_w, g_landing_h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
}

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

    // Background image lives under <project>/renders/, and the binary runs
    // from <project>/build/, so the relative path goes up one and into renders/.
    load_landing_texture("../renders/landing_bg.jpg");
}

void ui_shutdown()
{
    if (g_landing_tex) {
        glDeleteTextures(1, &g_landing_tex);
        g_landing_tex = 0;
    }
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

// ─── Layout helpers ──────────────────────────────────────────────────────
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

// Draw the Escher landing image full-bleed plus a soft vertical darkening
// so overlay text and buttons read clearly against the busy stone scene.
static void draw_landing_background()
{
    if (!g_landing_tex) return;
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 sz = io.DisplaySize;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->AddImage((ImTextureID)(intptr_t)g_landing_tex,
                 ImVec2(0, 0), sz,
                 ImVec2(0, 0), ImVec2(1, 1));
    // Vertical gradient darkening (top lighter → bottom darker) so the
    // bottom-right button cluster sits on a deeper backdrop. Picked to read
    // as a soft vignette, not a hard overlay.
    dl->AddRectFilledMultiColor(
        ImVec2(0, 0), sz,
        IM_COL32(0,  0,  0,  60),  IM_COL32(0,  0,  0,  60),
        IM_COL32(0,  0, 10, 170),  IM_COL32(0,  0, 10, 170));
}

// ─── Title screen ────────────────────────────────────────────────────────
static void draw_title(AppState& state, GLFWwindow* window)
{
    draw_landing_background();

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 sz = io.DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(sz);
    ImGui::Begin("##title", nullptr, fullscreen_window_flags());

    // ── Title — upper-left, over the cyan light pool. No subtitle, no
    // panel; just the words sitting in the lit corner of the render.
    const ImVec4 cream   = ImVec4(0.96f, 0.92f, 0.83f, 1.0f);
    const ImVec4 dimcream= ImVec4(0.84f, 0.78f, 0.68f, 0.92f);
    const ImVec4 magenta = ImVec4(0.94f, 0.46f, 0.66f, 1.00f);
    const ImVec4 cyan    = ImVec4(0.50f, 0.86f, 0.90f, 1.00f);

    ImGui::SetCursorPos(ImVec2(72, 72));
    ImGui::PushStyleColor(ImGuiCol_Text, cream);
    ImGui::SetWindowFontScale(3.6f);
    ImGui::TextUnformatted("Impossible Shapes");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // ── Minimal direction bar pinned to the very bottom of the window.
    // A thin translucent strip with the three key hints inline, separated
    // by faint divider dots. Reads like an editor status bar — present
    // but quiet enough to step aside for the Escher render behind it.
    const float barH    = 44.0f;
    const float barPadX = 32.0f;
    const ImVec2 barTL(0.0f,        sz.y - barH);
    const ImVec2 barBR(sz.x,        sz.y);
    ImGui::GetWindowDrawList()->AddRectFilled(
        barTL, barBR, IM_COL32(8, 10, 16, 190), 0.0f);
    // Hairline brass rule along the top edge — picks up the gold rails.
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(0.0f, sz.y - barH),
        ImVec2(sz.x, sz.y - barH),
        IM_COL32(180, 145, 90, 110), 1.0f);

    // Stack the hints from the LEFT of the bar, vertically centered.
    const float textY = sz.y - barH + (barH - 16) * 0.5f;
    float xCur = barPadX;
    auto draw_pair = [&](const char* key, const ImVec4& keyCol, const char* desc, bool last) {
        ImGui::SetCursorPos(ImVec2(xCur, textY));
        ImGui::PushStyleColor(ImGuiCol_Text, keyCol);
        ImGui::TextUnformatted(key);
        ImGui::PopStyleColor();
        xCur += ImGui::CalcTextSize(key).x + 10.0f;

        ImGui::SetCursorPos(ImVec2(xCur, textY));
        ImGui::PushStyleColor(ImGuiCol_Text, cream);
        ImGui::TextUnformatted(desc);
        ImGui::PopStyleColor();
        xCur += ImGui::CalcTextSize(desc).x + 22.0f;

        if (!last) {
            ImGui::SetCursorPos(ImVec2(xCur, textY));
            ImGui::PushStyleColor(ImGuiCol_Text, dimcream);
            ImGui::TextUnformatted("·");
            ImGui::PopStyleColor();
            xCur += ImGui::CalcTextSize("·").x + 22.0f;
        }
    };
    draw_pair("ENTER", magenta, "begin journey",  false);
    draw_pair("M",     cyan,    "shrine select",  true);

    ImGui::End();
}

// ─── Shrine select ───────────────────────────────────────────────────────
// Same Escher backdrop and minimal bottom bar as the landing — the two
// screens read as one continuous antechamber. Cards in a 4×2 grid with the
// brass-rim translucent-panel treatment.
static void draw_shrine_select(AppState& state, int& selected_shape)
{
    draw_landing_background();

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 sz = io.DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(sz);
    ImGui::Begin("##select", nullptr, fullscreen_window_flags());

    const ImVec4 cream    = ImVec4(0.96f, 0.92f, 0.83f, 1.0f);
    const ImVec4 dimcream = ImVec4(0.84f, 0.78f, 0.68f, 0.92f);
    const ImVec4 magenta  = ImVec4(0.94f, 0.46f, 0.66f, 1.00f);
    const ImVec4 cyan     = ImVec4(0.50f, 0.86f, 0.90f, 1.00f);
    const ImU32  panelBg  = IM_COL32(10, 12, 18, 195);
    const ImU32  panelRim = IM_COL32(180, 145, 90, 110);

    // ── Title, upper-left to mirror the landing layout.
    ImGui::SetCursorPos(ImVec2(72, 72));
    ImGui::PushStyleColor(ImGuiCol_Text, cream);
    ImGui::SetWindowFontScale(3.0f);
    ImGui::TextUnformatted("Shrine Select");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // ── 4 × 2 card grid, centered horizontally, sat below the title.
    const int   cols   = 4;
    const int   rows   = 2;
    const float cardW  = 220.0f;
    const float cardH  = 200.0f;
    const float gapX   = 18.0f;
    const float gapY   = 18.0f;
    const float gridW  = cols * cardW + (cols - 1) * gapX;
    const float gridX0 = (sz.x - gridW) * 0.5f;
    const float gridY0 = 200.0f;

    for (int i = 0; i < SHRINE_COUNT; i++)
    {
        int   r = i / cols;
        int   c = i % cols;
        ImVec2 tl(gridX0 + c * (cardW + gapX), gridY0 + r * (cardH + gapY));
        ImVec2 br(tl.x + cardW, tl.y + cardH);

        // Translucent dark panel with brass rim — same recipe as the landing.
        ImGui::GetWindowDrawList()->AddRectFilled(tl, br, panelBg, 14.0f);
        ImGui::GetWindowDrawList()->AddRect    (tl, br, panelRim, 14.0f, 0, 1.5f);

        // Make the whole card the clickable button.
        ImGui::SetCursorPos(tl);
        ImGui::PushID(i);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.05f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f, 1.0f, 1.0f, 0.10f));
        if (ImGui::Button("##card", ImVec2(cardW, cardH))) {
            selected_shape = i;
            state          = AppState::IN_SHAPE;
        }
        ImGui::PopStyleColor(3);

        // Card body: shrine theme (tinted), shape number (cream small), shape
        // name wrapped. Painted via ImDrawList so they don't intercept the
        // button click.
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float pad = 18.0f;
        // Theme heading — bigger, in the shrine's hue
        ImGui::PushFont(ImGui::GetFont());
        dl->AddText(NULL, 22.0f, ImVec2(tl.x + pad, tl.y + pad),
                    ImGui::ColorConvertFloat4ToU32(SHRINE_TINTS[i]),
                    SHRINE_THEMES[i]);
        // Slot label
        char slot[16];
        snprintf(slot, sizeof(slot), "Shape %d", i + 1);
        dl->AddText(NULL, 13.0f, ImVec2(tl.x + pad, tl.y + pad + 36),
                    ImGui::ColorConvertFloat4ToU32(dimcream), slot);
        // Shape name (cream)
        dl->AddText(NULL, 14.0f, ImVec2(tl.x + pad, tl.y + pad + 68),
                    ImGui::ColorConvertFloat4ToU32(cream), SHRINE_NAMES[i]);
        // Small key hint at bottom-right of card
        char keyHint[8];
        snprintf(keyHint, sizeof(keyHint), "%d", i + 1);
        ImVec2 keySz = ImGui::CalcTextSize(keyHint);
        dl->AddText(NULL, 18.0f,
                    ImVec2(br.x - pad - keySz.x - 4, br.y - pad - 20),
                    ImGui::ColorConvertFloat4ToU32(cyan), keyHint);
        ImGui::PopFont();

        ImGui::PopID();
    }

    // ── Minimal bottom bar — same recipe as landing, hint set for this screen.
    const float barH = 44.0f;
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(0, sz.y - barH), ImVec2(sz.x, sz.y),
        IM_COL32(8, 10, 16, 190), 0.0f);
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(0,     sz.y - barH),
        ImVec2(sz.x, sz.y - barH),
        IM_COL32(180, 145, 90, 110), 1.0f);

    const float textY  = sz.y - barH + (barH - 16) * 0.5f;
    const float barPad = 32.0f;
    float xCur = barPad;
    auto draw_pair = [&](const char* key, const ImVec4& keyCol, const char* desc, bool last) {
        ImGui::SetCursorPos(ImVec2(xCur, textY));
        ImGui::PushStyleColor(ImGuiCol_Text, keyCol);
        ImGui::TextUnformatted(key);
        ImGui::PopStyleColor();
        xCur += ImGui::CalcTextSize(key).x + 10.0f;
        ImGui::SetCursorPos(ImVec2(xCur, textY));
        ImGui::PushStyleColor(ImGuiCol_Text, cream);
        ImGui::TextUnformatted(desc);
        ImGui::PopStyleColor();
        xCur += ImGui::CalcTextSize(desc).x + 22.0f;
        if (!last) {
            ImGui::SetCursorPos(ImVec2(xCur, textY));
            ImGui::PushStyleColor(ImGuiCol_Text, dimcream);
            ImGui::TextUnformatted("·");
            ImGui::PopStyleColor();
            xCur += ImGui::CalcTextSize("·").x + 22.0f;
        }
    };
    draw_pair("1-8",   cyan,    "enter shape", false);
    draw_pair("ESC",   magenta, "back",        true);

    ImGui::End();
}

void ui_draw_menu(AppState& state, int& selected_shape, GLFWwindow* window)
{
    if (state == AppState::TITLE)
        draw_title(state, window);
    else if (state == AppState::SHRINE_SELECT)
        draw_shrine_select(state, selected_shape);
}
