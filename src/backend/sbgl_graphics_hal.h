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

sbgl_GfxContext* sbgl_gfx_Init(sbgl_Window* window, struct SblArena* arena);
void sbgl_gfx_Shutdown(sbgl_GfxContext* ctx);

bool sbgl_gfx_BeginFrame(sbgl_GfxContext* ctx, float r, float g, float b, float a);
void sbgl_gfx_EndFrame(sbgl_GfxContext* ctx);
void sbgl_gfx_DeviceWaitIdle(sbgl_GfxContext* ctx);

sbgl_Buffer
sbgl_gfx_CreateBuffer(sbgl_GfxContext* ctx, sbgl_BufferUsage usage, size_t size, const void* data);
void sbgl_gfx_DestroyBuffer(sbgl_GfxContext* ctx, sbgl_Buffer buffer);

sbgl_Shader
sbgl_gfx_LoadShader(sbgl_GfxContext* ctx, sbgl_ShaderStage stage, const uint32_t* bytecode, size_t size);
void sbgl_gfx_DestroyShader(sbgl_GfxContext* ctx, sbgl_Shader shader);

sbgl_Pipeline sbgl_gfx_CreatePipeline(sbgl_GfxContext* ctx, const sbgl_PipelineConfig* config);
void sbgl_gfx_DestroyPipeline(sbgl_GfxContext* ctx, sbgl_Pipeline pipeline);

void sbgl_gfx_BindPipeline(sbgl_GfxContext* ctx, sbgl_Pipeline pipeline);
void sbgl_gfx_BindBuffer(sbgl_GfxContext* ctx, sbgl_Buffer buffer, sbgl_BufferUsage usage);

/**
 * @brief Data layout for a GPU-side indirect draw command.
 *
 * This structure matches the layout expected by vkCmdDrawIndexedIndirect,
 * allowing the GPU to read draw parameters directly from a buffer.
 */
typedef struct {
    uint32_t indexCount;    /**< Number of indices to draw. */
    uint32_t instanceCount; /**< Number of instances to draw. */
    uint32_t firstIndex;    /**< Byte offset of the first index in the index buffer. */
    int32_t  vertexOffset;  /**< Value added to each index before addressing the vertex buffer. */
    uint32_t firstInstance; /**< ID of the first instance to draw. */
} sbgl_IndirectCommand;

void sbgl_gfx_Draw(sbgl_GfxContext* ctx, uint32_t vertexCount, uint32_t firstVertex);
void sbgl_gfx_DrawIndexed(
	sbgl_GfxContext* ctx,
	uint32_t indexCount,
	uint32_t firstIndex,
	int32_t vertexOffset
);

/**
 * @brief Submits a batch of draw calls stored in a GPU buffer.
 *
 * @param ctx The graphics context.
 * @param buffer Handle to the buffer containing an array of sbgl_IndirectCommand.
 * @param drawCount The number of commands to execute from the buffer.
 */
void sbgl_gfx_DrawIndirect(sbgl_GfxContext* ctx, sbgl_Buffer buffer, uint32_t drawCount);

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

#endif // SBGL_GRAPHICS_HAL_H
