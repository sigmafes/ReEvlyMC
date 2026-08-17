#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include "Camera.h"
#include "Chunk.h"
#include "Window.h"
#include "World.h"

namespace {

constexpr const char* kVertexShader = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vColor;
out float vDistance;

void main() {
    vec4 viewPos = uView * vec4(aPos, 1.0);
    vDistance = length(viewPos.xyz);
    vColor = aColor;
    gl_Position = uProjection * viewPos;
}
)";

constexpr const char* kFragmentShader = R"(#version 330 core
in vec3 vColor;
in float vDistance;

uniform vec3 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

out vec4 FragColor;

void main() {
    float fog = clamp((vDistance - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);
    FragColor = vec4(mix(vColor, uFogColor, fog), 1.0);
}
)";

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compilation failed: ") + log);
    }
    return shader;
}

GLuint createShaderProgram() {
    GLuint vertex = compileShader(GL_VERTEX_SHADER, kVertexShader);
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        glDeleteProgram(program);
        throw std::runtime_error(std::string("Shader linking failed: ") + log);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

struct MouseState {
    Camera* camera = nullptr;
    double lastX = 0.0;
    double lastY = 0.0;
    bool firstMove = true;
};

MouseState g_mouse;

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    (void)window;
    if (!g_mouse.camera) {
        return;
    }
    if (g_mouse.firstMove) {
        g_mouse.lastX = xpos;
        g_mouse.lastY = ypos;
        g_mouse.firstMove = false;
    }

    const float xOffset = static_cast<float>(xpos - g_mouse.lastX);
    const float yOffset = static_cast<float>(g_mouse.lastY - ypos);
    g_mouse.lastX = xpos;
    g_mouse.lastY = ypos;
    g_mouse.camera->processMouse(xOffset, yOffset);
}

int envInt(const char* name, int fallback) {
    const char* value = std::getenv(name);
    if (!value) {
        return fallback;
    }
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        return fallback;
    }
}

}  // namespace

int main() {
    try {
        const int seed = envInt("EVLYMC_SEED", 1337);
        const int renderDistance = envInt("EVLYMC_RENDER_DISTANCE", 6);
        const int exitAfterFrames = envInt("EVLYMC_EXIT_AFTER_FRAMES", 0);

        Window window(1280, 720, "EvlyMC");
        window.setCursorCaptured(true);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        const GLuint program = createShaderProgram();
        const GLint viewLocation = glGetUniformLocation(program, "uView");
        const GLint projectionLocation = glGetUniformLocation(program, "uProjection");
        const GLint fogColorLocation = glGetUniformLocation(program, "uFogColor");
        const GLint fogStartLocation = glGetUniformLocation(program, "uFogStart");
        const GLint fogEndLocation = glGetUniformLocation(program, "uFogEnd");

        World world(seed, renderDistance);
        world.generate();
        world.buildMeshes();
        std::cout << "Generated " << world.chunkCount() << " chunks (seed " << seed << ")\n";

        Camera camera(glm::vec3(8.0f, world.surfaceHeight(8, 8) + 12.0f, 8.0f));
        g_mouse.camera = &camera;
        glfwSetCursorPosCallback(window.handle(), cursorPosCallback);

        const glm::vec3 skyColor(0.53f, 0.74f, 0.94f);
        const float viewDistance = static_cast<float>(renderDistance * Chunk::kWidth);

        double lastTime = glfwGetTime();
        int frames = 0;

        while (window.isOpen()) {
            const double now = glfwGetTime();
            const float deltaTime = static_cast<float>(now - lastTime);
            lastTime = now;

            window.pollEvents();
            if (glfwGetKey(window.handle(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                window.close();
            }
            camera.processKeyboard(window.handle(), deltaTime);

            glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(program);
            const glm::mat4 view = camera.viewMatrix();
            const glm::mat4 projection = camera.projectionMatrix(window.aspectRatio());
            glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
            glUniform3fv(fogColorLocation, 1, glm::value_ptr(skyColor));
            glUniform1f(fogStartLocation, viewDistance * 0.55f);
            glUniform1f(fogEndLocation, viewDistance);

            world.draw(camera.position(), viewDistance);

            window.swapBuffers();

            if (exitAfterFrames > 0 && ++frames >= exitAfterFrames) {
                window.close();
            }
        }

        glDeleteProgram(program);
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
