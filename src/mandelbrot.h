#pragma once

enum class ColorScheme {
    Classic = 0,
    Fire,
    Ocean,
    Grayscale,
    Psychedelic,
    Ultra,
    COUNT
};

struct MandelbrotState {
    double centerX = -0.5;
    double centerY = 0.0;
    float zoom = 1.5f;       // fractal units per half-height
    int maxIter = 256;
    ColorScheme colorScheme = ColorScheme::Classic;
    int windowWidth = 1024;
    int windowHeight = 768;

    // Drag state
    bool dragging = false;
    double dragStartPx = 0.0, dragStartPy = 0.0;
    double dragStartCenterX = 0.0, dragStartCenterY = 0.0;

    void reset();
    void pixelToComplex(double px, double py, double& cx, double& cy) const;
    void zoomAtPixel(double px, double py, float factor);
};
