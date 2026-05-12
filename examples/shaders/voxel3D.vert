#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_ARB_shader_draw_parameters : require

layout(location = 0) out vec3 outNormal;
layout(location = 1) flat out uint outVoxelID;
layout(location = 2) flat out vec3 outChunkOrigin;
layout(location = 3) flat out uint outMaterialID;
layout(location = 4) out float outAO;

struct AABB { vec4 min; vec4 max; };
layout(buffer_reference, std430) readonly buffer ChunkAABBs { AABB aabbs[]; };
layout(buffer_reference, std430) readonly buffer VoxelData { uvec2 voxels[]; };

layout(push_constant) uniform PushConstants {
  mat4 viewProj;
  uint64_t aabbAddress;
  uint64_t voxelDataAddress;
  uint64_t paletteAddress;
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
  uvec2 instanceData = instanceBuffer.voxels[chunkID * 65536 + slotInstanceIdx];
  
  // The voxelID occupies the first 18 bits, representing a 64x64x64 grid.
  uint voxelID = instanceData.x & 0x3FFFFu;
  
  // The yScale is packed into bits 18-25, allowing for vertical strip merging.
  uint yScale = (instanceData.x >> 18u) & 0xFFu;

  // The materialID is packed into bits 26-31.
  uint materialID = (instanceData.x >> 26u) & 0x3Fu;

  // Unpack the AO value for the current corner.
  uint cornerIdx = CUBE_INDICES[vertId];
  outAO = float((instanceData.y >> (cornerIdx * 2u)) & 0x3u);

  ChunkAABBs aabbBuffer = ChunkAABBs(pc.aabbAddress);
  vec3 chunkOrigin = aabbBuffer.aabbs[chunkID].min.xyz;
  
  // Decode the voxelID into 3D grid coordinates within the chunk.
  int x = int(voxelID & 63u);
  int y = int((voxelID >> 6u) & 63u);
  int z = int(voxelID >> 12u);

  // Pass necessary data to the fragment shader for lighting and coloring.
  outVoxelID = voxelID;
  outChunkOrigin = chunkOrigin;
  outNormal = CUBE_NORMALS[faceId];
  outMaterialID = materialID;
  
  // Apply the vertical scaling factor to the local vertex position.
  // This stretches the unit cube into a vertical strip for binary meshing.
  vec3 localPos = CUBE_VERTS[CUBE_INDICES[vertId]];
  localPos.y *= float(yScale);

  // Calculate the world position using the chunk origin and voxel coordinates.
  // The scale of 4.0 is applied to each voxel unit for visual clarity.
  vec3 worldPos = chunkOrigin + (vec3(x, y, z) + localPos) * 4.0;
  gl_Position = pc.viewProj * vec4(worldPos, 1.0);
}
