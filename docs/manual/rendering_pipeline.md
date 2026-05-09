# SBgl Rendering Pipeline

The SBgl Rendering Pipeline is built on Vulkan 1.3's Dynamic Rendering, eliminating the need for complex RenderPass and Framebuffer boilerplate. It follows Data-Oriented Design (DOD) principles by using lightweight integer handles for resource management.

## Key Concepts

### Handle-Based Resource Management

Resources like Buffers, Shaders, and Pipelines are referenced via `sbgl_Buffer`, `sbgl_Shader`, and `sbgl_Pipeline` handles (32-bit unsigned integers). Internally, these handles map to contiguous arrays in the Vulkan backend, ensuring cache-efficient access.

### Explicit Pipeline State Objects (PSO)

SBgl requires explicit creation of graphics pipelines. A pipeline encapsulates the vertex/fragment shaders and the vertex input layout. This design ensures predictable performance by moving pipeline compilation to initialization time.

### Vertex Input Layout

The `sbgl_VertexLayout` structure defines how vertex data in a buffer is mapped to shader input locations.

*   **Stride:** Total size in bytes of a single vertex.
*   **Attributes:** Array of `sbgl_VertexAttribute` defining the location and offset of each component.

## Shader Loading Strategies

SBgl supports two primary ways to load SPIR-V shaders: dynamic file loading and hardcoded byte arrays.

### Dynamic File Loading

Ideal for development and modding, allowing shaders to be recompiled without rebuilding the application.

```c
size_t size;
uint32_t* bytecode = read_spirv_file("examples/shaders/example_shader.vert.spv", &size);
sbgl_Shader shader = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_VERTEX, bytecode, size);
// ... handle cleanup of bytecode if necessary ...
```

### Hardcoded (Static) Loading

Recommended for production releases to ensure the application is self-contained and prevents tampering with core assets.

The `xxd` tool facilitates converting a compiled `.spv` file into a C header:
`xxd -i examples/shaders/example_shader.vert.spv > example_shader_vert.h`

```c
#include "example_shader_vert.h"
// xxd generates an array named 'example_shader_vert_spv' and 'example_shader_vert_spv_len'
sbgl_Shader shader = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_VERTEX, 
                                     (uint32_t*)example_shader_vert_spv, 
                                     example_shader_vert_spv_len);
```

## Vertex Layout Use Case

Defining a standard vertex with Position and Color attributes using the library-provided `sbgl_Vertex` structure:

```c
// sbgl_Vertex is defined as:
// typedef struct {
//     sbgl_Vec3 position;
//     sbgl_Vec3 color;
// } sbgl_Vertex;

sbgl_VertexAttribute attributes[] = {
    { .location = 0, .offset = offsetof(sbgl_Vertex, position), .format = SBGL_FORMAT_R32G32B32_SFLOAT },
    { .location = 1, .offset = offsetof(sbgl_Vertex, color), .format = SBGL_FORMAT_R32G32B32_SFLOAT }
};

sbgl_VertexLayout layout = {
    .stride = sizeof(sbgl_Vertex),
    .attributeCount = 2,
    .attributes = attributes
};
```

## Complete Drawing Example

```c
sbgl_BeginDrawing(ctx);
sbgl_Clear(ctx, 0.0f, 0.0f, 0.0f, 1.0f);

sbgl_BindPipeline(ctx, example_pipeline);
sbgl_BindBuffer(ctx, example_vbo, SBGL_BUFFER_USAGE_VERTEX);

// Update interactive data via Push Constants
float time = get_current_time();
sbgl_PushConstants(ctx, sizeof(float), &time);

sbgl_Draw(ctx, 3, 0);

sbgl_EndDrawing(ctx);

// Teardown: Wait for GPU to finish before destroying resources
sbgl_DeviceWaitIdle(ctx);
sbgl_DestroyPipeline(ctx, example_pipeline);
sbgl_DestroyBuffer(ctx, example_vbo);
```

## Depth Buffering & 3D Sorting

SBgl utilizes a dedicated depth attachment to ensure correct geometry sorting in 3D space. 

*   **Automatic Management:** The engine automatically creates a depth buffer matching the window resolution.
*   **Pipeline Integration:** All pipelines created via `sbgl_CreatePipeline` have depth testing and depth writing enabled by default.
*   **Clearing:** Every frame, the depth buffer is automatically cleared to `1.0` during the `sbgl_BeginDrawing` (or `sbgl_Clear`) phase.

## Synchronization & Frames in Flight

To maximize efficiency and prevent CPU/GPU bottlenecks, SBgl implements a **Frames in Flight** model.

*   **Double Buffering:** The engine uses 2 sets of command buffers and synchronization primitives (semaphores, fences).
*   **Overlapping Execution:** This allows the CPU to begin recording the *next* frame while the GPU is still processing the *previous* one.
*   **Safe Teardown:** Before destroying resources (e.g., exiting an application), `sbgl_DeviceWaitIdle(ctx)` MUST be called to ensure all in-flight GPU work is complete.

## Automated Batching

SBgl provides an automated batching system to minimize CPU-to-GPU communication overhead. This system collects multiple draw requests into a single submission, leveraging GPU-side features like Indirect Drawing.

### Render Queues

A `sbgl_RenderQueue` acts as a collector for draw commands. It is initialized using an `SblArena` to ensure efficient, contiguous memory allocation for the queued data.

```c
SblArena arena;
sbl_arena_init(&arena, 1024 * 1024);
sbgl_RenderQueue* queue = sbgl_CreateRenderQueue(ctx, &arena);
```

### Submitting Draws

Instead of immediate execution, draw calls are submitted to a queue. The system stores the vertex count, first vertex, and instance count for each draw.

```c
// Queue up 10,000 instances of a single triangle
for (int i = 0; i < 10000; i++) {
    sbgl_InstanceData data;
    data.transform = sbgl_Mat4Translate(sbgl_Vec3Set((float)i * 0.1f, 0.0f, 0.0f));
    data.color = sbgl_Vec4Set(1.0f, 1.0f, 1.0f, 1.0f);
    
    // meshId=0, materialId=0, blend=0, sided=0, tags=0, sortKey=0
    sbgl_SubmitDraw(queue, 0, 0, 0, 0, 0, 0, &data);
}
```

### Executing the Queue

The `sbgl_RenderQueues` function processes one or more queues. It performs internal sorting (e.g., via Radix Sort in the backend) and "bakes" the draws into a single Vulkan Indirect Draw command.

```c
sbgl_BeginDrawing(ctx);
sbgl_BindPipeline(ctx, pipeline);
sbgl_BindBuffer(ctx, vbo, SBGL_BUFFER_USAGE_VERTEX);

// Process and execute all queued draws
sbgl_Mat4 vp = sbgl_Mat4Identity();
sbgl_RenderQueues(ctx, &queue, 1, &vp);

sbgl_EndDrawing(ctx);
```

## Optimized Batch Submission

To resolve CPU bottlenecks during high-frequency geometry submission (e.g., voxels, particle systems), SBgl utilizes an internal **Transient Allocation** system.

### Persistent Mapping
Dynamic data required for each frame—such as per-instance transformation matrices and indirect draw commands—is written directly into a set of persistently mapped GPU buffers.
- **Zero Allocation Overhead:** Unlike standard buffer creation, transient allocation simply increments a pointer in a pre-allocated pool.
- **Cache Efficiency:** Data is written sequentially by the CPU and read sequentially by the GPU, maximizing throughput.

### Multi-Draw Indirect (MDI)
The system "bakes" sorted draw packets into `sbgl_IndirectCommand` structures. These are submitted using `vkCmdDrawIndexedIndirect`, allowing the GPU to process massive numbers of draw calls with a single command dispatch from the CPU.

## Batch Rendering & DOD Alignment

The handle system is designed for batching. By iterating through arrays of transformation data (SoA) and binding buffers once, geometry submission is achieved. The automated batcher utilizes Multi-Draw Indirect (MDI) to reduce CPU overhead by submitting geometry with a single Vulkan command.

### Dynamic Vertex Updates

For CPU-driven effects (e.g., particle systems), vertex buffers can be recreated or updated every frame. The system handles the underlying synchronization to ensure the GPU has finished using the old buffer before it is destroyed.

```c
// Updating a vertex array every frame
sbgl_Vertex particles[100];
simulate_particles(particles, 100);

// Create a new buffer for this frame
sbgl_Buffer vbo = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_VERTEX, sizeof(particles), particles);

// Bind and draw
sbgl_BindBuffer(ctx, vbo, SBGL_BUFFER_USAGE_VERTEX);
sbgl_Draw(ctx, 100, 0);

// Use deferred destruction to safely release the buffer after the frame is retired
sbgl_DestroyBufferDeferred(ctx, vbo);
```

### Multi-Queue Submission

Managing separate render queues allows for logical separation of draw calls (e.g., Opaque, Transparent, UI). These queues can be merged and processed in a single backend call to optimize pipeline state changes.

```c
sbgl_RenderQueue* opaque_queue;
sbgl_RenderQueue* ui_queue;

// ... populate queues ...

sbgl_RenderQueue* queues[] = { opaque_queue, ui_queue };
sbgl_Mat4 vp = get_view_proj();

// Process all queues, performing cross-queue sorting for state optimization
sbgl_RenderQueues(ctx, queues, 2, &vp);
```

## Multithreading Considerations

The current implementation records commands into a single primary command buffer per context. To support multithreading:

*   The backend will be extended to support Secondary Command Buffers.
*   Each worker thread will record commands into its own buffer.
*   The main thread will execute all recorded buffers in a single submission.
