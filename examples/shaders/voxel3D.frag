/**
 * @file voxel3D.frag
 * @brief Simple Lambertian shading for 3D voxel rendering.
 *
 * This fragment shader calculates surface lighting using a basic directional 
 * light model to provide depth and visual clarity to the voxel geometry.
 */
#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) flat in uint inVoxelID;
layout(location = 2) flat in vec3 inChunkOrigin;

layout(location = 0) out vec4 outFragColor;

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

    // Procedural color based on height within the chunk to look like terrain.
    // Higher voxels are greener (grass), lower are browner (dirt/rock).
    float h = float(y) / 63.0;
    vec3 color = mix(vec3(0.3, 0.6, 0.2), vec3(0.4, 0.3, 0.2), h);
    
    // Add a bit of variation based on local XZ to reduce tiling look.
    // Using chunkOrigin ensures the variation is consistent across the chunk.
    color += (hash(vec3(x, 0, z) + inChunkOrigin * 0.1) - 0.5) * 0.1;

    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(inNormal, lightDir), 0.2);
    outFragColor = vec4(color * diff, 1.0);
}
