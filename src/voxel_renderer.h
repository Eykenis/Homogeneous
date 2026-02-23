#ifndef VOXEL_RENDERER_H
#define VOXEL_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "shader.h"
#include "voxel.h"

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
    void setFOV(float fov) { this->fov = fov; }

    void setVoxels(const std::vector<Voxel>& voxels);
    void addVoxel(const Voxel& voxel);
    void clearVoxels();
    int getVoxelCount() const { return static_cast<int>(voxelData.size()); }

private:
    void setupQuad();
    void uploadVoxelData();

    Shader* shader;
    GLuint VAO, VBO;
    GLuint ssbo;

    glm::vec3 cameraPos;
    glm::vec3 cameraTarget;
    float fov;

    // GPU-side voxel data layout (matches SSBO struct)
    struct GPUVoxel
    {
        glm::vec4 posAndSize;  // xyz = position, w = unused
        glm::vec4 color;       // rgba
    };

    std::vector<GPUVoxel> voxelData;
    bool voxelDataDirty;
};

#endif // VOXEL_RENDERER_H
