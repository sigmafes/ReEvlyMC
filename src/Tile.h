#pragma once

#include <cstdint>
#include <glm/glm.hpp>

enum class TileID : std::uint8_t {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Sand,
    Water,
    Wood,
    Leaves,
    Count
};

enum class Face : std::uint8_t {
    Top = 0,
    Bottom,
    North,
    South,
    East,
    West
};

struct TileProperties {
    bool solid;
    bool transparent;
    std::uint8_t lightLevel = 0;  // Emitted blocklight level (0-15).
    glm::vec3 topColor;
    glm::vec3 sideColor;
    glm::vec3 bottomColor;
};

namespace Tile {

const TileProperties& properties(TileID id);

bool isSolid(TileID id);
bool isTransparent(TileID id);
bool isAir(TileID id);
std::uint8_t lightLevel(TileID id);

// Flat colors stand in for an atlas lookup until texturing lands.
glm::vec3 faceColor(TileID id, Face face);

}  // namespace Tile
