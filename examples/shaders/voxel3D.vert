#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_ARB_shader_draw_parameters : require

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;

float hash(vec3 p) {
  p = fract(p * 0.3183099 + 0.1);
  p *= 17.0;
  return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

struct AABB { vec4 min; vec4 max; };
layout(buffer_reference, std430) readonly buffer ChunkAABBs { AABB aabbs[]; };
layout(buffer_reference, std430) readonly buffer VoxelData { uint voxels[]; };

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    uint64_t aabbAddress;
    uint64_t voxelDataAddress;
} pc;

const vec3 CUBE_VERTS[8] = vec3[](
    vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 1, 0), vec3(0, 1, 0),
    vec3(0, 0, 1), vec3(1, 0, 1), vec3(1, 1, 1), vec3(0, 1, 1)
);

// Standardized CW indices for all 6 faces to match the backend's Y-flip.
const int CUBE_INDICES[36] = int[](
    2,1,0, 0,3,2, // Back (-Z)
    4,5,6, 6,7,4, // Front (+Z)
    1,5,4, 4,0,1, // Bottom (-Y)
    3,7,6, 6,2,3, // Top (+Y)
    0,4,7, 7,3,0, // Left (-X)
    5,1,2, 2,6,5  // Right (+X)
);

const vec3 CUBE_NORMALS[6] = vec3[](
    vec3(0,0,-1), vec3(0,0,1), vec3(0,-1,0), vec3(0,1,0), vec3(-1,0,0), vec3(1,0,0)
);

void main() {
    uint chunkID = gl_InstanceIndex / 65536;
    uint slotInstanceIdx = gl_InstanceIndex % 65536;
    int vertId = gl_VertexIndex % 36;
    int faceId = vertId / 6;

    VoxelData instanceBuffer = VoxelData(pc.voxelDataAddress);
    uint voxelID = instanceBuffer.voxels[chunkID * 65536 + slotInstanceIdx];

    // Decode voxelID to 3D grid coordinates
    int x = int(voxelID & 63u);
    int y = int((voxelID >> 6u) & 63u);
    int z = int(voxelID >> 12u);

    // Procedural color based on height within the chunk to look like terrain.
    // Higher voxels are greener (grass), lower are browner (dirt/rock).
    float h = float(y) / 63.0;
    vec3 color = mix(vec3(0.4, 0.3, 0.2), vec3(0.3, 0.6, 0.2), h);
    
    // Add a bit of variation based on local XZ to reduce tiling look.
    color += (hash(vec3(x, 0, z)) - 0.5) * 0.1;

    ChunkAABBs aabbBuffer = ChunkAABBs(pc.aabbAddress);
    vec3 chunkOrigin = aabbBuffer.aabbs[chunkID].min.xyz;
    
    // SCALE: 4x4x4 per voxel for visibility.
    vec3 worldPos = chunkOrigin + (vec3(x, y, z) + CUBE_VERTS[CUBE_INDICES[vertId]]) * 4.0;

    outColor = color;
    outNormal = CUBE_NORMALS[faceId];
    gl_Position = pc.viewProj * vec4(worldPos, 1.0);
}
