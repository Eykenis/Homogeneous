#include "voxel_renderer.h"
#include <iostream>
#include <cstring>

VoxelRenderer::VoxelRenderer()
    : shader(nullptr)
    , VAO(0)
    , VBO(0)
    , ssbo(0)
    , cameraPos(0.0f, 0.0f, 5.0f)
    , cameraTarget(0.0f, 0.0f, 0.0f)
    , fov(60.0f)
    , voxelDataDirty(true)
{
}

VoxelRenderer::~VoxelRenderer()
{
    cleanup();
}

void VoxelRenderer::init()
{
    shader = new Shader("assets/shaders/raymarching.vert", "assets/shaders/raymarching.frag");
    setupQuad();

    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    std::cout << "VoxelRenderer initialized" << std::endl;
}

void VoxelRenderer::setupQuad()
{
    float quadVertices[] = {
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void VoxelRenderer::uploadVoxelData()
{
    if (!voxelDataDirty) return;

    // SSBO layout: [int voxelCount, int pad0, int pad1, int pad2, GPUVoxel[] voxels]
    int count = static_cast<int>(voxelData.size());
    size_t headerSize = sizeof(int) * 4;  // 16 bytes for alignment (std430)
    size_t dataSize = headerSize + voxelData.size() * sizeof(GPUVoxel);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataSize, nullptr, GL_DYNAMIC_DRAW);

    // Upload count
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &count);

    // Upload voxel array
    if (!voxelData.empty())
    {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, headerSize, voxelData.size() * sizeof(GPUVoxel), voxelData.data());
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    voxelDataDirty = false;
}

void VoxelRenderer::render(int width, int height)
{
    uploadVoxelData();

    shader->use();
    shader->setVec3("u_cameraPos", cameraPos);
    shader->setVec3("u_cameraTarget", cameraTarget);
    shader->setFloat("u_fov", fov);
    shader->setVec2("u_resolution", glm::vec2(width, height));
    shader->setFloat("u_voxelSize", 1.0f);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void VoxelRenderer::cleanup()
{
    if (VAO != 0) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    if (VBO != 0) { glDeleteBuffers(1, &VBO); VBO = 0; }
    if (ssbo != 0) { glDeleteBuffers(1, &ssbo); ssbo = 0; }
    if (shader != nullptr) { delete shader; shader = nullptr; }
}

void VoxelRenderer::setVoxels(const std::vector<Voxel>& voxels)
{
    voxelData.clear();
    voxelData.reserve(voxels.size());
    for (const auto& v : voxels)
    {
        GPUVoxel gv;
        gv.posAndSize = glm::vec4(glm::vec3(v.getPosition()), 0.0f);
        gv.color = v.getColor();
        voxelData.push_back(gv);
    }
    voxelDataDirty = true;
}

void VoxelRenderer::addVoxel(const Voxel& voxel)
{
    GPUVoxel gv;
    gv.posAndSize = glm::vec4(glm::vec3(voxel.getPosition()), 0.0f);
    gv.color = voxel.getColor();
    voxelData.push_back(gv);
    voxelDataDirty = true;
}

void VoxelRenderer::clearVoxels()
{
    voxelData.clear();
    voxelDataDirty = true;
}
