#include "voxel_renderer.h"
#include <iostream>
#include <cstring>
#include <memory>
#include <queue>
constexpr int MAX_DEPTH = 16;

bool allPointsSameColor(const std::vector<Voxel>& points) {
    if (points.empty()) return true;
    glm::vec4 firstColor = points[0].getColor();
    for (const auto& p : points) {
        if (p.getColor() != firstColor) {
            return false;
        }
    }
    return true;
}

// build tree in [min, max]
std::shared_ptr<OctreeNode> buildOctree(const std::vector<Voxel>& points, glm::vec3 min, glm::vec3 max, int depth) {
    if (points.empty()) return nullptr;

    auto node = std::make_shared<OctreeNode>();

    // leaf: create leaf when node size reaches voxel resolution (1x1x1) or at max depth
    float nodeSize = max.x - min.x;
    if (depth >= MAX_DEPTH || nodeSize <= 1.0f) {
        node->leaf = true;
        node->color = points[0].getColor();
        return node;
    }

    glm::vec3 center = (min + max) * 0.5f;
    // distribute points to 8 sub-cubes
    std::vector<Voxel> subPoints[8];
    for (const auto& p : points) {
        glm::vec3 pos = glm::vec3(p.getPosition());
        int idx = (pos.x >= center.x ? 1 : 0) |
                  (pos.y >= center.y ? 2 : 0) |
                  (pos.z >= center.z ? 4 : 0);
        subPoints[idx].push_back(p);
    }

    for (int i = 0; i < 8; ++i) {
        glm::vec3 subMin = min, subMax = max;

        subMin.x = (i & 1) ? center.x : min.x;
        subMax.x = (i & 1) ? max.x : center.x;
        subMin.y = (i & 2) ? center.y : min.y;
        subMax.y = (i & 2) ? max.y : center.y;
        subMin.z = (i & 4) ? center.z : min.z;
        subMax.z = (i & 4) ? max.z : center.z;

        node->children[i] = buildOctree(subPoints[i], subMin, subMax, depth + 1);
        if (node->children[i]) {
            node->childMask |= (1 << i);
        }
    }
    return node;
}

void VoxelRenderer::buildOctreeFromVoxels(const std::vector<Voxel>& points) {
    if (points.empty()) return;

    // Compute per-axis bounding box
    glm::vec3 bmin(1e9f), bmax(-1e9f);
    for (const auto& v : points) {
        glm::vec3 pos = glm::vec3(v.getPosition());
        bmin = glm::min(bmin, pos);
        bmax = glm::max(bmax, pos);
    }

    // Each voxel at integer position P occupies [P, P+1), so the octree must cover
    // [bmin, bmax+1). Round the extent up to a power of two and keep min integer-aligned
    // so that every subdivision boundary falls on an integer — matching voxel grid coords.
    float extent = glm::max(glm::max(bmax.x - bmin.x + 1.0f, bmax.y - bmin.y + 1.0f), bmax.z - bmin.z + 1.0f);
    float pot = 1.0f;
    while (pot < extent) pot *= 2.0f;

    octreeBoundsMin = glm::floor(bmin);
    octreeBoundsMax = octreeBoundsMin + glm::vec3(pot);

    std::cout << "Build info: " << points.size() << " voxels, octree bounds ["
              << octreeBoundsMin.x << ", " << octreeBoundsMax.x << "]" << std::endl;

    auto octreeRoot = buildOctree(points, octreeBoundsMin, octreeBoundsMax, 0);
    if (!octreeRoot) return;

    octreeData.clear();

    // Helper function to pack color into uint32_t (RGBA8)
    auto packColor = [](const glm::vec4& color) -> uint32_t {
        uint8_t r = static_cast<uint8_t>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f);
        uint8_t g = static_cast<uint8_t>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f);
        uint8_t b = static_cast<uint8_t>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f);
        uint8_t a = static_cast<uint8_t>(glm::clamp(color.a, 0.0f, 1.0f) * 255.0f);
        return (r << 24) | (g << 16) | (b << 8) | a;
    };

    // BFS: collect nodes in level-order, track each node's index in octreeData
    // and which octreeData slot needs its childMask updated with the first child index.
    struct QueueEntry {
        std::shared_ptr<OctreeNode> node;
        uint32_t selfIdx; // index in octreeData for this node
    };

    std::queue<QueueEntry> queue;

    // Allocate root slot
    octreeData.push_back(GPUNode{0, 0});
    QueueEntry rootEntry;
    rootEntry.node = octreeRoot;
    rootEntry.selfIdx = 0;
    queue.push(rootEntry);

    while (!queue.empty()) {
        QueueEntry entry = queue.front();
        queue.pop();

        octreeData[entry.selfIdx].color = packColor(entry.node->color);

        if (entry.node->leaf) {
            // Leaf: childMask stays 0
            octreeData[entry.selfIdx].childMask = 0;
        } else {
            // Allocate contiguous slots for all existing children
            uint32_t firstChildIdx = static_cast<uint32_t>(octreeData.size());

            for (int i = 0; i < 8; ++i) {
                if (entry.node->children[i]) {
                    uint32_t childSlot = static_cast<uint32_t>(octreeData.size());
                    octreeData.push_back(GPUNode{0, 0});

                    QueueEntry childEntry;
                    childEntry.node = entry.node->children[i];
                    childEntry.selfIdx = childSlot;
                    queue.push(childEntry);
                }
            }

            // Encode: upper 24 bits = first child index, lower 8 bits = existence mask
            octreeData[entry.selfIdx].childMask = (firstChildIdx << 8) | static_cast<uint32_t>(entry.node->childMask);
        }
    }
}

VoxelRenderer::VoxelRenderer()
    : shader(nullptr)
    , VAO(0)
    , VBO(0)
    , ssbo(0)
    , octreeSSBO(0)
    , cameraPos(0.0f, 0.0f, 5.0f)
    , cameraTarget(0.0f, 0.0f, 0.0f)
    , fov(45.0f)
    , voxelDataDirty(true)
    , octreeDataDirty(false)
    , shadow(true)
    , aoSampleCount(4)
    , useVoxelColor(true)
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

    // Create SSBO for voxel data (binding point 0)
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Create SSBO for octree data (binding point 1)
    glGenBuffers(1, &octreeSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, octreeSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, octreeSSBO);
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

void VoxelRenderer::uploadOctreeData()
{
    if (!octreeDataDirty) return;
    if (octreeData.empty()) return;

    int count = static_cast<int>(octreeData.size());
    size_t headerSize = sizeof(int) * 4;
    size_t dataSize = headerSize + octreeData.size() * sizeof(GPUNode);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, octreeSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dataSize, nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &count);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, headerSize, octreeData.size() * sizeof(GPUNode), octreeData.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    octreeDataDirty = false;
}

void VoxelRenderer::render(int width, int height)
{
    uploadVoxelData();
    uploadOctreeData();

    shader->use();
    shader->setVec3("u_cameraPos", cameraPos);
    shader->setVec3("u_cameraTarget", cameraTarget);
    shader->setFloat("u_fov", fov);
    shader->setVec2("u_resolution", glm::vec2(width, height));
    shader->setFloat("u_voxelSize", 1.0f);
    shader->setVec3("u_octreeMin", octreeBoundsMin);
    shader->setVec3("u_octreeMax", octreeBoundsMax);
    shader->setBool("u_shadow", shadow);
    shader->setInt("u_aoSampleCount", aoSampleCount);
    shader->setBool("u_useVoxelColor", useVoxelColor);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, octreeSSBO);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void VoxelRenderer::cleanup()
{
    if (VAO != 0) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    if (VBO != 0) { glDeleteBuffers(1, &VBO); VBO = 0; }
    if (ssbo != 0) { glDeleteBuffers(1, &ssbo); ssbo = 0; }
    if (octreeSSBO != 0) { glDeleteBuffers(1, &octreeSSBO); octreeSSBO = 0; }
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

    // Build octree from voxels
    buildOctreeFromVoxels(voxels);
    octreeDataDirty = true;
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
