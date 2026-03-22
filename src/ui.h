#pragma once
#include "mandelbrot.h"
#include <string>

struct UIState {
    bool showOverlay = true;
    bool pendingSave = false;
    std::string saveStatus;
    float saveStatusTimer = 0.0f;
};

class UI {
public:
    void applyTheme();
    void initialize();
    void render(MandelbrotState& state, UIState& uiState);
};

std::string saveFramebuffer(int width, int height);
