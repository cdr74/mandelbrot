#pragma once
#include "mandelbrot.h"
#include <string>

struct UIState {
    bool showOverlay = true;
    bool pendingSave = false;
    std::string saveStatus;
    float saveStatusTimer = 0.0f;
    // Overlay bounds (updated each frame) — used by cursor-pos callback
    float overlayX = 20, overlayY = 20, overlayW = 350, overlayH = 300;
};

class UI {
public:
    void applyTheme();
    void initialize();
    void render(MandelbrotState& state, UIState& uiState, OrbitState& orbitState);
};

std::string saveFramebuffer(int width, int height);
