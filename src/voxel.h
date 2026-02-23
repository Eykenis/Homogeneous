/**
 * Voxel Class
 *
 * Core voxel representation for the rendering system.
 * Provides a more feature-rich representation than the basic VOX file format,
 * suitable for GPU rendering and runtime manipulation.
 */

#ifndef VOXEL_H
#define VOXEL_H

#include <cstdint>
#include <glm/glm.hpp>

/**
 * Voxel class representing a single volumetric pixel
 *
 * This class extends the basic VOX format with:
 * - Normalized color representation (float RGBA)
 * - World-space position support
 * - Material properties for rendering
 */
class Voxel {
public:
    // Constructors
    Voxel();
    Voxel(uint8_t x, uint8_t y, uint8_t z, uint8_t colorIndex);
    Voxel(const glm::ivec3& position, const glm::vec4& color);
    Voxel(int x, int y, int z, float r, float g, float b, float a = 1.0f);

    // Position accessors (voxel grid coordinates)
    glm::ivec3 getPosition() const { return position; }
    void setPosition(const glm::ivec3& pos) { position = pos; }
    void setPosition(int x, int y, int z) { position = glm::ivec3(x, y, z); }

    // Color accessors (normalized RGBA [0.0, 1.0])
    glm::vec4 getColor() const { return color; }
    void setColor(const glm::vec4& col) { color = col; }
    void setColor(float r, float g, float b, float a = 1.0f) { color = glm::vec4(r, g, b, a); }

    // Convert to/from VOX file format
    void setFromVoxFormat(uint8_t x, uint8_t y, uint8_t z, uint8_t colorIndex, const glm::vec4& paletteColor);
    uint8_t getColorIndex() const { return colorIndex; }

    // Material properties
    float getEmission() const { return emission; }
    void setEmission(float e) { emission = glm::clamp(e, 0.0f, 1.0f); }

    float getRoughness() const { return roughness; }
    void setRoughness(float r) { roughness = glm::clamp(r, 0.0f, 1.0f); }

    float getMetallic() const { return metallic; }
    void setMetallic(float m) { metallic = glm::clamp(m, 0.0f, 1.0f); }

    // Utility
    bool isTransparent() const { return color.a < 1.0f; }
    bool isEmissive() const { return emission > 0.0f; }

private:
    glm::ivec3 position;    // Voxel grid position (integer coordinates)
    glm::vec4 color;        // RGBA color (normalized [0.0, 1.0])
    uint8_t colorIndex;     // Original palette index from VOX file (0 if not from file)

    // Material properties for stylized rendering
    float emission;         // Emission strength [0.0, 1.0]
    float roughness;        // Surface roughness [0.0, 1.0]
    float metallic;         // Metallic property [0.0, 1.0]
};

#endif // VOXEL_H
