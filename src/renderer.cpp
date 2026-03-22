#include "renderer.h"
#include <fstream>
#include <sstream>
#include <iostream>

Renderer::Renderer() {}

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize(int width, int height) {
    shaderProgram = loadShaders("shaders/mandelbrot.vert", "shaders/mandelbrot.frag");
    if (shaderProgram == 0) {
        return false;
    }

    // Fullscreen quad: two triangles covering NDC [-1,1]
    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f,
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void Renderer::shutdown() {
    if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    if (VBO) { glDeleteBuffers(1, &VBO); VBO = 0; }
    if (shaderProgram) { glDeleteProgram(shaderProgram); shaderProgram = 0; }
}

void Renderer::render(const MandelbrotState& state) {
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);

    glUniform2f(glGetUniformLocation(shaderProgram, "u_center"),
                (float)state.centerX, (float)state.centerY);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_zoom"), state.zoom);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_maxIter"), state.maxIter);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_colorScheme"), (int)state.colorScheme);
    glUniform2f(glGetUniformLocation(shaderProgram, "u_resolution"),
                (float)state.windowWidth, (float)state.windowHeight);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glUseProgram(0);
}

GLuint Renderer::loadShaders(const std::string& vertexPath, const std::string& fragmentPath) {
    std::ifstream vFile(vertexPath), fFile(fragmentPath);

    if (!vFile.is_open() || !fFile.is_open()) {
        std::cerr << "Failed to open shader files: " << vertexPath << ", " << fragmentPath << std::endl;
        return 0;
    }

    std::stringstream vStream, fStream;
    vStream << vFile.rdbuf();
    fStream << fFile.rdbuf();

    std::string vertCode = vStream.str();
    std::string fragCode = fStream.str();
    const char* vSrc = vertCode.c_str();
    const char* fSrc = fragCode.c_str();

    int success;
    char infoLog[512];

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vSrc, nullptr);
    glCompileShader(vert);
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vert, 512, nullptr, infoLog);
        std::cerr << "Vertex shader error:\n" << infoLog << std::endl;
        return 0;
    }

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fSrc, nullptr);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(frag, 512, nullptr, infoLog);
        std::cerr << "Fragment shader error:\n" << infoLog << std::endl;
        glDeleteShader(vert);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader link error:\n" << infoLog << std::endl;
        glDeleteShader(vert);
        glDeleteShader(frag);
        return 0;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    return program;
}
