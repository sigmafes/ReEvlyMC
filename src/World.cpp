#include "World.h"

#include <cmath>

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
    return true;
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

void World::generate() {
    for (int cx = -m_renderDistance; cx <= m_renderDistance; ++cx) {
        for (int cz = -m_renderDistance; cz <= m_renderDistance; ++cz) {
            auto chunk = std::make_unique<Chunk>(cx, cz);
            generateChunk(*chunk);
            m_chunks.emplace(key(cx, cz), std::move(chunk));
        }
    }
}

void World::buildMeshes() {
    for (auto& [chunkKey, chunk] : m_chunks) {
        (void)chunkKey;
        if (chunk->isDirty()) {
            chunk->buildMesh(*this);
        }
    }
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
