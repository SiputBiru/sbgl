#ifndef SBGL_PLATFORM_H
#define SBGL_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include "sbgl_types.h"

// --- Window Management ---
struct SblArena;
sbgl_Window* sbgl_os_CreateWindow(struct SblArena* arena, int width, int height, const char* title);
void         sbgl_os_DestroyWindow(sbgl_Window* window);
bool         sbgl_os_WindowShouldClose(sbgl_Window* window);
void         sbgl_os_GetWindowSize(sbgl_Window* window, int* w, int* h);

// --- Event Polling ---
void sbgl_os_PollEvents(sbgl_Window* window);

// --- Timing ---
uint64_t sbgl_os_GetPerfCount(void);
uint64_t sbgl_os_GetPerfFreq(void);

// --- Vulkan Native Integration ---
void* sbgl_os_GetNativeWindowHandle(sbgl_Window* window);
void* sbgl_os_GetNativeInstanceHandle(void);
void* sbgl_os_GetNativeDisplayHandle(void);

#endif // SBGL_PLATFORM_H
