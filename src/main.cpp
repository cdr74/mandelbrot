#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
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
Renderer renderer;
UI ui;

void errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
    mandelbrot.windowWidth  = width;
    mandelbrot.windowHeight = height;
}

void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_H) {
        uiState.showOverlay ^= true;
    }
}

void scrollCallback(GLFWwindow* window, double /*xoff*/, double yoff) {
    // Skip if ImGui wants the mouse
    if (ImGui::GetIO().WantCaptureMouse) return;

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    float factor = (yoff > 0) ? 0.8f : 1.25f;
    mandelbrot.zoomAtPixel(mx, my, factor);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/) {
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);
            mandelbrot.dragging      = true;
            mandelbrot.dragStartPx   = mx;
            mandelbrot.dragStartPy   = my;
            mandelbrot.dragStartCenterX = mandelbrot.centerX;
            mandelbrot.dragStartCenterY = mandelbrot.centerY;
        } else if (action == GLFW_RELEASE) {
            mandelbrot.dragging = false;
        }
    }
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

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

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

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ui.render(mandelbrot, uiState);

        renderer.render(mandelbrot);

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
