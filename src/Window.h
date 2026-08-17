#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool isOpen() const;
    void close();
    void pollEvents();
    void swapBuffers();

    GLFWwindow* handle() const { return m_handle; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    float aspectRatio() const;

    void setCursorCaptured(bool captured);
    bool isCursorCaptured() const { return m_cursorCaptured; }

private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* m_handle = nullptr;
    int m_width = 0;
    int m_height = 0;
    bool m_cursorCaptured = false;
};
