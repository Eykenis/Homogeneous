/**
 * VOX File Reader Implementation
 *
 * Implements parsing of MagicaVoxel .vox format files.
 */

#include "vox_reader.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <stdexcept>

// Magic number for VOX files
static constexpr char VOX_MAGIC[4] = {'V', 'O', 'X', ' '};

// Current VOX version
static constexpr int VOX_VERSION = 150;

// Chunk IDs
static constexpr char CHUNK_ID_MAIN[4] = {'M', 'A', 'I', 'N'};
static constexpr char CHUNK_ID_SIZE[4] = {'S', 'I', 'Z', 'E'};
static constexpr char CHUNK_ID_XYZI[4] = {'X', 'Y', 'Z', 'I'};
static constexpr char CHUNK_ID_RGBA[4] = {'R', 'G', 'B', 'A'};

VoxFile VoxReader::load(const std::string& filepath) {
    VoxFile voxFile;
    voxFile.version = 0;
    voxFile.models.clear();
    initializeDefaultPalette(voxFile.palette);

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open VOX file: " + filepath);
    }

    // Read and verify magic number
    char magic[4];
    file.read(magic, 4);
    if (std::memcmp(magic, VOX_MAGIC, 4) != 0) {
        throw std::runtime_error("Invalid VOX file: magic number mismatch");
    }

    // Read version
    uint32_t version = readInt32(file);
    voxFile.version = static_cast<int>(version);

    // Read main chunk header
    char chunkId[4];
    uint32_t contentSize;
    uint32_t childrenSize;

    readChunkHeader(file, chunkId, contentSize, childrenSize);

    if (std::memcmp(chunkId, CHUNK_ID_MAIN, 4) != 0) {
        throw std::runtime_error("Invalid VOX file: expected MAIN chunk");
    }

    // Skip main chunk content (should be empty)
    if (contentSize > 0) {
        skipChunkContent(file, contentSize);
    }

    // Parse children of MAIN chunk
    if (childrenSize > 0) {
        parseMainChunk(file, voxFile);
    }

    file.close();
    return voxFile;
}

bool VoxReader::isValidVoxFile(const std::string& filepath) {
    try {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        char magic[4];
        file.read(magic, 4);
        file.close();

        return std::memcmp(magic, VOX_MAGIC, 4) == 0;
    } catch (...) {
        return false;
    }
}

void VoxReader::initializeDefaultPalette(std::array<RGBAColor, 256>& palette) {
    // Initialize with a simple default palette
    // Index 0 is transparent
    palette[0] = {0, 0, 0, 0};
    
    // Generate a simple color palette for indices 1-255
    for (int i = 1; i < 256; ++i) {
        // Create a varied palette using HSV-like distribution
        float hue = (i - 1) / 255.0f * 6.0f;  // 0-6 range for color wheel
        int region = static_cast<int>(hue);
        float frac = hue - region;
        
        uint8_t r, g, b;
        int brightness = 128 + (i % 128);  // Vary brightness
        
        switch (region % 6) {
            case 0: r = brightness; g = static_cast<uint8_t>(brightness * frac); b = 0; break;
            case 1: r = static_cast<uint8_t>(brightness * (1 - frac)); g = brightness; b = 0; break;
            case 2: r = 0; g = brightness; b = static_cast<uint8_t>(brightness * frac); break;
            case 3: r = 0; g = static_cast<uint8_t>(brightness * (1 - frac)); b = brightness; break;
            case 4: r = static_cast<uint8_t>(brightness * frac); g = 0; b = brightness; break;
            default: r = brightness; g = 0; b = static_cast<uint8_t>(brightness * (1 - frac)); break;
        }
        
        palette[i] = {r, g, b, 255};
    }
}

uint32_t VoxReader::readInt32(std::ifstream& file) {
    uint8_t bytes[4];
    file.read(reinterpret_cast<char*>(bytes), 4);

    // Little-endian conversion
    return static_cast<uint32_t>(bytes[0]) |
           (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) |
           (static_cast<uint32_t>(bytes[3]) << 24);
}

void VoxReader::readChunkHeader(std::ifstream& file, char chunkId[4], uint32_t& contentSize, uint32_t& childrenSize) {
    file.read(chunkId, 4);
    contentSize = readInt32(file);
    childrenSize = readInt32(file);
}

void VoxReader::parseMainChunk(std::ifstream& file, VoxFile& voxFile) {
    // Keep reading chunks until we reach the end
    while (file.good() && !file.eof()) {
        char chunkId[4];
        uint32_t contentSize;
        uint32_t childrenSize;

        // Try to read chunk header
        file.read(chunkId, 4);

        // Check if we've reached end of file
        if (file.gcount() < 4) {
            break;
        }

        contentSize = readInt32(file);
        childrenSize = readInt32(file);

        if (std::memcmp(chunkId, CHUNK_ID_SIZE, 4) == 0) {
            // SIZE chunk - start a new model
            VoxelModel model;
            parseSizeChunk(file, model);

            // Skip children if any
            if (childrenSize > 0) {
                skipChunkContent(file, childrenSize);
            }

            // Add model to list (XYZI will be in next chunk)
            voxFile.models.push_back(model);

        } else if (std::memcmp(chunkId, CHUNK_ID_XYZI, 4) == 0) {
            // XYZI chunk - voxel data for the last model
            if (!voxFile.models.empty()) {
                parseXYZIChunk(file, voxFile.models.back());
            } else {
                // No model to attach to, skip
                skipChunkContent(file, contentSize);
            }

            // Skip children if any
            if (childrenSize > 0) {
                skipChunkContent(file, childrenSize);
            }

        } else if (std::memcmp(chunkId, CHUNK_ID_RGBA, 4) == 0) {
            // RGBA chunk - color palette
            parseRGBAChunk(file, voxFile);

            // Skip children if any
            if (childrenSize > 0) {
                skipChunkContent(file, childrenSize);
            }

        } else {
            // Unknown chunk - skip content and children
            skipChunkContent(file, contentSize);
            if (childrenSize > 0) {
                skipChunkContent(file, childrenSize);
            }
        }
    }
}

void VoxReader::parseSizeChunk(std::ifstream& file, VoxelModel& model) {
    model.sizeX = static_cast<int>(readInt32(file));
    model.sizeY = static_cast<int>(readInt32(file));
    model.sizeZ = static_cast<int>(readInt32(file));
    model.voxels.clear();
}

void VoxReader::parseXYZIChunk(std::ifstream& file, VoxelModel& model) {
    uint32_t numVoxels = readInt32(file);
    model.voxels.reserve(numVoxels);

    for (uint32_t i = 0; i < numVoxels; ++i) {
        VoxData voxel;
        file.read(reinterpret_cast<char*>(&voxel.x), 1);
        file.read(reinterpret_cast<char*>(&voxel.z), 1);  // Note: VOX stores z before y
        file.read(reinterpret_cast<char*>(&voxel.y), 1);
        file.read(reinterpret_cast<char*>(&voxel.colorIndex), 1);
        model.voxels.push_back(voxel);
    }
}

void VoxReader::parseRGBAChunk(std::ifstream& file, VoxFile& voxFile) {
    for (int i = 0; i < 256; ++i) {
        file.read(reinterpret_cast<char*>(&voxFile.palette[i].r), 1);
        file.read(reinterpret_cast<char*>(&voxFile.palette[i].g), 1);
        file.read(reinterpret_cast<char*>(&voxFile.palette[i].b), 1);
        file.read(reinterpret_cast<char*>(&voxFile.palette[i].a), 1);
    }
}

void VoxReader::skipChunkContent(std::ifstream& file, uint32_t size) {
    file.seekg(static_cast<std::streamoff>(size), std::ios::cur);
}
