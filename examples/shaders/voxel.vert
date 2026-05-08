#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(location = 0) out vec3 outColor;

struct InstanceData {
    mat4 transform;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    uint64_t instanceAddress;
} pc;

float get_height(float x, float z) {
    return floor((sin(x * 0.1) + cos(z * 0.1) + sin(x * 0.05) * 1.5) * 6.0);
}

// Cube vertex lookup table (36 indices)
const vec3 CUBE_VERTS[8] = vec3[](
    vec3(-0.5, -0.5, -0.5), vec3( 0.5, -0.5, -0.5), vec3( 0.5,  0.5, -0.5), vec3(-0.5,  0.5, -0.5),
    vec3(-0.5, -0.5,  0.5), vec3( 0.5, -0.5,  0.5), vec3( 0.5,  0.5,  0.5), vec3(-0.5,  0.5,  0.5)
);

const int CUBE_INDICES[36] = int[](
    0,3,2, 2,1,0, // Back
    4,5,6, 6,7,4, // Front
    0,4,7, 7,3,0, // Left
    1,2,6, 6,5,1, // Right
    3,7,6, 6,2,3, // Top
    0,1,5, 5,4,0  // Bottom
);

void main() {
    // Determine which cube and which vertex we are in this chunk.
    int cubeId = gl_VertexIndex / 36;
    int vertId = gl_VertexIndex % 36;

    // Local coordinates in the 32x32 chunk.
    int localX = cubeId % 32;
    int localZ = cubeId / 32;

    // Use gl_InstanceIndex to identify the chunk in the 11x11 grid.
    const int RADIUS = 5;
    const int GRID_WIDTH = (RADIUS * 2 + 1);
    int chunkXOffset = (gl_InstanceIndex % GRID_WIDTH) - RADIUS;
    int chunkZOffset = (gl_InstanceIndex / GRID_WIDTH) - RADIUS;

    // Retrieve camera chunk metadata from the first instance in the buffer.
    // We store camChunkX and camChunkZ in the first row of the transform matrix.
    InstanceBuffer instanceData = InstanceBuffer(pc.instanceAddress);
    InstanceData metadata = instanceData.instances[0];
    int camX = int(metadata.transform[0][0]);
    int camZ = int(metadata.transform[0][1]);

    // Final world coordinates.
    float worldX = float(localX + (camX + chunkXOffset) * 32);
    float worldZ = float(localZ + (camZ + chunkZOffset) * 32);
    float worldY = get_height(worldX, worldZ);

    // Cube geometry generation.
    vec3 localPos = CUBE_VERTS[CUBE_INDICES[vertId]];
    
    // Shading.
    vec3 color;
    if (localPos.y > 0.4) {
        color = vec3(0.4, 1.0, 0.4); // Top
    } else {
        color = vec3(0.6, 0.4, 0.3); // Sides
        if (abs(localPos.x) > 0.4 || abs(localPos.z) > 0.4) color *= 0.8;
    }

    // Pillar optimization.
    if (localPos.y < -0.4) localPos.y -= 10.0;

    gl_Position = pc.viewProj * vec4(localPos.x + worldX, localPos.y + worldY, localPos.z + worldZ, 1.0);
    outColor = color;
}
