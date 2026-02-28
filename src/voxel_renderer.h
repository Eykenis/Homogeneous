#ifndef VOXEL_RENDERER_H
#define VOXEL_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "shader.h"
#include "voxel.h"

struct OctreeNode {
    uint8_t childMask = 0;
    std::shared_ptr<OctreeNode> children[8];
    glm::vec4 color = glm::vec4(0.0f);
    bool leaf = false;
}; // only for cpu & memory.

bool allPointsSameColor(const std::vector<Voxel>& points);
std::shared_ptr<OctreeNode> buildOctree(const std::vector<Voxel>& points, glm::vec3 min, glm::vec3 max, int depth);

class VoxelRenderer
{
public:
    VoxelRenderer();
    ~VoxelRenderer();

    void init();
    void render(int width, int height);
    void cleanup();

    void setCameraPos(const glm::vec3& pos) { cameraPos = pos; }
    void setCameraTarget(const glm::vec3& target) { cameraTarget = target; }
    void setFOV(float fovValue) { this->fov = fovValue; }

    void setVoxels(const std::vector<Voxel>& voxels);
    void addVoxel(const Voxel& voxel);
    void clearVoxels();
    int getVoxelCount() const { return static_cast<int>(voxelData.size()); }

    // public render state
    bool shadow;
    int aoSampleCount;
    bool useVoxelColor;

private:
    void setupQuad();
    void uploadVoxelData();
    void uploadOctreeData();
    void buildOctreeFromVoxels(const std::vector<Voxel>& points);

    Shader* shader;
    GLuint VAO, VBO;
    GLuint ssbo;
    GLuint octreeSSBO;

    glm::vec3 cameraPos;
    glm::vec3 cameraTarget;
    float fov;

    // GPU-side voxel data layout (matches SSBO struct)
    struct GPUVoxel
    {
        glm::vec4 posAndSize;  // xyz = position, w = side length
        glm::vec4 color;       // rgba
    };

    struct GPUNode
    {
        uint32_t childMask;
        uint32_t color; // Packed RGBA as uint32_t
    };

    std::vector<GPUVoxel> voxelData;
    std::vector<GPUNode> octreeData;
    bool voxelDataDirty;
    bool octreeDataDirty;

    // Octree world-space bounds (power-of-two aligned cube)
    glm::vec3 octreeBoundsMin = glm::vec3(-128.0f);
    glm::vec3 octreeBoundsMax = glm::vec3(128.0f);
};

#endif // VOXEL_RENDERER_H
