#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <algorithm>
#include <iostream>
#include "mandelbrot.h"
#include "renderer.h"
#include "ui.h"

const int WINDOW_WIDTH  = 1024;
const int WINDOW_HEIGHT = 768;
const char* WINDOW_TITLE = "Mandelbrot Explorer";

// Global state (needed by GLFW callbacks)
MandelbrotState mandelbrot;
UIState uiState;
OrbitState orbitState;
Renderer renderer;
UI ui;
bool needsRedraw = true;

void errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
    mandelbrot.windowWidth  = width;
    mandelbrot.windowHeight = height;
    needsRedraw = true;
}

void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_H) {
        uiState.showOverlay ^= true;
    }
    needsRedraw = true;
}

void scrollCallback(GLFWwindow* window, double /*xoff*/, double yoff) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    float factor = (yoff > 0) ? 0.8f : 1.25f;
    mandelbrot.zoomAtPixel(mx, my, factor);
    needsRedraw = true;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);
            mandelbrot.dragging         = true;
            mandelbrot.dragStartPx      = mx;
            mandelbrot.dragStartPy      = my;
            mandelbrot.dragStartCenterX = mandelbrot.centerX;
            mandelbrot.dragStartCenterY = mandelbrot.centerY;
        } else if (action == GLFW_RELEASE) {
            mandelbrot.dragging = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        double cx, cy;
        mandelbrot.pixelToComplex(mx, my, cx, cy);
        orbitState.compute(cx, cy, mandelbrot.maxIter);
    }
    needsRedraw = true;
}

void cursorPosCallback(GLFWwindow*, double xpos, double ypos) {
    // Only redraw for cursor movement when the cursor is over the overlay
    // (needed for hover highlights). Movement over the fractal is ignored.
    if (xpos >= uiState.overlayX && xpos <= uiState.overlayX + uiState.overlayW &&
        ypos >= uiState.overlayY && ypos <= uiState.overlayY + uiState.overlayH)
        needsRedraw = true;
}

int main() {
    std::cout << "Starting Mandelbrot Explorer..." << std::endl;

    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    mandelbrot.windowWidth  = WINDOW_WIDTH;
    mandelbrot.windowHeight = WINDOW_HEIGHT;

    if (!renderer.initialize(WINDOW_WIDTH, WINDOW_HEIGHT)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        glfwTerminate();
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    ui.applyTheme();
    ui.initialize();

    std::cout << "Entering main loop..." << std::endl;

    double lastTime = glfwGetTime();
    bool imguiWantsMouse = false; // updated each frame after render

    while (!glfwWindowShouldClose(window)) {
        // Block until an event arrives when nothing needs continuous redraws.
        // ImGui needs redraws while the cursor is over its window (hover highlights).
        bool animating = mandelbrot.cyclingColors || mandelbrot.dragging
                      || imguiWantsMouse || orbitState.needsAnimation();
        if (animating)
            glfwPollEvents();
        else
            glfwWaitEvents();

        // Skip render entirely if nothing changed
        if (!needsRedraw && !animating)
            continue;
        needsRedraw = false;

        double now = glfwGetTime();
        float  dt  = (float)(now - lastTime);
        lastTime   = now;

        // Process drag pan
        if (mandelbrot.dragging) {
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);

            double dx = mx - mandelbrot.dragStartPx;
            double dy = my - mandelbrot.dragStartPy;
            double aspect = (double)mandelbrot.windowWidth / (double)mandelbrot.windowHeight;
            double unitsX = 2.0 * mandelbrot.zoom * aspect / mandelbrot.windowWidth;
            double unitsY = 2.0 * mandelbrot.zoom / mandelbrot.windowHeight;

            mandelbrot.centerX = mandelbrot.dragStartCenterX - dx * unitsX;
            mandelbrot.centerY = mandelbrot.dragStartCenterY - dy * unitsY;
        }

        if (mandelbrot.cyclingColors)
            mandelbrot.colorOffset += dt * 15.0f;

        // Advance orbit animation
        if (orbitState.active && orbitState.playing) {
            if (orbitState.pauseTimer > 0.0f) {
                orbitState.pauseTimer = std::max(0.0f, orbitState.pauseTimer - dt);
            } else {
                orbitState.timer += dt;
                float step = 1.0f / orbitState.speed;
                while (orbitState.timer >= step) {
                    orbitState.timer -= step;
                    int last = (int)orbitState.orbit.size() - 1;
                    if (orbitState.displayIter < last) {
                        orbitState.displayIter++;
                    } else if (orbitState.looping) {
                        orbitState.displayIter = 0;
                        orbitState.pauseTimer  = 0.8f;
                        break;
                    } else {
                        orbitState.playing = false;
                        break;
                    }
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ui.render(mandelbrot, uiState, orbitState);

        renderer.render(mandelbrot);

        if (uiState.pendingSave) {
            uiState.pendingSave = false;
            std::string name = saveFramebuffer(mandelbrot.windowWidth, mandelbrot.windowHeight);
            uiState.saveStatus = "Saved: " + name;
            uiState.saveStatusTimer = 3.0f;
        }

        imguiWantsMouse = ImGui::GetIO().WantCaptureMouse;

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
