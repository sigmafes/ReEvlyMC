#pragma once

#include "Camera.h"
#include "Tile.h"
#include "World.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    bool intersects(const AABB& other) const;
    bool contains(const glm::vec3& point) const;
};

class Player {
public:
    struct RaycastHit {
        bool hit = false;
        glm::ivec3 block{0};
        glm::ivec3 normal{0};
        float distance = 0.0f;
    };

    explicit Player(const glm::vec3& spawnPosition = glm::vec3(8.0f, 80.0f, 8.0f));

    void processMouse(float xOffset, float yOffset);
    void update(GLFWwindow* window, float deltaTime, World& world);

    bool breakBlock(World& world) const;
    bool placeBlock(World& world, TileID id = TileID::Stone) const;

    const Camera& camera() const { return m_camera; }
    Camera& camera() { return m_camera; }

    glm::vec3 position() const { return m_position; }
    glm::vec3 eyePosition() const;
    AABB bounds() const;
    bool isGrounded() const { return m_isGrounded; }
    float width() const { return m_width; }
    float height() const { return m_height; }

    RaycastHit raycast(const World& world, float maxDistance = 5.0f) const;

private:
    void processKeyboard(GLFWwindow* window, float deltaTime);
    void resolveAxis(World& world, int axis, float deltaTime);
    void updateGrounded(const World& world);

    static bool collidesWithWorld(const World& world, const AABB& box);

    glm::vec3 m_position;
    glm::vec3 m_velocity{0.0f};
    Camera m_camera;

    float m_width = 0.6f;
    float m_height = 1.8f;
    float m_eyeHeight = 1.62f;

    bool m_isGrounded = false;
    bool m_jumpRequested = false;

    // Player input state to detect a single Space press.
    bool m_spacePressed = false;

    static constexpr float kGravity = -28.0f;
    static constexpr float kJumpHeight = 1.25f;
    static constexpr float kWalkSpeed = 4.3f;
    static constexpr float kAirSpeed = 0.3f;
    static constexpr float kGroundAcceleration = 14.0f;
    static constexpr float kAirAcceleration = 2.0f;
    static constexpr float kGroundFriction = 10.0f;
    static constexpr float kAirFriction = 0.5f;
    static constexpr float kEpsilon = 1e-4f;
};
