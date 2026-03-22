#include "ui.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <cstdio>
#include <ctime>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// glReadPixels is a core GL 1.0 function in opengl32.lib but not in our minimal glad
#ifdef _WIN32
extern "C" void __stdcall glReadPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*);
#endif

static const char* colorSchemeNames[] = {
    "Classic", "Fire", "Ocean", "Grayscale", "Psychedelic", "Ultra"
};

void UI::applyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]       = ImVec4(0.10f, 0.10f, 0.18f, 0.90f);
    colors[ImGuiCol_TitleBg]        = ImVec4(0.10f, 0.10f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgActive]  = ImVec4(0.15f, 0.15f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBg]        = ImVec4(0.15f, 0.15f, 0.25f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.35f, 0.80f);
    colors[ImGuiCol_FrameBgActive]  = ImVec4(0.25f, 0.25f, 0.40f, 1.00f);
    colors[ImGuiCol_SliderGrab]        = ImVec4(0.00f, 0.83f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]  = ImVec4(0.00f, 0.70f, 0.85f, 1.00f);
    colors[ImGuiCol_Button]        = ImVec4(0.15f, 0.15f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.83f, 1.00f, 0.40f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(0.00f, 0.83f, 1.00f, 0.60f);
    colors[ImGuiCol_Header]        = ImVec4(0.00f, 0.83f, 1.00f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.83f, 1.00f, 0.80f);
    colors[ImGuiCol_HeaderActive]  = ImVec4(0.00f, 0.83f, 1.00f, 1.00f);
    colors[ImGuiCol_CheckMark]     = ImVec4(0.00f, 0.83f, 1.00f, 1.00f);
}

void UI::initialize() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding  = 3.0f;
    style.WindowPadding  = ImVec2(15, 15);
    style.FramePadding   = ImVec2(8, 4);
    style.ItemSpacing    = ImVec2(12, 8);
    style.ScrollbarSize  = 15.0f;

    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.5f;
}

void UI::render(MandelbrotState& state, UIState& uiState) {
    if (!uiState.showOverlay) return;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(310, 0), ImGuiCond_FirstUseEver);

    ImGui::Begin("Mandelbrot Explorer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

    // Max iterations slider
    ImGui::Text("Max Iterations");
    ImGui::SliderInt("##maxiter", &state.maxIter, 50, 2000);

    // Color scheme dropdown
    ImGui::Text("Color Scheme");
    int scheme = (int)state.colorScheme;
    if (ImGui::BeginCombo("##colorscheme", colorSchemeNames[scheme])) {
        for (int i = 0; i < (int)ColorScheme::COUNT; i++) {
            bool selected = (scheme == i);
            if (ImGui::Selectable(colorSchemeNames[i], selected))
                state.colorScheme = (ColorScheme)i;
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    // Precision warning
    if (state.zoom < 1e-5f) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Float precision limit reached");
    }

    // Frame time
    ImGui::Text("Frame:  %.1f ms", 1000.0f * io.DeltaTime);

    ImGui::Separator();

    // Zoom buttons
    float bw = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2) / 3.0f;
    if (ImGui::Button("Zoom In", ImVec2(bw, 0))) {
        double mx = state.windowWidth  / 2.0;
        double my = state.windowHeight / 2.0;
        state.zoomAtPixel(mx, my, 0.8f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Zoom Out", ImVec2(bw, 0))) {
        double mx = state.windowWidth  / 2.0;
        double my = state.windowHeight / 2.0;
        state.zoomAtPixel(mx, my, 1.25f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset", ImVec2(bw, 0))) {
        state.reset();
    }

    // Cycle colors + Save JPG
    float hw = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.0f;
    bool cycling = state.cyclingColors;
    if (cycling)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.60f, 0.30f, 1.00f));
    if (ImGui::Button(cycling ? "Cycling..." : "Cycle Colors", ImVec2(hw, 0)))
        state.cyclingColors = !state.cyclingColors;
    if (cycling)
        ImGui::PopStyleColor();
    ImGui::SameLine();
    if (ImGui::Button("Save JPG", ImVec2(hw, 0))) {
        uiState.pendingSave = true;
    }

    if (uiState.saveStatusTimer > 0.0f) {
        uiState.saveStatusTimer -= io.DeltaTime;
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "%s", uiState.saveStatus.c_str());
    }

    ImGui::Text("H = toggle overlay  |  ESC = quit");

    ImGui::End();
}

std::string saveFramebuffer(int width, int height) {
    std::vector<uint8_t> pixels(width * height * 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Flip vertically — OpenGL origin is bottom-left
    for (int y = 0; y < height / 2; y++) {
        uint8_t* top = pixels.data() + y * width * 3;
        uint8_t* bot = pixels.data() + (height - 1 - y) * width * 3;
        for (int x = 0; x < width * 3; x++)
            std::swap(top[x], bot[x]);
    }

    time_t now = time(nullptr);
    tm* t = localtime(&now);
    char filename[64];
    snprintf(filename, sizeof(filename), "mandelbrot_%04d%02d%02d_%02d%02d%02d.jpg",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);

    stbi_write_jpg(filename, width, height, 3, pixels.data(), 95);
    return std::string(filename);
}
