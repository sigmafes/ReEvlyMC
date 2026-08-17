#pragma once

#include <FastNoiseLite.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "Chunk.h"
#include "Tile.h"

class World {
public:
    explicit World(int seed = 1337, int renderDistance = 6);

    void generate();
    void buildMeshes();
    void draw(const glm::vec3& cameraPosition, float maxDistance) const;

    TileID tileAt(int worldX, int worldY, int worldZ) const;
    bool setTile(int worldX, int worldY, int worldZ, TileID id);
    int surfaceHeight(int worldX, int worldZ) const;

    Chunk* chunkAt(int chunkX, int chunkZ);
    const Chunk* chunkAt(int chunkX, int chunkZ) const;
    int renderDistance() const { return m_renderDistance; }
    std::size_t chunkCount() const { return m_chunks.size(); }

private:
    static std::uint64_t key(int chunkX, int chunkZ);
    void generateChunk(Chunk& chunk);

    int m_seed;
    int m_renderDistance;
    FastNoiseLite m_noise;
    std::unordered_map<std::uint64_t, std::unique_ptr<Chunk>> m_chunks;
};
