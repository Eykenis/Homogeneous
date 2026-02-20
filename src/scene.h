/**
 * Scene Class
 *
 * Manages a voxel scene composed of blocks:
 * - Contains a 16x16x256 3D array of blocks
 * - Provides OpenGL rendering functionality using VAO/VBO
 * - Generates mesh geometry from block data
 * - Supports texturing for blocks
 */

#ifndef SCENE_H
#define SCENE_H

#include "block.h"
#include "texture.h"
#include "shader.h"
#include "camera.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <unordered_map>

class Scene {
public:
    // Scene dimensions
    static constexpr int WIDTH = 16;
    static constexpr int HEIGHT = 256;
    static constexpr int DEPTH = 16;
    static constexpr int TOTAL_BLOCKS = WIDTH * HEIGHT * DEPTH;

    /**
     * Constructor - initializes the scene with empty blocks
     */
    Scene();

    /**
     * Destructor - cleans up OpenGL resources
     */
    ~Scene();

    /**
     * Initialize the scene
     * Sets up OpenGL buffers and generates default terrain
     */
    void init();

    /**
     * Render the scene with the given shader and camera
     * @param shader The shader program to use for rendering
     * @param camera The camera providing view/projection matrices
     */
    void render(const Shader& shader, const Camera& camera) const;

    /**
     * Get a block at the specified position
     * @param x X coordinate [0, WIDTH-1]
     * @param y Y coordinate [0, HEIGHT-1]
     * @param z Z coordinate [0, DEPTH-1]
     * @return Reference to the block
     */
    Block& getBlock(int x, int y, int z);

    /**
     * Get a block at the specified position (const version)
     */
    const Block& getBlock(int x, int y, int z) const;

    /**
     * Set a block at the specified position
     * @param x X coordinate [0, WIDTH-1]
     * @param y Y coordinate [0, HEIGHT-1]
     * @param z Z coordinate [0, DEPTH-1]
     * @param type The block type to set
     */
    void setBlock(int x, int y, int z, Block::BlockType type);

    /**
     * Regenerate the mesh geometry from block data
     * Call this after modifying blocks
     */
    void updateMesh();

    /**
     * Check if the scene has been initialized
     */
    bool isInitialized() const { return initialized; }

private:
    // 3D array of blocks
    std::array<Block, TOTAL_BLOCKS> blocks;

    // OpenGL buffers
    unsigned int VAO, VBO;

    // Mesh data
    std::vector<float> vertices;
    int vertexCount;

    // Initialization state
    bool initialized;

    // Textures for each block type
    std::unordered_map<Block::BlockType, Texture*> textures;

    /**
     * Load textures for block types
     */
    void loadTextures();

    /**
     * Initialize default terrain (simple ground plane)
     */
    void generateDefaultTerrain();

    /**
     * Check if a position is valid within the scene bounds
     */
    bool isValidPosition(int x, int y, int z) const;

    /**
     * Convert 3D coordinates to 1D index
     */
    int toIndex(int x, int y, int z) const;

    /**
     * Generate mesh vertices from blocks (naive approach - renders all solid blocks)
     */
    void generateMesh();

    /**
     * Check if a face should be rendered (neighbors check)
     */
    bool shouldRenderFace(int x, int y, int z, int face) const;

    /**
     * Add a cube face to the vertex buffer
     * @param x, y, z Block world position
     * @param face Face index: 0=front, 1=back, 2=left, 3=right, 4=top, 5=bottom
     * @param block The block to get texture info from
     */
    void addCubeFace(float x, float y, float z, int face, const Block& block);
};

#endif
