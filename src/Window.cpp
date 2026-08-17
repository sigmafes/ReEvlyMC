#include "Window.h"

#include <stdexcept>

Window::Window(int width, int height, const std::string& title)
    : m_width(width), m_height(height) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    m_handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_handle) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_handle);
    glfwSetWindowUserPointer(m_handle, this);
    glfwSetFramebufferSizeCallback(m_handle, framebufferSizeCallback);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(m_handle);
        glfwTerminate();
        throw std::runtime_error("Failed to load OpenGL function pointers");
    }

    glfwGetFramebufferSize(m_handle, &m_width, &m_height);
    glViewport(0, 0, m_width, m_height);
}

Window::~Window() {
    if (m_handle) {
        glfwDestroyWindow(m_handle);
    }
    glfwTerminate();
}

bool Window::isOpen() const {
    return !glfwWindowShouldClose(m_handle);
}

void Window::close() {
    glfwSetWindowShouldClose(m_handle, GLFW_TRUE);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_handle);
}

float Window::aspectRatio() const {
    return m_height == 0 ? 1.0f : static_cast<float>(m_width) / static_cast<float>(m_height);
}

void Window::setCursorCaptured(bool captured) {
    m_cursorCaptured = captured;
    glfwSetInputMode(m_handle, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self) {
        return;
    }
    self->m_width = width;
    self->m_height = height;
    glViewport(0, 0, width, height);
}
