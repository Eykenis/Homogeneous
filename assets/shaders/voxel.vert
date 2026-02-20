#version 330 core

// Vertex position
layout (location = 0) in vec3 aPos;
// Texture coordinates
layout (location = 1) in vec2 aTexCoord;

// Output to fragment shader
out vec2 TexCoord;

// View-projection matrix
uniform mat4 viewProjection;

void main()
{
    gl_Position = viewProjection * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
