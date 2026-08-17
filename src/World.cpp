#include "World.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <tuple>

namespace {

constexpr int kSeaLevel = 62;
constexpr int kBaseHeight = 64;
constexpr float kHeightAmplitude = 28.0f;

int floorDiv(int value, int divisor) {
    int quotient = value / divisor;
    if ((value % divisor != 0) && ((value < 0) != (divisor < 0))) {
        --quotient;
    }
    return quotient;
}

int floorMod(int value, int divisor) {
    return value - floorDiv(value, divisor) * divisor;
}

FastNoiseLite makeTerrainNoise(int seed) {
    FastNoiseLite noise(seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(4);
    noise.SetFrequency(0.008f);
    return noise;
}

bool lightCanPass(TileID id) {
    return !Tile::isSolid(id) || Tile::isTransparent(id);
}

using LightQueue = std::queue<std::tuple<int, int, int>>;

}  // namespace

World::World(int seed, int renderDistance)
    : m_seed(seed), m_renderDistance(renderDistance), m_noise(makeTerrainNoise(seed)) {}

std::uint64_t World::key(int chunkX, int chunkZ) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkX)) << 32) |
           static_cast<std::uint32_t>(chunkZ);
}

Chunk* World::chunkAt(int chunkX, int chunkZ) {
    auto it = m_chunks.find(key(chunkX, chunkZ));
    return it == m_chunks.end() ? nullptr : it->second.get();
}

const Chunk* World::chunkAt(int chunkX, int chunkZ) const {
    auto it = m_chunks.find(key(chunkX, chunkZ));
    return it == m_chunks.end() ? nullptr : it->second.get();
}

bool World::setTile(int worldX, int worldY, int worldZ, TileID id) {
    if (worldY < 0 || worldY >= Chunk::kHeight) {
        return false;
    }

    Chunk* chunk = chunkAt(floorDiv(worldX, Chunk::kWidth), floorDiv(worldZ, Chunk::kDepth));
    if (!chunk) {
        return false;
    }

    chunk->setTile(floorMod(worldX, Chunk::kWidth), worldY, floorMod(worldZ, Chunk::kDepth), id);

    // Recompute the lighting for the chunk so shadows update after edits.
    propagateSunlight(*chunk);
    propagateBlocklight(*chunk);
    return true;
}

std::uint8_t World::lightAt(int worldX, int worldY, int worldZ) const {
    if (worldY < 0 || worldY >= Chunk::kHeight) {
        return 0;
    }
    const Chunk* chunk = chunkAt(floorDiv(worldX, Chunk::kWidth), floorDiv(worldZ, Chunk::kDepth));
    if (!chunk) {
        return 0;
    }
    return chunk->lightAt(floorMod(worldX, Chunk::kWidth), worldY, floorMod(worldZ, Chunk::kDepth));
}

float World::sampleLight(int worldX, int worldY, int worldZ) const {
    if (worldY >= Chunk::kHeight) {
        return 1.0f;
    }
    if (worldY < 0) {
        return 0.0f;
    }
    const std::uint8_t packed = lightAt(worldX, worldY, worldZ);
    const int sunlight = (packed >> 4) & 0x0F;
    const int blocklight = packed & 0x0F;
    return std::max(sunlight, blocklight) / 15.0f;
}

int World::surfaceHeight(int worldX, int worldZ) const {
    const float value = m_noise.GetNoise(static_cast<float>(worldX), static_cast<float>(worldZ));
    return kBaseHeight + static_cast<int>(std::lround(value * kHeightAmplitude));
}

TileID World::tileAt(int worldX, int worldY, int worldZ) const {
    if (worldY < 0 || worldY >= Chunk::kHeight) {
        return TileID::Air;
    }

    const Chunk* chunk = chunkAt(floorDiv(worldX, Chunk::kWidth), floorDiv(worldZ, Chunk::kDepth));
    if (!chunk) {
        return TileID::Air;
    }
    return chunk->tileAt(floorMod(worldX, Chunk::kWidth), worldY, floorMod(worldZ, Chunk::kDepth));
}

void World::generateChunk(Chunk& chunk) {
    for (int x = 0; x < Chunk::kWidth; ++x) {
        for (int z = 0; z < Chunk::kDepth; ++z) {
            const int worldX = chunk.chunkX() * Chunk::kWidth + x;
            const int worldZ = chunk.chunkZ() * Chunk::kDepth + z;
            const int height = surfaceHeight(worldX, worldZ);

            for (int y = 0; y <= height; ++y) {
                TileID id = TileID::Stone;
                if (y == height) {
                    id = height <= kSeaLevel + 1 ? TileID::Sand : TileID::Grass;
                } else if (y > height - 4) {
                    id = TileID::Dirt;
                }
                chunk.setTile(x, y, z, id);
            }

            for (int y = height + 1; y <= kSeaLevel; ++y) {
                chunk.setTile(x, y, z, TileID::Water);
            }
        }
    }
}

void World::propagateSunlight(Chunk& chunk) {
    // Top-down column skylight. Air blocks keep the current sky light value;
    // non-air transparent blocks (water, leaves) absorb one level per block.
    for (int x = 0; x < Chunk::kWidth; ++x) {
        for (int z = 0; z < Chunk::kDepth; ++z) {
            int light = 15;
            for (int y = Chunk::kHeight - 1; y >= 0; --y) {
                const TileID id = chunk.tileAt(x, y, z);
                if (!lightCanPass(id)) {
                    light = 0;
                    continue;
                }
                chunk.setSunlightAt(x, y, z, light);
                if (id != TileID::Air) {
                    // Transparent solids filter one light level.
                    light = std::max(0, light - 1);
                }
            }
        }
    }
}

void World::propagateBlocklight(Chunk& chunk) {
    LightQueue queue;

    for (int x = 0; x < Chunk::kWidth; ++x) {
        for (int z = 0; z < Chunk::kDepth; ++z) {
            for (int y = 0; y < Chunk::kHeight; ++y) {
                const TileID id = chunk.tileAt(x, y, z);
                const std::uint8_t emissive = Tile::lightLevel(id);
                if (emissive > 0) {
                    chunk.setBlocklightAt(x, y, z, emissive);
                    queue.emplace(chunk.chunkX() * Chunk::kWidth + x, y,
                                  chunk.chunkZ() * Chunk::kDepth + z);
                }
            }
        }
    }

    const std::array<glm::ivec3, 6> kNeighbors = {{
        {0, 1, 0},
        {0, -1, 0},
        {0, 0, 1},
        {0, 0, -1},
        {1, 0, 0},
        {-1, 0, 0},
    }};

    while (!queue.empty()) {
        const auto [wx, wy, wz] = queue.front();
        queue.pop();

        const int current = lightAt(wx, wy, wz) & 0x0F;
        if (current <= 1) {
            continue;
        }

        for (const auto& n : kNeighbors) {
            const int nx = wx + n.x;
            const int ny = wy + n.y;
            const int nz = wz + n.z;
            const TileID neighbor = tileAt(nx, ny, nz);
            if (!lightCanPass(neighbor)) {
                continue;
            }

            const int existing = lightAt(nx, ny, nz) & 0x0F;
            const int propagated = current - 1;
            if (propagated > existing) {
                Chunk* neighborChunk = chunkAt(floorDiv(nx, Chunk::kWidth),
                                               floorDiv(nz, Chunk::kDepth));
                if (neighborChunk) {
                    neighborChunk->setBlocklightAt(floorMod(nx, Chunk::kWidth), ny,
                                                   floorMod(nz, Chunk::kDepth), propagated);
                    queue.emplace(nx, ny, nz);
                }
            }
        }
    }
}

void World::generate() {
    for (int cx = -m_renderDistance; cx <= m_renderDistance; ++cx) {
        for (int cz = -m_renderDistance; cz <= m_renderDistance; ++cz) {
            auto chunk = std::make_unique<Chunk>(cx, cz);
            generateChunk(*chunk);
            m_chunks.emplace(key(cx, cz), std::move(chunk));
        }
    }

    for (auto& [chunkKey, chunk] : m_chunks) {
        (void)chunkKey;
        propagateSunlight(*chunk);
        propagateBlocklight(*chunk);
    }
}

void World::rebuildDirtyMeshes() {
    for (auto& [chunkKey, chunk] : m_chunks) {
        (void)chunkKey;
        if (chunk->isDirty()) {
            chunk->buildMesh(*this);
        }
    }
}

void World::buildMeshes() {
    rebuildDirtyMeshes();
}

void World::draw(const glm::vec3& cameraPosition, float maxDistance) const {
    const float maxDistanceSquared = maxDistance * maxDistance;
    for (const auto& [chunkKey, chunk] : m_chunks) {
        (void)chunkKey;
        const glm::vec3 delta = chunk->center() - glm::vec3(cameraPosition.x, 0.0f, cameraPosition.z);
        if (delta.x * delta.x + delta.z * delta.z <= maxDistanceSquared) {
            chunk->draw();
        }
    }
}
