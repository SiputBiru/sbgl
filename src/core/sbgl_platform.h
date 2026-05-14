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
#include "sbgl_input.h"
#include <stdbool.h>
#include <stdint.h>

struct SblArena;

/**
 * @brief Creates a native OS window.
 * @param arena Arena to allocate the window state from.
 * @param input Pointer to input state.
 * @param width Initial width.
 * @param height Initial height.
 * @param title Window title.
 * @param outWindow Pointer to store the created window.
 * @return Platform result code.
 */
sbgl_platform_Result sbgl_os_CreateWindow(
	struct SblArena* arena,
	sbgl_InputState* input,
	int width,
	int height,
	const char* title,
	sbgl_Window** outWindow
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
 * @brief Sets the visibility of the OS cursor for the given window.
 *
 * Provides a platform-agnostic way to show or hide the mouse pointer.
 *
 * @param window The native window handle.
 * @param visible True to show the cursor, false to hide it.
 */
void sbgl_os_SetCursorVisible(sbgl_Window* window, bool visible);

/**
 * @brief Locks or unlocks the cursor within the window bounds.
 *
 * When locked, the cursor is typically constrained to the window center
 * to support relative motion for 3D navigation.
 *
 * @param window The native window handle.
 * @param locked True to capture the cursor, false to release it.
 */
void sbgl_os_SetCursorLocked(sbgl_Window* window, bool locked);

/**
 * @brief Checks if the window currently has input focus.
 * @param window The native window handle.
 * @return True if focused.
 */
bool sbgl_os_IsWindowFocused(sbgl_Window* window);

/**
 * @brief Dispatches OS events (messages/protocol requests).
 * @param window The window to process events for.
 */
void sbgl_os_PollEvents(sbgl_Window* window);

/**
 * @brief Gets the high-resolution performance counter.
 * @param window The window handle.
 * @return Absolute ticks.
 */
uint64_t sbgl_os_GetPerfCount(sbgl_Window* window);

/**
 * @brief Gets the performance counter frequency.
 * @param window The window handle.
 * @return Ticks per second.
 */
uint64_t sbgl_os_GetPerfFreq(sbgl_Window* window);

/**
 * @brief Retrieves the raw window handle for Vulkan surface creation.
 * @param window The window handle.
 * @return Native handle (e.g., HWND or wl_surface*).
 */
void* sbgl_os_GetNativeWindowHandle(sbgl_Window* window);

/**
 * @brief Retrieves the native instance handle (Win32 specific).
 * @param window The window handle.
 * @return HINSTANCE on Windows, NULL otherwise.
 */
void* sbgl_os_GetNativeInstanceHandle(sbgl_Window* window);

/**
 * @brief Retrieves the native display handle (Linux specific).
 * @param window The window handle.
 * @return Display* or wl_display*.
 */
void* sbgl_os_GetNativeDisplayHandle(sbgl_Window* window);

#endif // SBGL_PLATFORM_H
