#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_ARB_shader_draw_parameters : require

layout(location = 0) out vec3 outNormal;
layout(location = 1) flat out uint outVoxelID;
layout(location = 2) flat out vec3 outChunkOrigin;

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

    ChunkAABBs aabbBuffer = ChunkAABBs(pc.aabbAddress);
    vec3 chunkOrigin = aabbBuffer.aabbs[chunkID].min.xyz;
    
    // Decode voxelID to 3D grid coordinates
    int x = int(voxelID & 63u);
    int y = int((voxelID >> 6u) & 63u);
    int z = int(voxelID >> 12u);

    // Pass data to fragment shader
    outVoxelID = voxelID;
    outChunkOrigin = chunkOrigin;
    outNormal = CUBE_NORMALS[faceId];
    
    // SCALE: 4x4x4 per voxel for visibility.
    vec3 worldPos = chunkOrigin + (vec3(x, y, z) + CUBE_VERTS[CUBE_INDICES[vertId]]) * 4.0;
    gl_Position = pc.viewProj * vec4(worldPos, 1.0);
}
