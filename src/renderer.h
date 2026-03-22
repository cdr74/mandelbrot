#pragma once
#include <glad/glad.h>
#include <string>
#include "mandelbrot.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool initialize(int width, int height);
    void shutdown();
    void render(const MandelbrotState& state);

private:
    GLuint shaderProgram = 0;
    GLuint VAO = 0, VBO = 0;

    GLuint loadShaders(const std::string& vertexPath, const std::string& fragmentPath);
};
