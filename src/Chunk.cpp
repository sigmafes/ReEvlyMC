#include "Chunk.h"

#include "World.h"

namespace {

struct FaceGeometry {
    glm::ivec3 normal;
    std::array<glm::vec3, 4> corners;
    float shade;
};

// Corner order: two triangles are emitted as 0-1-2 and 2-3-0.
const std::array<FaceGeometry, 6> kFaces = {{
    // Top (+Y)
    {{0, 1, 0}, {{{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}}, 1.00f},
    // Bottom (-Y)
    {{0, -1, 0}, {{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}}, 0.55f},
    // North (+Z)
    {{0, 0, 1}, {{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}}, 0.80f},
    // South (-Z)
    {{0, 0, -1}, {{{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}}}, 0.80f},
    // East (+X)
    {{1, 0, 0}, {{{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}}}, 0.68f},
    // West (-X)
    {{-1, 0, 0}, {{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}}, 0.68f},
}};

}  // namespace

Chunk::Chunk(int chunkX, int chunkZ)
    : m_chunkX(chunkX), m_chunkZ(chunkZ), m_tiles(kVolume, TileID::Air) {}

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
                       TileID id) const {
    const FaceGeometry& geometry = kFaces[static_cast<std::size_t>(face)];
    const glm::vec3 color = Tile::faceColor(id, face) * geometry.shade;
    const glm::vec3 base = worldOrigin() + glm::vec3(x, y, z);

    static constexpr std::array<int, 6> kTriangleOrder = {0, 1, 2, 2, 3, 0};
    for (int corner : kTriangleOrder) {
        const glm::vec3 position = base + geometry.corners[static_cast<std::size_t>(corner)];
        vertices.insert(vertices.end(), {position.x, position.y, position.z,
                                         color.r, color.g, color.b});
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

                for (std::size_t f = 0; f < kFaces.size(); ++f) {
                    const glm::ivec3& n = kFaces[f].normal;
                    if (faceVisible(world, x + n.x, y + n.y, z + n.z, id)) {
                        appendFace(vertices, static_cast<Face>(f), x, y, z, id);
                    }
                }
            }
        }
    }

    uploadMesh(vertices);
    m_dirty = false;
}

void Chunk::uploadMesh(const std::vector<float>& vertices) {
    constexpr GLsizei kFloatsPerVertex = 6;
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
