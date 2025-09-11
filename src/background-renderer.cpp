#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../extern/stb/stb_image.h"
#include <stdexcept>
#include <iostream>

static GLuint backgroundTexture{};
static GLuint backgroundVAO{}, backgroundVBO{}, backgroundEBO{};
static GLuint backgroundShaderProgram{};

static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 texCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    texCoord = aTexCoord;
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
in vec2 texCoord;
out vec4 fragColor;
uniform sampler2D backgroundTexture;
void main() {
    //fragColor = texture(backgroundTexture, texCoord);
    fragColor = texture(backgroundTexture, texCoord) * vec4(0.5, 0.5, 0.5, 1.0);
}
)";

static GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success{};
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        throw std::runtime_error(infoLog);
    }

    return shader;
}

/**
 * Initializes background image, quad geometry, and shaders.
 * Must be called after OpenGL context is active.
 */
void initBackgroundImage(const char* imagePath) {
    // Load image
    int texWidth{}, texHeight{}, channels{};
    unsigned char* data = stbi_load(imagePath, &texWidth, &texHeight, &channels, 0);
    if (!data) throw std::runtime_error("Failed to load background image");

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

    // Upload texture
    glGenTextures(1, &backgroundTexture);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, texWidth, texHeight, 0, format, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    // Quad vertex data
    float vertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
    };
    GLuint indices[] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &backgroundVAO);
    glGenBuffers(1, &backgroundVBO);
    glGenBuffers(1, &backgroundEBO);

    glBindVertexArray(backgroundVAO);

    glBindBuffer(GL_ARRAY_BUFFER, backgroundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, backgroundEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    // Compile shaders
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    backgroundShaderProgram = glCreateProgram();
    glAttachShader(backgroundShaderProgram, vs);
    glAttachShader(backgroundShaderProgram, fs);
    glLinkProgram(backgroundShaderProgram);
    GLint linked{};
    glGetProgramiv(backgroundShaderProgram, GL_LINK_STATUS, &linked);
    if (!linked) {
        char infoLog[512];
        glGetProgramInfoLog(backgroundShaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader link error: " << infoLog << "\n";
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

/**
 * Draws the fullscreen background image. Call once per frame before ImGui::NewFrame().
 */
void drawBackgroundImage() {
    glUseProgram(backgroundShaderProgram);
    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);
    glUniform1i(glGetUniformLocation(backgroundShaderProgram, "backgroundTexture"), 0);

    glBindVertexArray(backgroundVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}
