# SBgl Rendering Pipeline

The SBgl Rendering Pipeline is built on Vulkan 1.3's Dynamic Rendering, eliminating the need for complex RenderPass and Framebuffer boilerplate. It follows Data-Oriented Design (DOD) principles by using lightweight integer handles for resource management.

## Key Concepts

### Handle-Based Resource Management

Resources like Buffers, Shaders, and Pipelines are referenced via `sbgl_Buffer`, `sbgl_Shader`, and `sbgl_Pipeline` handles (32-bit unsigned integers). Internally, these handles map to contiguous arrays in the Vulkan backend, ensuring cache-efficient access.

### Explicit Pipeline State Objects (PSO)

SBgl requires explicit creation of graphics pipelines. A pipeline encapsulates the vertex/fragment shaders and the vertex input layout. This design ensures predictable performance by moving pipeline compilation to initialization time.

### Vertex Input Layout

The `sbgl_VertexLayout` structure defines how vertex data in a buffer is mapped to shader input locations.

- **Stride:** Total size in bytes of a single vertex.
- **Attributes:** Array of `sbgl_VertexAttribute` defining the location and offset of each component.

## Shader Loading Strategies

SBgl supports two primary ways to load SPIR-V shaders: dynamic file loading and hardcoded byte arrays.

### Dynamic File Loading

Ideal for development and modding, allowing shaders to be recompiled without rebuilding the application.

```c
size_t size;
uint32_t* bytecode = read_spirv_file("examples/shaders/my_shader.vert.spv", &size);
sbgl_Shader shader = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_VERTEX, bytecode, size);
// ... handle cleanup of bytecode if necessary ...
```

### Hardcoded (Static) Loading

Recommended for production releases to ensure the application is self-contained and prevents external tampering with core assets.

You can use the `xxd` tool to convert a compiled `.spv` file into a C header:
`xxd -i examples/shaders/my_shader.vert.spv > my_shader_vert.h`

```c
#include "my_shader_vert.h"
// xxd generates an array named 'my_shader_vert_spv' and 'my_shader_vert_spv_len'
sbgl_Shader shader = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_VERTEX, 
                                     (uint32_t*)my_shader_vert_spv, 
                                     my_shader_vert_spv_len);
```

## Vertex Layout Use Case

Defining a standard vertex with Position and Color attributes:

```c
typedef struct {
    float pos[3];
    float color[3];
} MyVertex;

sbgl_VertexAttribute attributes[] = {
    { .location = 0, .offset = offsetof(MyVertex, pos), .format = SBGL_FORMAT_R32G32B32_SFLOAT },
    { .location = 1, .offset = offsetof(MyVertex, color), .format = SBGL_FORMAT_R32G32B32_SFLOAT }
};

sbgl_VertexLayout layout = {
    .stride = sizeof(MyVertex),
    .attributeCount = 2,
    .attributes = attributes
};
```

## Complete Drawing Example

```c
sbgl_BeginDrawing(ctx);
sbgl_Clear(ctx, 0.0f, 0.0f, 0.0f, 1.0f);

sbgl_BindPipeline(ctx, my_pipeline);
sbgl_BindBuffer(ctx, my_vbo, SBGL_BUFFER_USAGE_VERTEX);

// Update interactive data via Push Constants
float time = get_current_time();
sbgl_PushConstants(ctx, sizeof(float), &time);

sbgl_Draw(ctx, 3, 0);

sbgl_EndDrawing(ctx);
```

## Depth Buffering & 3D Sorting

SBgl utilizes a dedicated depth attachment to ensure correct geometry sorting in 3D space. 

- **Automatic Management:** The engine automatically creates a depth buffer matching the window resolution.
- **Pipeline Integration:** All pipelines created via `sbgl_CreatePipeline` have depth testing and depth writing enabled by default.
- **Clearing:** Every frame, the depth buffer is automatically cleared to `1.0` during the `sbgl_BeginDrawing` (or `sbgl_Clear`) phase.

## Synchronization & Frames in Flight

To maximize performance and prevent CPU/GPU bottlenecks, SBgl implements a **Frames in Flight** model.

- **Double Buffering:** The engine uses 2 sets of command buffers and synchronization primitives (semaphores, fences).
- **Overlapping Execution:** This allows the CPU to begin recording the *next* frame while the GPU is still processing the *previous* one.
- **Safe Teardown:** Before destroying resources (e.g., exiting an application), you MUST call `sbgl_DeviceWaitIdle(ctx)` to ensure all in-flight GPU work is complete.

## Batch Rendering & DOD Alignment

The handle system is designed for batching. By iterating through arrays of transformation data (SoA) and binding buffers once, high-performance geometry submission is achieved. Future phases will extend this with Multi-Draw Indirect (MDI) to further reduce CPU overhead.

## Multithreading Considerations

The current implementation records commands into a single primary command buffer per context. To support multithreading:

- The backend will be extended to support Secondary Command Buffers.
- Each worker thread will record commands into its own buffer.
- The main thread will execute all recorded buffers in a single submission.
