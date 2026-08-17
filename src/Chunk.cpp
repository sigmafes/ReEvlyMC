#include "Chunk.h"

#include "World.h"

#include <algorithm>
#include <cmath>

namespace {

struct FaceGeometry {
    glm::ivec3 normal;
    std::array<glm::vec3, 4> corners;
    float shade;  // Directional face shading multiplier.
};

// Corner order: two triangles are emitted as 0-1-2 and 2-3-0.
const std::array<FaceGeometry, 6> kFaces = {{
    // Top (+Y): 100%
    {{0, 1, 0}, {{{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}}, 1.00f},
    // Bottom (-Y): 50%
    {{0, -1, 0}, {{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}}, 0.50f},
    // North (+Z): 60%
    {{0, 0, 1}, {{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}}, 0.60f},
    // South (-Z): 60%
    {{0, 0, -1}, {{{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}}}, 0.60f},
    // East (+X): 80%
    {{1, 0, 0}, {{{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}}}, 0.80f},
    // West (-X): 80%
    {{-1, 0, 0}, {{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}}, 0.80f},
}};

const std::array<float, 4> kAmbientOcclusionTable = {1.0f, 0.84f, 0.68f, 0.52f};

// Computes the incoming light for one corner of a face. AO is calculated from
// the two edge-neighbor blocks and the diagonal block as described by
// Mikola Lysenko's vertex ambient occlusion technique.
float cornerLight(const FaceGeometry& face, int cornerIndex,
                  const glm::ivec3& blockWorldPos, const World& world,
                  float frontLight) {
    const glm::vec3& cornerPos = face.corners[cornerIndex];

    glm::ivec3 side1 = face.normal;
    glm::ivec3 side2 = face.normal;
    glm::ivec3 cornerOff = face.normal;

    int assigned = 0;
    for (int axis = 0; axis < 3; ++axis) {
        if (face.normal[axis] != 0) {
            continue;
        }
        const int direction = (cornerPos[axis] == 0.0f) ? 1 : -1;
        glm::ivec3 unit(0);
        unit[axis] = direction;
        if (assigned == 0) {
            side1 += unit;
            cornerOff += unit;
            assigned = 1;
        } else {
            side2 += unit;
            cornerOff += unit;
        }
    }

    const auto isSolidAt = [&](const glm::ivec3& offset) -> bool {
        const glm::ivec3 p = blockWorldPos + offset;
        return Tile::isSolid(world.tileAt(p.x, p.y, p.z));
    };

    const bool s1 = isSolidAt(side1);
    const bool s2 = isSolidAt(side2);
    const bool cr = isSolidAt(cornerOff);

    int occupied = (s1 ? 1 : 0) + (s2 ? 1 : 0) + (cr ? 1 : 0);
    if (s1 && s2) {
        occupied = 3;
    }
    occupied = std::clamp(occupied, 0, 3);

    const float ao = kAmbientOcclusionTable[static_cast<std::size_t>(occupied)];
    return frontLight * face.shade * ao;
}

}  // namespace

Chunk::Chunk(int chunkX, int chunkZ)
    : m_chunkX(chunkX), m_chunkZ(chunkZ), m_tiles(kVolume, TileID::Air), m_light(kVolume, 0) {}

Chunk::~Chunk() {
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
    }
}

bool Chunk::inBounds(int x, int y, int z) {
    return x >= 0 && x < kWidth && y >= 0 && y < kHeight && z >= 0 && z < kDepth;
}

int Chunk::index(int x, int y, int z) {
    return (y * kDepth + z) * kWidth + x;
}

TileID Chunk::tileAt(int x, int y, int z) const {
    if (!inBounds(x, y, z)) {
        return TileID::Air;
    }
    return m_tiles[index(x, y, z)];
}

void Chunk::setTile(int x, int y, int z, TileID id) {
    if (!inBounds(x, y, z)) {
        return;
    }
    TileID& current = m_tiles[index(x, y, z)];
    if (current == id) {
        return;
    }
    current = id;
    m_dirty = true;
}

std::uint8_t Chunk::lightAt(int x, int y, int z) const {
    if (!inBounds(x, y, z)) {
        return 0;
    }
    return m_light[index(x, y, z)];
}

void Chunk::setLightAt(int x, int y, int z, std::uint8_t packed) {
    if (!inBounds(x, y, z)) {
        return;
    }
    m_light[index(x, y, z)] = packed;
}

int Chunk::sunlightAt(int x, int y, int z) const {
    return (lightAt(x, y, z) >> 4) & 0x0F;
}

void Chunk::setSunlightAt(int x, int y, int z, int level) {
    const std::uint8_t clamped = static_cast<std::uint8_t>(std::clamp(level, 0, 15) << 4);
    setLightAt(x, y, z, (lightAt(x, y, z) & 0x0F) | clamped);
}

int Chunk::blocklightAt(int x, int y, int z) const {
    return lightAt(x, y, z) & 0x0F;
}

void Chunk::setBlocklightAt(int x, int y, int z, int level) {
    const std::uint8_t clamped = static_cast<std::uint8_t>(std::clamp(level, 0, 15));
    setLightAt(x, y, z, (lightAt(x, y, z) & 0xF0) | clamped);
}

glm::vec3 Chunk::worldOrigin() const {
    return glm::vec3(m_chunkX * kWidth, 0.0f, m_chunkZ * kDepth);
}

glm::vec3 Chunk::center() const {
    return worldOrigin() + glm::vec3(kWidth * 0.5f, 0.0f, kDepth * 0.5f);
}

bool Chunk::faceVisible(const World& world, int x, int y, int z, TileID self) const {
    if (y < 0 || y >= kHeight) {
        return true;
    }

    TileID neighbor = inBounds(x, y, z)
                          ? m_tiles[index(x, y, z)]
                          : world.tileAt(m_chunkX * kWidth + x, y, m_chunkZ * kDepth + z);

    if (Tile::isAir(neighbor)) {
        return true;
    }
    // A transparent neighbor of a different kind still leaves the face visible.
    return Tile::isTransparent(neighbor) && neighbor != self;
}

void Chunk::appendFace(std::vector<float>& vertices, Face face, int x, int y, int z,
                       TileID id, const float cornerLights[4]) const {
    const FaceGeometry& geometry = kFaces[static_cast<std::size_t>(face)];
    const glm::vec3 color = Tile::faceColor(id, face);
    const glm::vec3 base = worldOrigin() + glm::vec3(x, y, z);

    static constexpr std::array<int, 6> kTriangleOrder = {0, 1, 2, 2, 3, 0};
    for (int corner : kTriangleOrder) {
        const glm::vec3 position = base + geometry.corners[static_cast<std::size_t>(corner)];
        vertices.insert(vertices.end(), {position.x, position.y, position.z,
                                         color.r, color.g, color.b,
                                         cornerLights[corner]});
    }
}

void Chunk::buildMesh(const World& world) {
    std::vector<float> vertices;
    vertices.reserve(4096);

    for (int y = 0; y < kHeight; ++y) {
        for (int z = 0; z < kDepth; ++z) {
            for (int x = 0; x < kWidth; ++x) {
                const TileID id = m_tiles[index(x, y, z)];
                if (Tile::isAir(id)) {
                    continue;
                }

                const glm::ivec3 blockWorldPos = glm::ivec3(worldOrigin()) + glm::ivec3(x, y, z);

                for (std::size_t f = 0; f < kFaces.size(); ++f) {
                    const FaceGeometry& face = kFaces[f];
                    const glm::ivec3 n = face.normal;
                    if (faceVisible(world, x + n.x, y + n.y, z + n.z, id)) {
                        const glm::ivec3 frontBlock = blockWorldPos + n;
                        const float frontLight = world.sampleLight(frontBlock.x, frontBlock.y, frontBlock.z);

                        float cornerLights[4];
                        for (int c = 0; c < 4; ++c) {
                            cornerLights[c] = cornerLight(face, c, blockWorldPos, world, frontLight);
                        }

                        appendFace(vertices, static_cast<Face>(f), x, y, z, id, cornerLights);
                    }
                }
            }
        }
    }

    uploadMesh(vertices);
    m_dirty = false;
}

void Chunk::uploadMesh(const std::vector<float>& vertices) {
    constexpr GLsizei kFloatsPerVertex = 7;
    m_vertexCount = static_cast<GLsizei>(vertices.size()) / kFloatsPerVertex;

    if (m_vao == 0) {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
    }

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.empty() ? nullptr : vertices.data(), GL_STATIC_DRAW);

    const GLsizei stride = kFloatsPerVertex * static_cast<GLsizei>(sizeof(float));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Chunk::draw() const {
    if (m_vao == 0 || m_vertexCount == 0) {
        return;
    }
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    glBindVertexArray(0);
}
