# Voxel Rendering Architecture

The system utilizes a "Pure Procedural" GPU-driven pipeline to render massive voxel environments with minimal CPU overhead. By leveraging Vulkan 1.3 features such as Multi-Draw Indirect (MDI) and Buffer Device Address (BDA), the engine can synthesize geometry mathematically on the GPU rather than streaming it from host memory.

## Procedural Geometry Synthesis

Traditional rendering requires the CPU to upload vertex and index data for every mesh. The voxel system eliminates this bandwidth bottleneck by synthesizing the cube topology directly in the Vertex Shader using the `gl_VertexIndex` built-in variable.

### Geometry Generation Logic
- **Cube Topology**: A lookup table (LUT) within the shader defines the 8 corners and 36 indices of a unit cube.
- **Index Decoding**: The shader uses `gl_VertexIndex % 36` to retrieve the current vertex's local coordinates.
- **Grid Translation**: The shader uses `gl_VertexIndex / 36` to determine the block's $(x, z)$ position within a $32 \times 32$ chunk.

## Data-Oriented Metadata Passthrough

To maintain performance, the system avoids frequent descriptor set updates. Instead, it utilizes the Instance Storage Buffer as a metadata channel.

- **Chunk Offsets**: The current camera chunk coordinates $(X, Z)$ are embedded within the `transform` field of the first instance entry.
- **Vertex Retrieval**: The shader uses a 64-bit GPU virtual address (passed via Push Constants) to read this metadata and correctly position the terrain chunks in world space.

## Performance Characteristics

| Component | Traditional Method | SBgl Voxel Method |
| :--- | :--- | :--- |
| **CPU Submission** | ~100,000 Draw Calls | **Exactly 1 MDI Call** |
| **Geometry Bandwidth** | ~4.0 MB / frame | **Zero (Shader-Synthesized)** |
| **Memory Allocation** | Per-Chunk Buffers | **Zero (Persistent Unit Cube)** |

## Procedural Shading

Surface normals are approximated in the shader by analyzing the local vertex position relative to the cube center. This allows for dynamic coloring (e.g., Grass tops vs. Dirt sides) without storing normal attributes in memory.

### Pillar Optimization
To resolve visual gaps (blue holes) on steep 2.5D terrain slopes, the shader implements a "Pillar" logic. Vertices at the base of a block are mathematically extended downwards into the ground, ensuring a solid visual surface regardless of terrain verticality.
