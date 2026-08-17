#include "Tile.h"

#include <array>

namespace {

const std::array<TileProperties, static_cast<std::size_t>(TileID::Count)> kTiles = {{
    // Air
    {false, true, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
    // Grass
    {true, false, {0.35f, 0.68f, 0.28f}, {0.45f, 0.36f, 0.24f}, {0.42f, 0.32f, 0.21f}},
    // Dirt
    {true, false, {0.42f, 0.32f, 0.21f}, {0.42f, 0.32f, 0.21f}, {0.42f, 0.32f, 0.21f}},
    // Stone
    {true, false, {0.55f, 0.55f, 0.57f}, {0.50f, 0.50f, 0.52f}, {0.45f, 0.45f, 0.47f}},
    // Sand
    {true, false, {0.85f, 0.80f, 0.55f}, {0.82f, 0.77f, 0.52f}, {0.80f, 0.75f, 0.50f}},
    // Water
    {false, true, {0.20f, 0.40f, 0.85f}, {0.18f, 0.36f, 0.80f}, {0.16f, 0.32f, 0.75f}},
    // Wood
    {true, false, {0.48f, 0.36f, 0.20f}, {0.36f, 0.26f, 0.14f}, {0.48f, 0.36f, 0.20f}},
    // Leaves
    {true, true, {0.22f, 0.52f, 0.20f}, {0.20f, 0.48f, 0.18f}, {0.18f, 0.44f, 0.16f}},
}};

}  // namespace

namespace Tile {

const TileProperties& properties(TileID id) {
    const auto index = static_cast<std::size_t>(id);
    return kTiles[index < kTiles.size() ? index : 0];
}

bool isSolid(TileID id) {
    return properties(id).solid;
}

bool isTransparent(TileID id) {
    return properties(id).transparent;
}

bool isAir(TileID id) {
    return id == TileID::Air;
}

glm::vec3 faceColor(TileID id, Face face) {
    const TileProperties& props = properties(id);
    switch (face) {
        case Face::Top:
            return props.topColor;
        case Face::Bottom:
            return props.bottomColor;
        default:
            return props.sideColor;
    }
}

}  // namespace Tile
