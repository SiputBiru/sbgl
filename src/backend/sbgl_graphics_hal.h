#ifndef SBGL_GRAPHICS_HAL_H
#define SBGL_GRAPHICS_HAL_H

#include <stdbool.h>

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

#endif // SBGL_GRAPHICS_HAL_H
