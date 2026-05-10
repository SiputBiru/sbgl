# Compute Shaders in SBgl

Compute shaders provide a mechanism for performing general-purpose calculations on the GPU, outside the traditional graphics pipeline. This capability is essential for data-parallel tasks that do not directly map to vertex or fragment processing, such as physics simulations, advanced culling, and post-processing.

## Differences Between Compute and Graphics Shaders

The graphics pipeline is a fixed-function sequence (with programmable stages like vertex and fragment) designed for transforming geometry into pixels. Compute shaders, however, operate on a more flexible execution model:

* **No Fixed Input/Output:** Unlike vertex shaders (which receive attributes) or fragment shaders (which output colors), compute shaders read from and write to arbitrary memory buffers (SSBOs) or images.
* **Workgroup Execution:** Compute work is organized into "workgroups." Each workgroup contains multiple "local invocations" (threads). This hierarchy allows for efficient data sharing and synchronization within a group.
* **Explicit Dispatch:** The application must explicitly define the number of workgroups to execute in three dimensions (X, Y, Z), rather than relying on geometry or screen resolution.

## Using the Compute API

SBgl provides a streamlined API for creating and executing compute pipelines.

### Creating a Compute Pipeline

A compute pipeline is created from a single shader stage. The shader must be loaded with the `SBGL_SHADER_STAGE_COMPUTE` stage.

```c
sbgl_Shader computeShader = sbgl_LoadShader(ctx, "shaders/compute_task.spv", SBGL_SHADER_STAGE_COMPUTE);
sbgl_ComputePipeline pipeline = sbgl_CreateComputePipeline(ctx, computeShader);
```

### Dispatching Work

Execution is triggered using `sbgl_DispatchCompute`. The parameters specify the number of workgroups to launch.

```c
sbgl_BindComputePipeline(ctx, pipeline);
// Dispatch 64x64 workgroups (Z=1 for 2D tasks)
sbgl_DispatchCompute(ctx, 64, 64, 1);
```

### Memory Synchronization

Because the GPU may execute commands asynchronously, synchronization is required when one operation depends on the results of another. `sbgl_MemoryBarrier` ensures that memory writes are visible to subsequent operations.

```c
// Ensure compute writes are complete before the graphics pipeline reads them
sbgl_MemoryBarrier(ctx, SBGL_BARRIER_COMPUTE_TO_GRAPHICS);

// Ensure graphics writes (e.g., to an SSBO) are visible to compute
sbgl_MemoryBarrier(ctx, SBGL_BARRIER_GRAPHICS_TO_COMPUTE);
```

## GPU-Driven Frustum Culling

One of the most powerful applications of compute shaders in SBgl is GPU-driven culling. In traditional engines, the CPU iterates over every object to determine visibility. For scenes with millions of voxels or instances, this becomes a major bottleneck.

SBgl shifts this responsibility to the GPU:

* **Visibility Testing:** A compute shader tests each chunk or object against the camera's view frustum.
* **Indirect Draw Generation:** The compute shader populates a buffer with `VkDrawIndexedIndirectCommand` structures. Only visible objects are added to this buffer.
* **Single Draw Call:** The CPU issues one `vkCmdDrawIndexedIndirect` call. The GPU then renders exactly the number of visible instances determined by the compute shader.

This approach minimizes CPU overhead and eliminates the need to transfer visibility results back from the GPU.

## Other Applications

The compute API supports various other advanced techniques:

* **Particle Systems:** Simulating millions of particles with complex physics (gravity, collisions) entirely on the GPU.
* **Post-Processing:** Implementing sophisticated image filters like Bloom, Depth of Field, or Temporal Anti-Aliasing (TAA) through compute-based image manipulation.
* **Terrain Generation:** Procedurally generating heightmaps or voxel data on the fly based on player proximity.
* **Physics Integration:** Resolving collisions and constraints for large-scale simulations directly within the graphics memory space.
