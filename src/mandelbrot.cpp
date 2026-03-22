#include "mandelbrot.h"

void MandelbrotState::reset() {
    centerX = -0.5;
    centerY = 0.0;
    zoom = 1.5f;
    maxIter = 256;
    colorScheme = ColorScheme::Classic;
    dragging = false;
}

void MandelbrotState::pixelToComplex(double px, double py, double& cx, double& cy) const {
    double aspect = static_cast<double>(windowWidth) / static_cast<double>(windowHeight);
    cx = centerX + (px / windowWidth - 0.5) * 2.0 * zoom * aspect;
    cy = centerY + (py / windowHeight - 0.5) * 2.0 * zoom;
}

void MandelbrotState::zoomAtPixel(double px, double py, float factor) {
    double cx, cy;
    pixelToComplex(px, py, cx, cy);

    zoom *= factor;

    double aspect = static_cast<double>(windowWidth) / static_cast<double>(windowHeight);
    centerX = cx - (px / windowWidth - 0.5) * 2.0 * zoom * aspect;
    centerY = cy - (py / windowHeight - 0.5) * 2.0 * zoom;
}
