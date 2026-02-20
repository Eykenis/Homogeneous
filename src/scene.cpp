/**
 * Scene Class Implementation
 */

#include "scene.h"
#include <iostream>

// Texture coordinates for each face (normalized 0-1)
// Each vertex has 2 floats: texU, texV
static const float FACE_TEX_COORDS[6][4][2] = {
    // Front
    {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}},
    // Back
    {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}},
    // Left
    {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}},
    // Right
    {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}},
    // Top
    {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}},
    // Bottom
    {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}
};

Scene::Scene() : VAO(0), VBO(0), vertexCount(0), initialized(false) {
    // Blocks are automatically initialized to AIR via Block constructor
}

Scene::~Scene() {
    // Clean up textures
    for (auto& pair : textures) {
        delete pair.second;
    }
    textures.clear();

    // Clean up OpenGL resources
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
    }
}

void Scene::loadTextures() {
    // Load texture for SOLID blocks
    Texture* solidTexture = new Texture("../../../assets/textures/stone.png");
    if (solidTexture->ID != 0) {
        textures[Block::BlockType::SOLID] = solidTexture;
    } else {
        delete solidTexture;
    }
}

void Scene::init() {
    if (initialized) {
        std::cout << "Scene already initialized" << std::endl;
        return;
    }

    // Load textures
    loadTextures();

    // Generate default terrain
    generateDefaultTerrain();

    // Generate mesh from block data
    generateMesh();

    // Create VAO and VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind VAO
    glBindVertexArray(VAO);

    // Bind and set VBO data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Set vertex attributes
    // Position (x, y, z) - location 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinates (texU, texV) - location 1
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    initialized = true;
    std::cout << "Scene initialized with " << vertexCount / 3 << " vertices" << std::endl;
}

void Scene::render(const Shader& shader, const Camera& camera) const {
    if (!initialized) {
        std::cerr << "Cannot render uninitialized scene" << std::endl;
        return;
    }

    // Use shader
    shader.use();

    // Bind texture
    auto it = textures.find(Block::BlockType::SOLID);
    if (it != textures.end()) {
        it->second->bind(0);
        shader.setInt("textureSampler", 0);
    } else {
        std::cerr << "WARNING: No texture found for SOLID blocks!" << std::endl;
    }

    // Set view-projection matrix
    shader.setMatrix4fv("viewProjection", camera.getViewProjectionMatrix());

    // Bind VAO and draw
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);

    // Unbind texture
    if (it != textures.end()) {
        it->second->unbind();
    }
}

Block& Scene::getBlock(int x, int y, int z) {
    return blocks[toIndex(x, y, z)];
}

const Block& Scene::getBlock(int x, int y, int z) const {
    return blocks[toIndex(x, y, z)];
}

void Scene::setBlock(int x, int y, int z, Block::BlockType type) {
    blocks[toIndex(x, y, z)].type = type;
}

void Scene::updateMesh() {
    if (!initialized) {
        std::cerr << "Cannot update mesh of uninitialized scene" << std::endl;
        return;
    }

    // Regenerate mesh
    generateMesh();

    // Update VBO data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    std::cout << "Mesh updated with " << vertexCount / 3 << " vertices" << std::endl;
}

void Scene::generateDefaultTerrain() {
    // Create a simple ground plane at y=0
    for (int x = 0; x < WIDTH; ++x) {
        for (int z = 0; z < DEPTH; ++z) {
            // Set block at y=0 to SOLID
            setBlock(x, 0, z, Block::BlockType::SOLID);
        }
    }

    // Add some additional blocks for testing
    for (int x = 5; x < 11; ++x) {
        for (int z = 5; z < 11; ++z) {
            for (int y = 1; y < 3; ++y) {
                setBlock(x, y, z, Block::BlockType::SOLID);
            }
        }
    }
}

bool Scene::isValidPosition(int x, int y, int z) const {
    return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT && z >= 0 && z < DEPTH;
}

int Scene::toIndex(int x, int y, int z) const {
    return x + y * WIDTH + z * WIDTH * HEIGHT;
}

void Scene::generateMesh() {
    vertices.clear();
    vertexCount = 0;

    for (int x = 0; x < WIDTH; ++x) {
        for (int y = 0; y < HEIGHT; ++y) {
            for (int z = 0; z < DEPTH; ++z) {
                Block& block = blocks[toIndex(x, y, z)];

                // Only generate mesh for SOLID blocks
                if (block.type != Block::BlockType::SOLID) {
                    continue;
                }

                // Check each face
                for (int face = 0; face < 6; ++face) {
                    if (shouldRenderFace(x, y, z, face)) {
                        addCubeFace((float)x, (float)y, (float)z, face, block);
                    }
                }
            }
        }
    }
}

bool Scene::shouldRenderFace(int x, int y, int z, int face) const {
    // Get neighbor position
    int nx = x, ny = y, nz = z;

    switch (face) {
        case 0: nz = z + 1; break;  // Front
        case 1: nz = z - 1; break;  // Back
        case 2: nx = x - 1; break;  // Left
        case 3: nx = x + 1; break;  // Right
        case 4: ny = y + 1; break;  // Top
        case 5: ny = y - 1; break;  // Bottom
    }

    // If neighbor is out of bounds, render the face
    if (!isValidPosition(nx, ny, nz)) {
        return true;
    }

    // If neighbor is not solid, render the face
    return blocks[toIndex(nx, ny, nz)].type != Block::BlockType::SOLID;
}

void Scene::addCubeFace(float x, float y, float z, int face, const Block& block) {
    // Vertex positions for each face (two triangles per face)
    // Triangle 1: vertex 0, 1, 2
    // Triangle 2: vertex 3, 4, 5 (using same corners as triangle 1)
    static const int vertexIndices[6] = {0, 1, 2, 0, 2, 3};

    // Vertex positions for each face
    static const float facePositions[6][4][3] = {
        // Front (+Z)
        {{0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
        // Back (-Z)
        {{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}},
        // Left (-X)
        {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        // Right (+X)
        {{1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        // Top (+Y)
        {{0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        // Bottom (-Y)
        {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}
    };

    // Add vertices with texture coordinates
    for (int i = 0; i < 6; ++i) {
        int cornerIdx = vertexIndices[i];

        // Position
        vertices.push_back(x + facePositions[face][cornerIdx][0]);
        vertices.push_back(y + facePositions[face][cornerIdx][1]);
        vertices.push_back(z + facePositions[face][cornerIdx][2]);

        // Texture coordinates (with offset for texture atlas support)
        vertices.push_back(FACE_TEX_COORDS[face][cornerIdx][0] + block.texUOffset);
        vertices.push_back(FACE_TEX_COORDS[face][cornerIdx][1] + block.texVOffset);
    }

    vertexCount += 6;
}
