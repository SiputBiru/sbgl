/**
 * @file voxel3D.frag
 * @brief Simple Lambertian shading for 3D voxel rendering.
 *
 * This fragment shader calculates surface lighting using a basic directional 
 * light model to provide depth and visual clarity to the voxel geometry.
 */
#version 450

layout(location = 0) in vec3 inColor;   /**< Unpacked RGB color from the vertex stage. */
layout(location = 1) in vec3 inNormal;  /**< Surface normal vector. */

layout(location = 0) out vec4 outColor; /**< Final rendered pixel color. */

void main() {
    // Basic directional lighting to emphasize the 3D structure of the voxels.
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(inNormal, lightDir), 0.0);
    
    // Reduce ambient to 0.2 and increase diffuse contribution to 0.8
    outColor = vec4(inColor * (diff * 0.8 + 0.2), 1.0);
}
