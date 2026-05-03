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

bool sbgl_gfx_Init(sbgl_Window* window, struct SblArena* arena);
void sbgl_gfx_Shutdown(void);

bool sbgl_gfx_BeginFrame(float r, float g, float b, float a);
void sbgl_gfx_EndFrame(void);

sbgl_Buffer   sbgl_gfx_CreateBuffer(sbgl_BufferUsage usage, size_t size, const void* data);
void          sbgl_gfx_DestroyBuffer(sbgl_Buffer buffer);

sbgl_Shader   sbgl_gfx_LoadShader(sbgl_ShaderStage stage, const uint32_t* bytecode, size_t size);
void          sbgl_gfx_DestroyShader(sbgl_Shader shader);

sbgl_Pipeline sbgl_gfx_CreatePipeline(const sbgl_PipelineConfig* config);
void          sbgl_gfx_DestroyPipeline(sbgl_Pipeline pipeline);

void sbgl_gfx_BindPipeline(sbgl_Pipeline pipeline);
void sbgl_gfx_BindBuffer(sbgl_Buffer buffer, sbgl_BufferUsage usage);

void sbgl_gfx_Draw(uint32_t vertexCount, uint32_t firstVertex);
void          sbgl_gfx_DrawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset);

void          sbgl_gfx_PushConstants(size_t size, const void* data);

#endif // SBGL_GRAPHICS_HAL_H

