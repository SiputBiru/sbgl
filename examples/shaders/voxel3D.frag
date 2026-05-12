/**
 * @file voxel3D.frag
 * @brief Simple Lambertian shading for 3D voxel rendering.
 *
 * This fragment shader calculates surface lighting using a basic directional 
 * light model to provide depth and visual clarity to the voxel geometry.
 */
#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(location = 0) in vec3 inNormal;
layout(location = 1) flat in uint inVoxelID;
layout(location = 2) flat in vec3 inChunkOrigin;
layout(location = 3) flat in uint inMaterialID;
layout(location = 4) in float inAO;

layout(location = 0) out vec4 outFragColor;

struct Material {
  vec4 color;
  float roughness;
  float metalness;
  uint textureID;
  uint _pad;
};

layout(buffer_reference, std430) readonly buffer MaterialPalette { Material materials[]; };

layout(push_constant) uniform PushConstants {
  mat4 viewProj;
  uint64_t aabbAddress;
  uint64_t voxelDataAddress;
  uint64_t paletteAddress;
} pc;

float hash(vec3 p) {
  p = fract(p * 0.3183099 + 0.1);
  p *= 17.0;
  return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

void main() {
    // Decode voxelID
    int x = int(inVoxelID & 63u);
    int y = int((inVoxelID >> 6u) & 63u);
    int z = int(inVoxelID >> 12u);

    // Retrieve material attributes from the GPU-side palette.
    MaterialPalette palette = MaterialPalette(pc.paletteAddress);
    vec3 color = palette.materials[inMaterialID].color.rgb;
    
    // Add a bit of variation based on local XZ to reduce tiling look.
    // Using chunkOrigin ensures the variation is consistent across the chunk.
    color += (hash(vec3(x, 0, z) + inChunkOrigin * 0.1) - 0.5) * 0.1;

    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(inNormal, lightDir), 0.2);
    
    // Apply Ambient Occlusion for soft corner shadows.
    // Each level of AO reduces the intensity by 20%, up to 60% for a full corner.
    float aoFactor = 1.0 - inAO * 0.2;
    outFragColor = vec4(color * diff * aoFactor, 1.0);
}
