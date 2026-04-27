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

void OrbitState::compute(double c_re, double c_im, int maxIter) {
    cx = c_re; cy = c_im;
    computedMaxIter = maxIter;
    orbit.clear();
    escapeAt = -1;

    double zx = 0.0, zy = 0.0;
    orbit.emplace_back(zx, zy);

    for (int i = 0; i < maxIter; i++) {
        double zx2 = zx * zx - zy * zy + c_re;
        double zy2 = 2.0 * zx * zy + c_im;
        zx = zx2; zy = zy2;
        orbit.emplace_back(zx, zy);
        if (zx * zx + zy * zy > 4.0) {
            escapeAt = (int)orbit.size() - 1;
            break;
        }
    }

    displayIter = 0;
    timer       = 0.0f;
    pauseTimer  = 0.0f;
    playing     = true;
    active      = true;
}

void OrbitState::clear() {
    active      = false;
    escapeAt    = -1;
    displayIter = 0;
    orbit.clear();
}

bool OrbitState::needsAnimation() const {
    if (!active || !playing) return false;
    if (pauseTimer > 0.0f) return true;
    return displayIter < (int)orbit.size() - 1;
}

void MandelbrotState::zoomAtPixel(double px, double py, float factor) {
    double cx, cy;
    pixelToComplex(px, py, cx, cy);

    zoom *= factor;

    double aspect = static_cast<double>(windowWidth) / static_cast<double>(windowHeight);
    centerX = cx - (px / windowWidth - 0.5) * 2.0 * zoom * aspect;
    centerY = cy - (py / windowHeight - 0.5) * 2.0 * zoom;
}
