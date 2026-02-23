#version 460 core

// Input from vertex shader
in vec3 Color;

// Output color
out vec4 FragColor;

void main()
{
    FragColor = vec4(Color, 1.0);
}
