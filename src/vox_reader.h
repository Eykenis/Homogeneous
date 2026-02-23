/**
 * VOX File Reader
 *
 * Reads MagicaVoxel .vox format files.
 * The .vox format is a binary chunk-based format used for voxel art.
 *
 * Format structure:
 * - Header: Magic "VOX " + version (4 bytes each, total 8 bytes)
 * - Chunk: ID (4 bytes) + contentSize (4 bytes) + childrenSize (4 bytes) + content + children
 *
 * Main chunks:
 * - SIZE: Model dimensions (12 bytes: sizeX, sizeY, sizeZ)
 * - XYZI: Voxel data (4 bytes: numVoxels + numVoxels * 4 bytes per voxel)
 * - RGBA: Color palette (1024 bytes: 256 * 4 bytes for r,g,b,a)
 */

#ifndef VOX_READER_H
#define VOX_READER_H

#include <vector>
#include <array>
#include <cstdint>
#include <string>

/**
 * Single voxel with position and color index (VOX file format)
 */
struct VoxData {
    uint8_t x;          // X position in voxel space [0, 255]
    uint8_t y;          // Y position in voxel space [0, 255]
    uint8_t z;          // Z position in voxel space [0, 255]
    uint8_t colorIndex; // Color palette index [1, 255], 0 is reserved/transparent
};

/**
 * RGBA color for palette entries
 */
struct RGBAColor {
    uint8_t r; // Red channel [0, 255]
    uint8_t g; // Green channel [0, 255]
    uint8_t b; // Blue channel [0, 255]
    uint8_t a; // Alpha channel [0, 255]
};

/**
 * A single model in a VOX file
 * (VOX files can contain multiple models)
 */
struct VoxelModel {
    int sizeX;                   // Model X dimension
    int sizeY;                   // Model Y dimension
    int sizeZ;                   // Model Z dimension
    std::vector<VoxData> voxels; // List of voxels in this model
};

/**
 * Complete VOX file representation
 */
struct VoxFile {
    int version;                            // File version (typically 150 for MagicaVoxel)
    std::vector<VoxelModel> models;         // All models in the file
    std::array<RGBAColor, 256> palette;    // Color palette (index 0 is unused)
};

/**
 * VOX file reader class
 * Provides static methods to load and parse .vox files
 */
class VoxReader {
public:
    /**
     * Load a VOX file from the given path
     * @param filepath Path to the .vox file
     * @return VoxFile structure containing the loaded data
     * @throws std::runtime_error if file cannot be opened or parsing fails
     */
    static VoxFile load(const std::string& filepath);

    /**
     * Check if a file has a valid .vox header
     * @param filepath Path to the file to check
     * @return true if the file appears to be a valid VOX file
     */
    static bool isValidVoxFile(const std::string& filepath);

private:
    /**
     * Initialize the default MagicaVoxel palette
     * @param palette The palette array to fill
     */
    static void initializeDefaultPalette(std::array<RGBAColor, 256>& palette);

    /**
     * Read a little-endian 32-bit integer from a binary stream
     * @param file The input file stream
     * @return The read integer value
     */
    static uint32_t readInt32(std::ifstream& file);

    /**
     * Read a chunk header from a binary stream
     * @param file The input file stream
     * @param chunkId Output parameter for chunk ID (4 chars)
     * @param contentSize Output parameter for content size
     * @param childrenSize Output parameter for children size
     */
    static void readChunkHeader(std::ifstream& file, char chunkId[4], uint32_t& contentSize, uint32_t& childrenSize);

    /**
     * Parse the main chunk and its children
     * @param file The input file stream positioned at the main chunk content
     * @param voxFile The VoxFile structure to populate
     */
    static void parseMainChunk(std::ifstream& file, VoxFile& voxFile);

    /**
     * Parse a SIZE chunk
     * @param file The input file stream positioned at SIZE chunk content
     * @param model The model to populate with dimensions
     */
    static void parseSizeChunk(std::ifstream& file, VoxelModel& model);

    /**
     * Parse an XYZI chunk (voxel data)
     * @param file The input file stream positioned at XYZI chunk content
     * @param model The model to populate with voxels
     */
    static void parseXYZIChunk(std::ifstream& file, VoxelModel& model);

    /**
     * Parse an RGBA chunk (color palette)
     * @param file The input file stream positioned at RGBA chunk content
     * @param voxFile The VoxFile structure to populate with palette data
     */
    static void parseRGBAChunk(std::ifstream& file, VoxFile& voxFile);

    /**
     * Skip a chunk's content (for unknown/unimplemented chunks)
     * @param file The input file stream
     * @param size The size of content to skip
     */
    static void skipChunkContent(std::ifstream& file, uint32_t size);
};

#endif // VOX_READER_H
