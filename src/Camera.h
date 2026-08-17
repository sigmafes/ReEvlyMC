#pragma once

#include <glm/glm.hpp>

struct GLFWwindow;

class Camera {
public:
    explicit Camera(const glm::vec3& position = glm::vec3(0.0f, 80.0f, 0.0f));

    void processKeyboard(GLFWwindow* window, float deltaTime);
    void processMouse(float xOffset, float yOffset);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspectRatio) const;

    const glm::vec3& position() const { return m_position; }
    void setPosition(const glm::vec3& position) { m_position = position; }

private:
    void updateVectors();

    glm::vec3 m_position;
    glm::vec3 m_front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};

    float m_yaw = -90.0f;
    float m_pitch = -20.0f;
    float m_speed = 20.0f;
    float m_sensitivity = 0.1f;
    float m_fov = 70.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 500.0f;
};
