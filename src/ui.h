#pragma once
#include "mandelbrot.h"

struct UIState {
    bool showOverlay = true;
};

class UI {
public:
    void applyTheme();
    void initialize();
    void render(MandelbrotState& state, UIState& uiState);
};
