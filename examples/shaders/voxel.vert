#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_ARB_shader_draw_parameters : require

layout(location = 0) out vec3 outColor;

struct InstanceData {
    mat4 transform;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

layout(buffer_reference, std430) readonly buffer HeightBuffer {
    float heights[];
};

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    uint64_t instanceAddress;
    uint64_t heightAddress;
} pc;

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

    // IMPORTANT: In a batched environment, gl_InstanceIndex is absolute.
    // We must subtract gl_BaseInstanceARB to get the 0-based ID within our chunk batch.
    int relativeInstance = gl_InstanceIndex - gl_BaseInstanceARB;

    // Use relativeInstance to identify the chunk in the 11x11 grid.
    const int RADIUS = 5;
    const int GRID_WIDTH = (RADIUS * 2 + 1);
    int chunkXOffset = (relativeInstance % GRID_WIDTH) - RADIUS;
    int chunkZOffset = (relativeInstance / GRID_WIDTH) - RADIUS;

    // Retrieve camera chunk metadata from the start of our specific batch.
    InstanceBuffer instanceData = InstanceBuffer(pc.instanceAddress);
    InstanceData metadata = instanceData.instances[gl_BaseInstanceARB];
    int camX = int(round(metadata.transform[0][0]));
    int camZ = int(round(metadata.transform[0][1]));

    // Calculate integer world coordinates for lookup.
    int worldIX = localX + (camX + chunkXOffset) * 32;
    int worldIZ = localZ + (camZ + chunkZOffset) * 32;
    
    // Heightmap lookup (Robust wrapping for 2048x2048)
    // We use a double modulo to handle negative coordinates correctly.
    HeightBuffer heightmap = HeightBuffer(pc.heightAddress);
    int hX = ((worldIX % 2048) + 2048) % 2048;
    int hZ = ((worldIZ % 2048) + 2048) % 2048;
    float worldY = heightmap.heights[hZ * 2048 + hX];

    // Floating point world coordinates for positioning.
    float worldX = float(worldIX);
    float worldZ = float(worldIZ);

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
    if (localPos.y < -0.4) localPos.y -= 1.0;

    gl_Position = pc.viewProj * vec4(localPos.x + worldX, localPos.y + worldY, localPos.z + worldZ, 1.0);
    outColor = color;
}
