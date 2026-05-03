/**
 * @file sbgl_platform.h
 * @brief Internal Platform Abstraction Layer (HAL).
 *
 * Defines the strict interface that OS-specific implementations must follow.
 * Prevents OS headers (windows.h, wayland-client.h) from leaking into the core.
 */

#ifndef SBGL_PLATFORM_H
#define SBGL_PLATFORM_H

#include "sbgl_types.h"
#include <stdbool.h>
#include <stdint.h>

struct SblArena;

/**
 * @brief Creates a native OS window.
 * @param arena Arena to allocate the window state from.
 * @param width Initial width.
 * @param height Initial height.
 * @param title Window title.
 * @return Opaque pointer to the window state.
 */
sbgl_Window* sbgl_os_CreateWindow(
	struct SblArena* arena,
	sbgl_InputState* input,
	int width,
	int height,
	const char* title
);

/**
 * @brief Destroys a native window.
 * @param window The window to destroy.
 */
void sbgl_os_DestroyWindow(sbgl_Window* window);

/**
 * @brief Checks the window's close flag.
 * @param window The window to check.
 * @return True if closing.
 */
bool sbgl_os_WindowShouldClose(sbgl_Window* window);

/**
 * @brief Retrieves the current client area size.
 * @param window The window handle.
 * @param w Pointer to store width.
 * @param h Pointer to store height.
 */
void sbgl_os_GetWindowSize(sbgl_Window* window, int* w, int* h);

/**
 * @brief Checks if the window has been resized since the last check.
 *
 * Resets the internal resize flag to false upon returning.
 *
 * @param window The window handle.
 * @return True if a resize event occurred.
 */
bool sbgl_os_WasWindowResized(sbgl_Window* window);

/**
 * @brief Dispatches OS events (messages/protocol requests).
 * @param window The window to process events for.
 */
void sbgl_os_PollEvents(sbgl_Window* window);

/**
 * @brief Gets the high-resolution performance counter.
 * @return Absolute ticks.
 */
uint64_t sbgl_os_GetPerfCount(void);

/**
 * @brief Gets the performance counter frequency.
 * @return Ticks per second.
 */
uint64_t sbgl_os_GetPerfFreq(void);

/**
 * @brief Retrieves the raw window handle for Vulkan surface creation.
 * @param window The window handle.
 * @return Native handle (e.g., HWND or wl_surface*).
 */
void* sbgl_os_GetNativeWindowHandle(sbgl_Window* window);

/**
 * @brief Retrieves the native instance handle (Win32 specific).
 * @return HINSTANCE on Windows, NULL otherwise.
 */
void* sbgl_os_GetNativeInstanceHandle(void);

/**
 * @brief Retrieves the native display handle (Linux specific).
 * @return Display* or wl_display*.
 */
void* sbgl_os_GetNativeDisplayHandle(void);

#endif // SBGL_PLATFORM_H
