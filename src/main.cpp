#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "Camera.h"
#include "Chunk.h"
#include "Player.h"
#include "Window.h"
#include "World.h"

namespace {

std::string loadFile(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Failed to open ") + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* sourcePtr = source.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
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
    const std::string vertexSource = loadFile("shaders/vertex.glsl");
    const std::string fragmentSource = loadFile("shaders/fragment.glsl");

    GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

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

struct InputState {
    Player* player = nullptr;
    World* world = nullptr;
    double lastX = 0.0;
    double lastY = 0.0;
    bool firstMove = true;
};

InputState g_input;

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    (void)window;
    if (!g_input.player) {
        return;
    }
    if (g_input.firstMove) {
        g_input.lastX = xpos;
        g_input.lastY = ypos;
        g_input.firstMove = false;
    }

    const float xOffset = static_cast<float>(xpos - g_input.lastX);
    const float yOffset = static_cast<float>(g_input.lastY - ypos);
    g_input.lastX = xpos;
    g_input.lastY = ypos;
    g_input.player->processMouse(xOffset, yOffset);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    (void)window;
    (void)mods;
    if (!g_input.player || !g_input.world || action != GLFW_PRESS) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (g_input.player->breakBlock(*g_input.world)) {
            g_input.world->buildMeshes();
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (g_input.player->placeBlock(*g_input.world)) {
            g_input.world->buildMeshes();
        }
    }
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

        Player player(glm::vec3(8.0f, world.surfaceHeight(8, 8) + 12.0f, 8.0f));
        g_input.player = &player;
        g_input.world = &world;
        glfwSetCursorPosCallback(window.handle(), cursorPosCallback);
        glfwSetMouseButtonCallback(window.handle(), mouseButtonCallback);

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

            player.update(window.handle(), deltaTime, world);
            world.buildMeshes();

            glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(program);
            const Camera& camera = player.camera();
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
