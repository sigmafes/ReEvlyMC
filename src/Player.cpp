#include "Player.h"

#include "Tile.h"

#include <algorithm>
#include <cmath>
#include <limits>

bool AABB::intersects(const AABB& other) const {
    return min.x < other.max.x && max.x > other.min.x &&
           min.y < other.max.y && max.y > other.min.y &&
           min.z < other.max.z && max.z > other.min.z;
}

bool AABB::contains(const glm::vec3& point) const {
    return point.x >= min.x && point.x < max.x &&
           point.y >= min.y && point.y < max.y &&
           point.z >= min.z && point.z < max.z;
}

Player::Player(const glm::vec3& spawnPosition)
    : m_position(spawnPosition),
      m_camera(eyePosition()) {}

glm::vec3 Player::eyePosition() const {
    return m_position + glm::vec3(0.0f, m_eyeHeight, 0.0f);
}

AABB Player::bounds() const {
    const glm::vec3 halfSize(m_width * 0.5f, 0.0f, m_width * 0.5f);
    AABB box;
    box.min = m_position - halfSize;
    box.max = box.min + glm::vec3(m_width, m_height, m_width);
    return box;
}

void Player::processMouse(float xOffset, float yOffset) {
    m_camera.processMouse(xOffset, yOffset);
}

void Player::processKeyboard(GLFWwindow* window, float deltaTime) {
    (void)deltaTime;

    bool space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (space && !m_spacePressed && m_isGrounded) {
        m_jumpRequested = true;
    }
    m_spacePressed = space;
}

void Player::update(GLFWwindow* window, float deltaTime, World& world) {
    processKeyboard(window, deltaTime);

    // --- Horizontal movement ---
    const float maxSpeed = m_isGrounded ? kWalkSpeed : kAirSpeed;
    const float acceleration = m_isGrounded ? kGroundAcceleration : kAirAcceleration;
    const float friction = m_isGrounded ? kGroundFriction : kAirFriction;

    glm::vec3 front = m_camera.front();
    front.y = 0.0f;
    if (glm::length(front) > 0.001f) {
        front = glm::normalize(front);
    }
    const glm::vec3 right = m_camera.right();

    glm::vec3 wishDir{0.0f};
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        wishDir += front;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        wishDir -= front;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        wishDir -= right;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        wishDir += right;
    }

    if (glm::length(wishDir) > 0.0f) {
        wishDir = glm::normalize(wishDir);
        m_velocity.x += wishDir.x * acceleration * deltaTime;
        m_velocity.z += wishDir.z * acceleration * deltaTime;

        const float horizontalSpeed = std::sqrt(m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z);
        if (horizontalSpeed > maxSpeed) {
            const float scale = maxSpeed / horizontalSpeed;
            m_velocity.x *= scale;
            m_velocity.z *= scale;
        }
    }

    // Friction (horizontal only).
    const float horizontalSpeed = std::sqrt(m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z);
    if (horizontalSpeed > 0.0f) {
        float drop = horizontalSpeed * friction * deltaTime;
        if (drop > horizontalSpeed) {
            drop = horizontalSpeed;
        }
        const float scale = (horizontalSpeed - drop) / horizontalSpeed;
        m_velocity.x *= scale;
        m_velocity.z *= scale;
    }

    // --- Vertical physics ---
    m_velocity.y += kGravity * deltaTime;

    if (m_jumpRequested && m_isGrounded) {
        m_velocity.y = std::sqrt(2.0f * -kGravity * kJumpHeight);
        m_jumpRequested = false;
        m_isGrounded = false;
    } else {
        m_jumpRequested = false;
    }

    // --- Collision resolution per axis ---
    resolveAxis(world, 0, deltaTime);  // X
    resolveAxis(world, 1, deltaTime);  // Y
    resolveAxis(world, 2, deltaTime);  // Z

    updateGrounded(world);
    m_camera.setPosition(eyePosition());
}

bool Player::collidesWithWorld(const World& world, const AABB& box) {
    const int minX = static_cast<int>(std::floor(box.min.x));
    const int maxX = static_cast<int>(std::floor(box.max.x));
    const int minY = static_cast<int>(std::floor(box.min.y));
    const int maxY = static_cast<int>(std::floor(box.max.y));
    const int minZ = static_cast<int>(std::floor(box.min.z));
    const int maxZ = static_cast<int>(std::floor(box.max.z));

    for (int y = minY; y <= maxY; ++y) {
        for (int z = minZ; z <= maxZ; ++z) {
            for (int x = minX; x <= maxX; ++x) {
                if (Tile::isSolid(world.tileAt(x, y, z))) {
                    return true;
                }
            }
        }
    }
    return false;
}

void Player::resolveAxis(World& world, int axis, float deltaTime) {
    const float move = m_velocity[axis] * deltaTime;

    // Always let gravity affect Y; skip near-zero horizontal motion.
    if (std::abs(move) < 1e-6f) {
        return;
    }

    m_position[axis] += move;
    AABB box = bounds();

    if (!collidesWithWorld(world, box)) {
        return;
    }

    if (axis == 0) {
        if (move > 0.0f) {
            const float boundary = std::floor(box.max.x);
            m_position.x = boundary - m_width * 0.5f - kEpsilon;
        } else {
            const float boundary = std::floor(box.min.x);
            m_position.x = boundary + 1.0f + m_width * 0.5f + kEpsilon;
        }
    } else if (axis == 1) {
        if (move > 0.0f) {
            const float boundary = std::floor(box.max.y);
            m_position.y = boundary - m_height - kEpsilon;
        } else {
            const float boundary = std::floor(box.min.y);
            m_position.y = boundary + 1.0f + kEpsilon;
            m_isGrounded = true;
        }
    } else {  // axis == 2
        if (move > 0.0f) {
            const float boundary = std::floor(box.max.z);
            m_position.z = boundary - m_width * 0.5f - kEpsilon;
        } else {
            const float boundary = std::floor(box.min.z);
            m_position.z = boundary + 1.0f + m_width * 0.5f + kEpsilon;
        }
    }

    m_velocity[axis] = 0.0f;
}

void Player::updateGrounded(const World& world) {
    const AABB box = bounds();
    const int minX = static_cast<int>(std::floor(box.min.x));
    const int maxX = static_cast<int>(std::floor(box.max.x));
    const int minZ = static_cast<int>(std::floor(box.min.z));
    const int maxZ = static_cast<int>(std::floor(box.max.z));
    const int blockBelowY = static_cast<int>(std::floor(m_position.y - 0.01f));

    m_isGrounded = false;
    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            if (Tile::isSolid(world.tileAt(x, blockBelowY, z))) {
                m_isGrounded = true;
                return;
            }
        }
    }
}

Player::RaycastHit Player::raycast(const World& world, float maxDistance) const {
    RaycastHit result;

    const glm::vec3 origin = eyePosition();
    const glm::vec3 dir = glm::normalize(m_camera.front());

    glm::ivec3 step;
    step.x = (dir.x > 0.0f) ? 1 : (dir.x < 0.0f ? -1 : 0);
    step.y = (dir.y > 0.0f) ? 1 : (dir.y < 0.0f ? -1 : 0);
    step.z = (dir.z > 0.0f) ? 1 : (dir.z < 0.0f ? -1 : 0);

    const auto tDelta = glm::vec3(
        dir.x != 0.0f ? std::abs(1.0f / dir.x) : std::numeric_limits<float>::max(),
        dir.y != 0.0f ? std::abs(1.0f / dir.y) : std::numeric_limits<float>::max(),
        dir.z != 0.0f ? std::abs(1.0f / dir.z) : std::numeric_limits<float>::max()
    );

    const auto nextBoundary = [](float coord, int s) {
        return (s > 0) ? std::floor(coord) + 1.0f : std::floor(coord);
    };

    glm::vec3 tMax;
    tMax.x = (dir.x != 0.0f) ? (nextBoundary(origin.x, step.x) - origin.x) / dir.x
                              : std::numeric_limits<float>::max();
    tMax.y = (dir.y != 0.0f) ? (nextBoundary(origin.y, step.y) - origin.y) / dir.y
                              : std::numeric_limits<float>::max();
    tMax.z = (dir.z != 0.0f) ? (nextBoundary(origin.z, step.z) - origin.z) / dir.z
                              : std::numeric_limits<float>::max();

    if (tMax.x < 0.0f) tMax.x += tDelta.x;
    if (tMax.y < 0.0f) tMax.y += tDelta.y;
    if (tMax.z < 0.0f) tMax.z += tDelta.z;

    glm::ivec3 voxel = glm::floor(origin);

    const int maxSteps = static_cast<int>(maxDistance * 3.0f) + 3;
    for (int i = 0; i < maxSteps; ++i) {
        int axis = 0;
        if (tMax.y < tMax.x) axis = 1;
        if (tMax.z < tMax[axis]) axis = 2;

        if (tMax[axis] > maxDistance) {
            break;
        }

        const float distance = tMax[axis];
        voxel[axis] += step[axis];
        tMax[axis] += tDelta[axis];

        const TileID tile = world.tileAt(voxel.x, voxel.y, voxel.z);
        if (tile != TileID::Air && Tile::isSolid(tile)) {
            result.hit = true;
            result.block = voxel;
            result.normal = glm::ivec3(0);
            result.normal[axis] = -step[axis];
            result.distance = distance;
            return result;
        }
    }

    return result;
}

bool Player::breakBlock(World& world) const {
    const RaycastHit hit = raycast(world);
    if (!hit.hit) {
        return false;
    }
    return world.setTile(hit.block.x, hit.block.y, hit.block.z, TileID::Air);
}

bool Player::placeBlock(World& world, TileID id) const {
    const RaycastHit hit = raycast(world);
    if (!hit.hit) {
        return false;
    }

    const glm::ivec3 placePos = hit.block + hit.normal;
    if (placePos.y < 0 || placePos.y >= Chunk::kHeight) {
        return false;
    }

    AABB newBlockBox;
    newBlockBox.min = glm::vec3(placePos);
    newBlockBox.max = newBlockBox.min + glm::vec3(1.0f);

    if (newBlockBox.intersects(bounds())) {
        return false;
    }

    return world.setTile(placePos.x, placePos.y, placePos.z, id);
}
