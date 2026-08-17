#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <array>
#include <vector>

#include "Tile.h"

class World;

class Chunk {
public:
    static constexpr int kWidth = 16;
    static constexpr int kHeight = 256;
    static constexpr int kDepth = 16;
    static constexpr int kVolume = kWidth * kHeight * kDepth;

    Chunk(int chunkX, int chunkZ);
    ~Chunk();

    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    static bool inBounds(int x, int y, int z);
    static int index(int x, int y, int z);

    TileID tileAt(int x, int y, int z) const;
    void setTile(int x, int y, int z, TileID id);

    std::uint8_t lightAt(int x, int y, int z) const;          // packed sunlight/blocklight
    void setLightAt(int x, int y, int z, std::uint8_t packed);
    int sunlightAt(int x, int y, int z) const;
    void setSunlightAt(int x, int y, int z, int level);
    int blocklightAt(int x, int y, int z) const;
    void setBlocklightAt(int x, int y, int z, int level);

    // Rebuilds the CPU vertex buffer and uploads it to the GPU.
    void buildMesh(const World& world);
    void draw() const;

    bool isDirty() const { return m_dirty; }
    void markDirty() { m_dirty = true; }

    int chunkX() const { return m_chunkX; }
    int chunkZ() const { return m_chunkZ; }
    glm::vec3 worldOrigin() const;
    glm::vec3 center() const;

private:
    void appendFace(std::vector<float>& vertices, Face face, int x, int y, int z,
                    TileID id, const float cornerLights[4]) const;
    bool faceVisible(const World& world, int x, int y, int z, TileID self) const;
    void uploadMesh(const std::vector<float>& vertices);

    int m_chunkX;
    int m_chunkZ;
    std::vector<TileID> m_tiles;
    std::vector<std::uint8_t> m_light;  // packed: high nibble = sunlight, low = blocklight

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLsizei m_vertexCount = 0;
    bool m_dirty = true;
};
