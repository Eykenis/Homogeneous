#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

// Camera uniforms
uniform vec3 u_cameraPos;
uniform vec3 u_cameraTarget;
uniform float u_fov;
uniform vec2 u_resolution;
uniform bool u_shadow;
uniform bool u_useVoxelColor;
uniform int u_aoSampleCount;

// Octree bounds (from CPU)
uniform vec3 u_octreeMin;
uniform vec3 u_octreeMax;
const int MAX_AO_SAMPLES = 16;

// ── SSBO binding 0: legacy flat voxel list (kept for compatibility) ──────────
struct VoxelData { vec4 posAndSize; vec4 color; };
layout(std430, binding = 0) buffer VoxelBuffer
{
    int voxelCount;
    int _pad0; int _pad1; int _pad2;
    VoxelData voxels[];
};

// ── SSBO binding 1: compact octree ──────────────────────────────────────────
// GPUNode layout (matches C++ struct):
//   childMask : uint  - high 24 bits = index of first child in nodes[]
//                       low   8 bits = child existence bitmask (bit i = child i exists)
//   packedColor : uint  - packed RGBA8 (R<<24 | G<<16 | B<<8 | A)
struct OctreeNode { uint childMask; uint packedColor; };
layout(std430, binding = 1) buffer OctreeBuffer
{
    int nodeCount;
    int _opad0; int _opad1; int _opad2;
    OctreeNode nodes[];
};

// ── Constants ───────────────────────────────────────────────────────────────
const int   MAX_DEPTH = 16;
const float EPSILON = 1e-6;

// ── Utility: unpack RGBA8 color ─────────────────────────────────────────────
#define UNPACK_RGBA(packed) unpackUnorm4x8(packed).abgr

// ── Ray-AABB intersection ───────────────────────────────────────────────────
// Returns (tMin, tMax). If tMin > tMax, no intersection.
vec2 rayAABB(vec3 ro, vec3 rd, vec3 bmin, vec3 bmax) {
    vec3 invDir = 1.0 / (rd + vec3(EPSILON));
    vec3 t0 = (bmin - ro) * invDir;
    vec3 t1 = (bmax - ro) * invDir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float tNear = max(max(tmin.x, tmin.y), tmin.z);
    float tFar  = min(min(tmax.x, tmax.y), tmax.z);
    return vec2(tNear, tFar);
}

// ── Octree traversal stack entry ────────────────────────────────────────────
struct StackEntry {
    uint nodeIdx;
    vec3 bmin;
    vec3 bmax;
    float tEnter;
    float tExit;
};

// ── Octree ray casting ──────────────────────────────────────────────────────
// Returns hit color (rgb) and distance (a). If no hit, a = -1.
// hitNormal is set to the entry face normal on hit.
vec4 traceOctree(vec3 ro, vec3 rd, out vec3 hitNormal) {
    hitNormal = vec3(0.0);
    if (nodeCount <= 0) return vec4(0.0, 0.0, 0.0, -1.0);

    // Check root AABB
    vec3 rootMin = u_octreeMin;
    vec3 rootMax = u_octreeMax;
    vec2 tRoot = rayAABB(ro, rd, rootMin, rootMax);
    if (tRoot.x > tRoot.y || tRoot.y < 0.0) {
        return vec4(0.0, 0.0, 0.0, -1.0); // miss
    }

    // Manual stack for DFS traversal
    StackEntry stack[64];
    int sp = 0;

    // Push root
    stack[sp++] = StackEntry(0u, rootMin, rootMax, max(tRoot.x, 0.0), tRoot.y);

    vec4 result = vec4(0.0, 0.0, 0.0, -1.0);
    float closestT = 1e10;

    while (sp > 0) {
        StackEntry entry = stack[--sp];

        // Skip if this node is farther than current closest hit
        if (entry.tEnter > closestT) continue;

        OctreeNode node = nodes[entry.nodeIdx];
        uint existMask = node.childMask & 0xFFu;

        // Leaf node: childMask == 0
        if (existMask == 0u) {
            // Hit this leaf
            if (entry.tEnter < closestT) {
                closestT = entry.tEnter;
                result = vec4(UNPACK_RGBA(node.packedColor).rgb, entry.tEnter);

                // Determine entry face from leaf AABB slab test
                vec3 invDir = 1.0 / rd;
                vec3 t0 = (entry.bmin - ro) * invDir;
                vec3 t1 = (entry.bmax - ro) * invDir;
                vec3 tNearV = min(t0, t1);
                // The axis with the largest tNear is the entry face
                if (tNearV.x > tNearV.y && tNearV.x > tNearV.z)
                    hitNormal = vec3(-sign(rd.x), 0.0, 0.0);
                else if (tNearV.y > tNearV.z)
                    hitNormal = vec3(0.0, -sign(rd.y), 0.0);
                else
                    hitNormal = vec3(0.0, 0.0, -sign(rd.z));
            }
            continue;
        }

        // Internal node: traverse children
        uint firstChildIdx = node.childMask >> 8u;
        vec3 center = (entry.bmin + entry.bmax) * 0.5;

        // Determine ray entry octant to prioritize front-to-back
        vec3 entryPoint = ro + rd * entry.tEnter;
        int entryOctant = (entryPoint.x > center.x ? 1 : 0)
                        | (entryPoint.y > center.y ? 2 : 0)
                        | (entryPoint.z > center.z ? 4 : 0);

        // Visit children in back-to-front push order so closest ends up on stack top (DFS front-to-back)
        for (int i = 7; i >= 0; i--) {
            // Compute child index with XOR to reverse order based on entry octant
            int childIdx = i ^ entryOctant;
            if ((existMask & (1u << childIdx)) == 0u) continue;

            // Compute child AABB
            vec3 cmin = entry.bmin;
            vec3 cmax = entry.bmax;
            if ((childIdx & 1) != 0) { cmin.x = center.x; } else { cmax.x = center.x; }
            if ((childIdx & 2) != 0) { cmin.y = center.y; } else { cmax.y = center.y; }
            if ((childIdx & 4) != 0) { cmin.z = center.z; } else { cmax.z = center.z; }

            vec2 tChild = rayAABB(ro, rd, cmin, cmax);
            if (tChild.x <= tChild.y && tChild.y >= 0.0 && tChild.x < closestT) {
                // Count existing children before this one to find its index in nodes[]
                uint childOffset = 0u;
                for (int j = 0; j < childIdx; j++) {
                    if ((existMask & (1u << j)) != 0u) childOffset++;
                }
                uint childNodeIdx = firstChildIdx + childOffset;

                // Push to stack (DFS)
                if (sp < 64) {
                    stack[sp++] = StackEntry(childNodeIdx, cmin, cmax, max(tChild.x, 0.0), tChild.y);
                }
            }
        }
    }

    return result;
}

bool traceShadow(vec3 ro, vec3 rd) {
    if (nodeCount <= 0) return false;
    vec3 rootMin = u_octreeMin;
    vec3 rootMax = u_octreeMax;
    vec2 tRoot = rayAABB(ro, rd, rootMin, rootMax);
    if (tRoot.x > tRoot.y || tRoot.y < 0.0) {
        return false;
    }

    StackEntry stack[64];
    int sp = 0;
    stack[sp++] = StackEntry(0u, rootMin, rootMax, max(tRoot.x, 0.0), tRoot.y);

    while (sp > 0) {
        StackEntry entry = stack[--sp];
        OctreeNode node = nodes[entry.nodeIdx];
        uint existMask = node.childMask & 0xFFu;
        if (existMask == 0u) { // leaf
            return true;
        }
        uint firstChildIdx = node.childMask >> 8u;
        vec3 center = (entry.bmin + entry.bmax) * 0.5;
        vec3 entryPoint = ro + rd * entry.tEnter;
        int entryOctant = (entryPoint.x > center.x ? 1 : 0)
                        | (entryPoint.y > center.y ? 2 : 0)
                        | (entryPoint.z > center.z ? 4 : 0);
        for (int i = 7; i >= 0; i--) {
            int childIdx = i ^ entryOctant;
            if ((existMask & (1u << childIdx)) == 0u) continue;
            vec3 cmin = entry.bmin;
            vec3 cmax = entry.bmax;
            if ((childIdx & 1) != 0) { cmin.x = center.x; } else { cmax.x = center.x; }
            if ((childIdx & 2) != 0) { cmin.y = center.y; } else { cmax.y = center.y; }
            if ((childIdx & 4) != 0) { cmin.z = center.z; } else { cmax.z = center.z; }
            vec2 tChild = rayAABB(ro, rd, cmin, cmax);
            if (tChild.x <= tChild.y && tChild.y >= 0.0) {
                uint childOffset = 0u;
                for (int j = 0; j < childIdx; j++) {
                    if ((existMask & (1u << j)) != 0u) childOffset++;
                }
                uint childNodeIdx = firstChildIdx + childOffset;
                if (sp < 64) {
                    stack[sp++] = StackEntry(childNodeIdx, cmin, cmax, max(tChild.x, 0.0), tChild.y);
                }
            }
        }
    }
    return false;
}

float ao(vec3 pos, vec3 norm) {
    float occ = 0.0;
    vec3 origin = pos + norm * 0.02;
    float seed = dot(floor(pos), vec3(127.1, 311.7, 74.7));
    float sca = 1.0;
    for (int i = 0; i < MAX_AO_SAMPLES; i++) {
        if (i >= u_aoSampleCount) break;
        float fi = float(i) + seed;
        vec3 dir = normalize(vec3(
            fract(sin(fi * 12.9898) * 43758.5453) * 2.0 - 1.0,
            fract(sin(fi * 78.233)  * 43758.5453) * 2.0 - 1.0,
            fract(sin(fi * 45.164)  * 43758.5453) * 2.0 - 1.0
        ));
        if (dot(dir, norm) < 0.0) dir = -dir;
        vec3 dummy;
        vec4 hit = traceOctree(origin, dir, dummy);
        if (hit.a >= 0.0 && hit.a < 4.0) {
            occ += sca;
        }
        sca *= 0.5;
    }
    return clamp(occ, 0.0, 1.0);
}

// ── Camera ray ──────────────────────────────────────────────────────────────
vec3 getCameraRay(vec2 uv, vec3 camPos, vec3 camTarget, float fov) {
    vec3 forward = normalize(camTarget - camPos);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));
    vec3 up = cross(forward, right);

    float aspect = u_resolution.x / u_resolution.y;
    float fovRad = radians(fov);
    vec2 ndc = uv * 2.0 - 1.0;
    ndc.x *= aspect;

    return normalize(forward + ndc.x * right * tan(fovRad * 0.5) + ndc.y * up * tan(fovRad * 0.5));
}

// ── Main ────────────────────────────────────────────────────────────────────
void main() {
    vec2 uv = TexCoord;
    vec3 ro = u_cameraPos;
    vec3 rd = getCameraRay(uv, u_cameraPos, u_cameraTarget, u_fov);

    vec3 normal;
    vec4 hit = traceOctree(ro, rd, normal);

    // Background gradient
    vec3 color = mix(vec3(0.15, 0.2, 0.35), vec3(0.4, 0.5, 0.6), uv.y);

    if (hit.a >= 0.0) {
        vec3 lightDir = normalize(vec3(1.0, 1.0, -1.0));
        float diff = max(dot(normal, lightDir), 0.0);
        float ambient = 0.3;
        color = (u_useVoxelColor ? hit.rgb : vec3(1.0)) * (ambient + diff * 0.7);
        float vao = ao(ro + rd * hit.a, normal);
        color = mix(color, color * 0.5, vao);
        // hard shadow
        vec3 origin = ro + rd * hit.a;
        if (u_shadow && traceShadow(origin + lightDir * 0.02, lightDir)) {
            color *= 0.2;
        }
    }

    FragColor = vec4(color, 1.0);
}
