/**
 * Voxel Class Implementation
 */

#include "voxel.h"
#include <algorithm>

// Default constructor - creates a transparent voxel at origin
Voxel::Voxel()
    : position(0, 0, 0)
    , color(0.0f, 0.0f, 0.0f, 0.0f)
    , colorIndex(0)
    , emission(0.0f)
    , roughness(0.5f)
    , metallic(0.0f)
{
}

// Constructor from VOX format (uint8_t coordinates and color index)
Voxel::Voxel(uint8_t x, uint8_t y, uint8_t z, uint8_t colorIndex)
    : position(static_cast<int>(x), static_cast<int>(y), static_cast<int>(z))
    , color(1.0f, 1.0f, 1.0f, 1.0f)  // Default white, should be set from palette
    , colorIndex(colorIndex)
    , emission(0.0f)
    , roughness(0.5f)
    , metallic(0.0f)
{
}

// Constructor from position and color vectors
Voxel::Voxel(const glm::ivec3& position, const glm::vec4& color)
    : position(position)
    , color(color)
    , colorIndex(0)
    , emission(0.0f)
    , roughness(0.5f)
    , metallic(0.0f)
{
}

// Constructor from individual components
Voxel::Voxel(int x, int y, int z, float r, float g, float b, float a)
    : position(x, y, z)
    , color(r, g, b, a)
    , colorIndex(0)
    , emission(0.0f)
    , roughness(0.5f)
    , metallic(0.0f)
{
}

// Set voxel data from VOX file format
void Voxel::setFromVoxFormat(uint8_t x, uint8_t y, uint8_t z, uint8_t colorIndex, const glm::vec4& paletteColor) {
    position = glm::ivec3(static_cast<int>(x), static_cast<int>(y), static_cast<int>(z));
    color = paletteColor;
    this->colorIndex = colorIndex;
}
