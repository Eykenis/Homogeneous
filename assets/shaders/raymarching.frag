#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

// Camera uniforms
uniform vec3 u_cameraPos;
uniform vec3 u_cameraTarget;
uniform float u_fov;
uniform vec2 u_resolution;
uniform float u_voxelSize;

// Voxel data via SSBO
struct VoxelData
{
    vec4 posAndSize;  // xyz = position, w = unused
    vec4 color;       // rgba
};

// TODO: Change voxel data like below
struct VoxelData
{
    uint header; // [0~7] gridlevel, [8~31] child position bitmask
};

layout(std430, binding = 0) buffer VoxelBuffer
{
    int voxelCount;
    int _pad0;
    int _pad1;
    int _pad2;
    VoxelData voxels[];
};

// Ray marching parameters
const int MAX_STEPS = 64;
const float MAX_DIST = 100.0;
const float EPSILON = 0.001;

// SDF for a box
float sdBox(vec3 p, vec3 b)
{
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

// Scene SDF - union of all voxels, also returns hit voxel index
float sceneSDF(vec3 p, out int hitIndex)
{
    float minDist = MAX_DIST;
    hitIndex = -1;
    vec3 halfSize = vec3(u_voxelSize * 0.5);

    for (int i = 0; i < voxelCount; i++)
    {
        vec3 voxelPos = voxels[i].posAndSize.xyz;
        float d = sdBox(p - voxelPos, halfSize);
        if (d < minDist)
        {
            minDist = d;
            hitIndex = i;
        }
    }

    return minDist;
}

// Overload without hit index for normal calculation
float sceneSDF(vec3 p)
{
    int dummy;
    return sceneSDF(p, dummy);
}

// Calculate normal using gradient
vec3 calcNormal(vec3 p)
{
    vec2 e = vec2(EPSILON, 0.0);
    return normalize(vec3(
        sceneSDF(p + e.xyy) - sceneSDF(p - e.xyy),
        sceneSDF(p + e.yxy) - sceneSDF(p - e.yxy),
        sceneSDF(p + e.yyx) - sceneSDF(p - e.yyx)
    ));
}

// Ray marching - returns distance and hit voxel index
float rayMarch(vec3 ro, vec3 rd, out int hitIndex)
{
    float t = 0.0;
    hitIndex = -1;

    for (int i = 0; i < MAX_STEPS; i++)
    {
        vec3 p = ro + rd * t;
        int idx;
        float d = sceneSDF(p, idx);

        if (d < EPSILON)
        {
            hitIndex = idx;
            return t;
        }

        t += d;

        if (t > MAX_DIST)
            break;
    }

    return -1.0;
}

// Create camera ray
vec3 getCameraRay(vec2 uv, vec3 camPos, vec3 camTarget, float fov)
{
    vec3 forward = normalize(camTarget - camPos);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));
    vec3 up = cross(forward, right);

    float aspect = u_resolution.x / u_resolution.y;
    float fovRad = radians(fov);
    vec2 ndc = uv * 2.0 - 1.0;
    ndc.x *= aspect;

    vec3 rd = normalize(forward + ndc.x * right * tan(fovRad * 0.5) + ndc.y * up * tan(fovRad * 0.5));
    return rd;
}

void main()
{
    vec2 uv = TexCoord;

    vec3 ro = u_cameraPos;
    vec3 rd = getCameraRay(uv, u_cameraPos, u_cameraTarget, u_fov);

    int hitIndex;
    float t = rayMarch(ro, rd, hitIndex);

    // Background gradient
    vec3 color = mix(vec3(0.15, 0.2, 0.35), vec3(0.4, 0.5, 0.6), uv.y);

    if (t > 0.0 && hitIndex >= 0)
    {
        vec3 p = ro + rd * t;
        vec3 normal = calcNormal(p);

        // Lighting
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        float diff = max(dot(normal, lightDir), 0.0);
        float ambient = 0.3;

        vec3 voxelColor = voxels[hitIndex].color.rgb;
        color = voxelColor * (ambient + diff * 0.7);
    }

    FragColor = vec4(color, 1.0);
}
