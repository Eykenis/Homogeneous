/**
 * Texture Class
 *
 * Manages OpenGL texture loading and binding:
 * - Loads texture images using stb_image
 * - Creates and configures OpenGL textures
 * - Supports binding for rendering
 */

#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <string>
#include <iostream>

// Define STB_IMAGE_IMPLEMENTATION in one .cpp file only
#define STB_IMAGE_IMPLEMENTATION

class Texture {
public:
    // Texture ID
    unsigned int ID;
    // Texture width and height
    int width, height;

    /**
     * Constructor - loads texture from file
     * @param path Path to texture file
     * @param texUnit Texture unit to bind to (default 0)
     */
    Texture(const char* path, unsigned int texUnit = 0);

    /**
     * Destructor - cleans up OpenGL resources
     */
    ~Texture();

    /**
     * Bind the texture to the specified texture unit
     */
    void bind(unsigned int texUnit = 0) const;

    /**
     * Unbind the texture
     */
    void unbind() const;
};

#endif
