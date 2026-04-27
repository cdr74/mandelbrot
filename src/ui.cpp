#include "ui.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <algorithm>
#include <cmath>
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

static void drawOrbitPortal(const MandelbrotState& ms, const OrbitState& orbit) {
    if (!orbit.active || orbit.orbit.empty()) return;

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    // Small crosshair on the fractal at the picked point c
    {
        double aspect = (double)ms.windowWidth / ms.windowHeight;
        float fx = (float)(((orbit.cx - ms.centerX) / (2.0 * ms.zoom * aspect) + 0.5) * ms.windowWidth);
        float fy = (float)(((orbit.cy - ms.centerY) / (2.0 * ms.zoom) + 0.5) * ms.windowHeight);
        const float cs = 7.0f;
        dl->AddLine(ImVec2(fx - cs, fy), ImVec2(fx + cs, fy), IM_COL32(255, 200, 50, 220), 1.5f);
        dl->AddLine(ImVec2(fx, fy - cs), ImVec2(fx, fy + cs), IM_COL32(255, 200, 50, 220), 1.5f);
        dl->AddCircle(ImVec2(fx, fy), 5.0f, IM_COL32(255, 200, 50, 170), 8, 1.5f);
    }

    // Portal geometry — bottom-right corner
    const float portalR    = 140.0f;
    const float pad        = 20.0f;
    const ImVec2 C((float)ms.windowWidth  - portalR - pad,
                   (float)ms.windowHeight - portalR - pad);

    // Independent coordinate system: always shows [-2.5, 2.5] x [-2.5, 2.5]
    // +Im is DOWN to match the fractal display convention
    const float viewExtent = 2.5f;
    const float scale      = portalR * 0.88f / viewExtent;

    // Convert complex coord -> portal pixel, clamped to keep everything inside the circle
    auto c2p = [&](double re, double im) -> ImVec2 {
        double lenSq = re * re + im * im;
        double cap   = viewExtent * 0.94;
        if (lenSq > cap * cap) {
            double len = std::sqrt(lenSq);
            re = re / len * cap;
            im = im / len * cap;
        }
        return ImVec2(C.x + (float)re * scale, C.y + (float)im * scale);
    };

    // Dark background
    dl->AddCircleFilled(C, portalR, IM_COL32(8, 10, 22, 248));

    // Subtle axes
    float ax = portalR * 0.92f;
    dl->AddLine(ImVec2(C.x - ax, C.y), ImVec2(C.x + ax, C.y), IM_COL32(65, 65, 75, 160), 1.0f);
    dl->AddLine(ImVec2(C.x, C.y - ax), ImVec2(C.x, C.y + ax), IM_COL32(65, 65, 75, 160), 1.0f);

    // Escape circle |z| = 2
    dl->AddCircle(C, 2.0f * scale, IM_COL32(130, 130, 160, 160), 64, 1.5f);

    // Orbit trail — fade older segments
    int n = std::min(orbit.displayIter + 1, (int)orbit.orbit.size());
    for (int i = 0; i + 1 < n; i++) {
        float frac    = n > 1 ? (float)i / (float)(n - 1) : 1.0f;
        uint8_t alpha = (uint8_t)(60 + frac * 170);
        dl->AddLine(c2p(orbit.orbit[i].first,     orbit.orbit[i].second),
                    c2p(orbit.orbit[i + 1].first, orbit.orbit[i + 1].second),
                    IM_COL32(220, 190, 55, alpha), 1.5f);
    }

    // Orbit dots
    for (int i = 0; i < n; i++) {
        ImVec2 p = c2p(orbit.orbit[i].first, orbit.orbit[i].second);
        bool isStart   = (i == 0);
        bool isEscaped = (orbit.escapeAt >= 0 && i == orbit.escapeAt);
        bool isLast    = (i == n - 1);
        ImU32 col; float r;
        if (isStart)        { col = IM_COL32(255, 255, 255, 245); r = 5.0f; }
        else if (isEscaped) { col = IM_COL32(255,  65,  65, 255); r = 6.0f; }
        else if (isLast)    { col = IM_COL32( 75, 220, 255, 255); r = 5.0f; }
        else                { col = IM_COL32( 65, 165, 210, 145); r = 3.0f; }
        dl->AddCircleFilled(p, r, col);
    }

    // c marker (the picked complex number)
    ImVec2 cp = c2p(orbit.cx, orbit.cy);
    const float cs = 5.5f;
    dl->AddCircle(cp, 4.5f, IM_COL32(255, 200, 50, 210), 8, 1.5f);
    dl->AddLine(ImVec2(cp.x - cs, cp.y), ImVec2(cp.x + cs, cp.y), IM_COL32(255, 200, 50, 190), 1.5f);
    dl->AddLine(ImVec2(cp.x, cp.y - cs), ImVec2(cp.x, cp.y + cs), IM_COL32(255, 200, 50, 190), 1.5f);

    // Portal border ring
    dl->AddCircle(C, portalR, IM_COL32(90, 115, 185, 230), 96, 2.0f);

    // Iteration label (small text at bottom of portal)
    char label[64];
    int  total = (int)orbit.orbit.size() - 1;
    if (orbit.escapeAt >= 0 && orbit.displayIter >= orbit.escapeAt)
        snprintf(label, sizeof(label), "z%d — escaped!", orbit.displayIter);
    else if (orbit.escapeAt >= 0)
        snprintf(label, sizeof(label), "z%d / %d  (escapes at z%d)", orbit.displayIter, total, orbit.escapeAt);
    else
        snprintf(label, sizeof(label), "z%d / %d", orbit.displayIter, total);
    float  fsz  = ImGui::GetFontSize() * 0.62f;
    ImVec2 tpos(C.x - portalR + 8.0f, C.y + portalR - fsz - 9.0f);
    ImU32  tcol = (orbit.escapeAt >= 0 && orbit.displayIter >= orbit.escapeAt)
                ? IM_COL32(255, 100, 100, 220)
                : IM_COL32(160, 160, 185, 210);
    dl->AddText(ImGui::GetFont(), fsz, tpos, tcol, label);
}

void UI::render(MandelbrotState& state, UIState& uiState, OrbitState& orbitState) {
    drawOrbitPortal(state, orbitState);

    if (!uiState.showOverlay) return;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(310, 0), ImGuiCond_FirstUseEver);

    ImGui::Begin("Mandelbrot Explorer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
    {
        ImVec2 p = ImGui::GetWindowPos(), s = ImGui::GetWindowSize();
        uiState.overlayX = p.x; uiState.overlayY = p.y;
        uiState.overlayW = s.x; uiState.overlayH = s.y;
    }

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

    ImGui::Separator();
    ImGui::Text("Orbit Visualization");

    if (!orbitState.active) {
        ImGui::TextDisabled("Right-click fractal to pick a point");
    } else {
        ImGui::Text("c = (%.5f, %.5fi)", orbitState.cx, orbitState.cy);

        int total = (int)orbitState.orbit.size() - 1;
        if (orbitState.escapeAt >= 0)
            ImGui::Text("Step %d / %d  (escapes at %d)", orbitState.displayIter, total, orbitState.escapeAt);
        else
            ImGui::Text("Step %d / %d  (bounded)", orbitState.displayIter, total);

        if (orbitState.computedMaxIter != state.maxIter)
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Max iter changed — re-pick to refresh");

        float bw3 = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2) / 3.0f;
        if (ImGui::Button(orbitState.playing ? "Pause" : "Play", ImVec2(bw3, 0)))
            orbitState.playing = !orbitState.playing;
        ImGui::SameLine();
        if (ImGui::Button("Restart", ImVec2(bw3, 0))) {
            orbitState.displayIter = 0;
            orbitState.timer       = 0.0f;
            orbitState.pauseTimer  = 0.0f;
            orbitState.playing     = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear", ImVec2(bw3, 0)))
            orbitState.clear();

        ImGui::Text("Speed");
        ImGui::SliderFloat("##orbitspeed", &orbitState.speed, 0.5f, 30.0f, "%.1f iter/s");
        ImGui::Checkbox("Loop", &orbitState.looping);
    }

    ImGui::Separator();
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
