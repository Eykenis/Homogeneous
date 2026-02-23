#version 330 core

// Vertex position
layout (location = 0) in vec3 aPos;
// Vertex color
layout (location = 1) in vec3 aColor;

// Output to fragment shader
out vec3 Color;

// View-projection matrix
uniform mat4 viewProjection;

struct Node {
    uint childrenMask; // 8位，标记哪个子象限有物体
    uint childPointer; // 指向子节点在数组中的起始索引
    vec4 color;        // 当前节点的颜色（或平均色）
};

layout(std430, binding = 0) buffer OctreeData {
    Node nodes[]; // 整个八叉树被压扁在这个数组里
};

void main()
{
    gl_Position = viewProjection * vec4(aPos, 1.0);
    Color = aColor;
}
