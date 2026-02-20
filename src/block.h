/**
 * Block Class
 *
 * Represents a single voxel block in the scene:
 * - Stores block position and type
 * - Provides texture coordinate offset support
 */

#ifndef BLOCK_H
#define BLOCK_H

class Block {
public:
    int x, y, z; // Block position in world coordinates

    enum BlockType {
        AIR,
        OPACITY,
        SOLID,
    };

    BlockType type;

    // Texture coordinate offset (for texture atlases, currently unused)
    // Values: 0-1, represents UV offset for this block type
    float texUOffset;
    float texVOffset;

    // Constructor to initialize type to AIR by default
    Block() : x(0), y(0), z(0), type(BlockType::AIR), texUOffset(0.0f), texVOffset(0.0f) {}
};

#endif
