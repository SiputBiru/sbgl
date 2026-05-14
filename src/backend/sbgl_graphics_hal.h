#ifndef SBGL_GRAPHICS_HAL_H
#define SBGL_GRAPHICS_HAL_H

#include <stdbool.h>
#include <stddef.h>

/**
 * SBGL Internal Graphics HAL
 *
 * Defines the interface between the core engine and the specific
 * graphics backend (Vulkan, etc.).
 */

#include "sbgl_types.h"

struct SblArena;

/**
 * @brief Opaque handle for the graphics backend context.
 */
typedef struct sbgl_GfxContext sbgl_GfxContext;

/**
 * @brief Initializes the graphics backend with configurable resource limits.
 *
 * @param window The platform window handle.
 * @param arena The arena for persistent allocations.
 * @param limits Pointer to resource limits (must not be NULL).
 * @param enableValidation Whether to enable Vulkan validation layers.
 * @return A pointer to the graphics context, or NULL on failure.
 */
sbgl_GfxContext* sbgl_gfx_Init(sbgl_Window* window, struct SblArena* arena, const sbgl_ResourceLimits* limits, bool enableValidation);
void sbgl_gfx_Shutdown(sbgl_GfxContext* ctx);

/**
 * @brief Starts a new frame, acquiring an image and starting the command buffer.
 *
 * This must be called before any GPU commands (Compute or Graphics) are recorded.
 */
bool sbgl_gfx_BeginFrame(sbgl_GfxContext* ctx);

/**
 * @brief Submits the current frame's commands and presents the image.
 */
void sbgl_gfx_EndFrame(sbgl_GfxContext* ctx);

/**
 * @brief Starts a graphics rendering pass.
 *
 * This must be called before any draw commands are recorded. It handles
 * clearing the attachments if requested.
 */
void sbgl_gfx_BeginRenderPass(sbgl_GfxContext* ctx, float r, float g, float b, float a);

/**
 * @brief Ends the current graphics rendering pass.
 */
void sbgl_gfx_EndRenderPass(sbgl_GfxContext* ctx);

void sbgl_gfx_DeviceWaitIdle(sbgl_GfxContext* ctx);

sbgl_Buffer
sbgl_gfx_CreateBuffer(sbgl_GfxContext* ctx, sbgl_BufferUsage usage, size_t size, const void* data);
void sbgl_gfx_DestroyBuffer(sbgl_GfxContext* ctx, sbgl_Buffer buffer);

/**
 * @brief Performs a hardware-accelerated buffer fill.
 */
void sbgl_gfx_FillBuffer(sbgl_GfxContext* ctx, sbgl_Buffer buffer, size_t offset, size_t size, uint32_t value);

/**
 * @brief Retrieves the current backend frame index.
 */
uint32_t sbgl_gfx_GetFrameIndex(sbgl_GfxContext* ctx);

void* sbgl_gfx_MapBuffer(sbgl_GfxContext* ctx, sbgl_Buffer buffer);
void sbgl_gfx_UnmapBuffer(sbgl_GfxContext* ctx, sbgl_Buffer buffer);

sbgl_Shader
sbgl_gfx_LoadShader(sbgl_GfxContext* ctx, sbgl_ShaderStage stage, const uint32_t* bytecode, size_t size);
void sbgl_gfx_DestroyShader(sbgl_GfxContext* ctx, sbgl_Shader shader);

sbgl_Pipeline sbgl_gfx_CreatePipeline(sbgl_GfxContext* ctx, const sbgl_PipelineConfig* config);
void sbgl_gfx_DestroyPipeline(sbgl_GfxContext* ctx, sbgl_Pipeline pipeline);

sbgl_ComputePipeline sbgl_gfx_CreateComputePipeline(sbgl_GfxContext* ctx, sbgl_Shader shader);
void sbgl_gfx_DestroyComputePipeline(sbgl_GfxContext* ctx, sbgl_ComputePipeline pipeline);

void sbgl_gfx_BindComputePipeline(sbgl_GfxContext* ctx, sbgl_ComputePipeline pipeline);
void sbgl_gfx_DispatchCompute(sbgl_GfxContext* ctx, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
void sbgl_gfx_MemoryBarrier(sbgl_GfxContext* ctx, sbgl_BarrierType type);

void sbgl_gfx_BindPipeline(sbgl_GfxContext* ctx, sbgl_Pipeline pipeline);
void sbgl_gfx_BindBuffer(sbgl_GfxContext* ctx, sbgl_Buffer buffer, sbgl_BufferUsage usage);

/**
 * @brief Represents a slice of a persistent GPU buffer used for transient data.
 */
typedef struct {
  sbgl_Buffer buffer;       /**< Handle to the underlying GPU buffer. */
  uint32_t offset;       /**< Offset in bytes within the buffer. */
  uint32_t size;         /**< Size in bytes of the allocated region. */
  void* mapped;         /**< CPU-side pointer for writing data. */
  uint64_t deviceAddress;     /**< GPU-side virtual address for the allocation. */
} sbgl_GfxTransientAllocation;

void sbgl_gfx_Draw(sbgl_GfxContext* ctx, uint32_t vertexCount, uint32_t firstVertex, uint32_t instanceCount);
void sbgl_gfx_DrawIndexed(
  sbgl_GfxContext* ctx,
  uint32_t indexCount,
  uint32_t firstIndex,
  int32_t vertexOffset,
  uint32_t instanceCount
);

/**
 * @brief Submits a batch of draw calls stored in a GPU buffer.
 *
 * @param ctx The graphics context.
 * @param buffer Handle to the buffer containing an array of sbgl_IndirectCommand.
 * @param offset The byte offset into the buffer where the commands begin.
 * @param drawCount The number of commands to execute from the buffer.
 */
void sbgl_gfx_DrawIndirect(
  sbgl_GfxContext* ctx,
  sbgl_Buffer buffer,
  size_t offset,
  uint32_t drawCount
);

/**
 * @brief Allocates a slice of GPU-visible memory for transient per-frame data.
 *
 * This memory is managed by the backend's internal per-frame ring buffers and 
 * does not require manual destruction.
 *
 * @param ctx The graphics context.
 * @param size The number of bytes to allocate.
 * @param alignment The required byte alignment for the allocation.
 * @return A structure containing the allocation metadata and mapped pointer.
 */
sbgl_GfxTransientAllocation
sbgl_gfx_AllocateTransient(sbgl_GfxContext* ctx, size_t size, uint32_t alignment);


/**
 * @brief Retrieves the 64-bit GPU virtual address for a buffer.
 *
 * Used primarily for passing buffer pointers to shaders via push constants
 * or storage buffers when using VK_KHR_buffer_device_address.
 *
 * @param ctx The graphics context.
 * @param buffer The buffer to query.
 * @return The 64-bit device address, or 0 if retrieval failed.
 */
uint64_t sbgl_gfx_GetBufferDeviceAddress(sbgl_GfxContext* ctx, sbgl_Buffer buffer);

/**
 * @brief Marks a buffer for destruction after current frames complete.
 *
 * This function should be used for temporary buffers that are submitted 
 * for GPU execution in the current frame and must not be destroyed 
 * until the GPU has finished using them.
 *
 * @param ctx The graphics context.
 * @param buffer Handle to the buffer to destroy.
 */
void sbgl_gfx_DestroyBufferDeferred(sbgl_GfxContext* ctx, sbgl_Buffer buffer);

void sbgl_gfx_PushConstants(sbgl_GfxContext* ctx, size_t size, const void* data);

/**
 * @brief Retrieves the elapsed GPU time for the previous frame in milliseconds.
 *
 * @param ctx The graphics context.
 * @return The duration in milliseconds.
 */
float sbgl_gfx_GetGpuTime(sbgl_GfxContext* ctx);

/**
 * @brief Retrieves the last VkResult from the backend for error inspection.
 *
 * @param ctx The graphics context.
 * @return The last VkResult code, or 0 if no error occurred.
 */
int32_t sbgl_gfx_GetLastVkResult(sbgl_GfxContext* ctx);

#endif // SBGL_GRAPHICS_HAL_H
