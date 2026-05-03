/**
 * @file sbgl.h
 * @brief Public API for the SiputBiru Graphics Library (SBgl).
 * 
 * This header defines the primary interface for window management,
 * frame lifecycle, and input handling.
 */

#ifndef SBGL_H
#define SBGL_H

#include <stdbool.h>
#include <stdint.h>
#include "sbgl_types.h"

// --- Scancodes (Subset of physical keys) ---

#define SBGL_KEY_UNKNOWN 0
#define SBGL_KEY_A 4
#define SBGL_KEY_B 5
#define SBGL_KEY_C 6
#define SBGL_KEY_D 7
#define SBGL_KEY_E 8
#define SBGL_KEY_F 9
#define SBGL_KEY_G 10
#define SBGL_KEY_H 11
#define SBGL_KEY_I 12
#define SBGL_KEY_J 13
#define SBGL_KEY_K 14
#define SBGL_KEY_L 15
#define SBGL_KEY_M 16
#define SBGL_KEY_N 17
#define SBGL_KEY_O 18
#define SBGL_KEY_P 19
#define SBGL_KEY_Q 20
#define SBGL_KEY_R 21
#define SBGL_KEY_S 22
#define SBGL_KEY_T 23
#define SBGL_KEY_U 24
#define SBGL_KEY_V 25
#define SBGL_KEY_W 26
#define SBGL_KEY_X 27
#define SBGL_KEY_Y 28
#define SBGL_KEY_Z 29

#define SBGL_KEY_1 30
#define SBGL_KEY_2 31
#define SBGL_KEY_3 32
#define SBGL_KEY_4 33
#define SBGL_KEY_5 34
#define SBGL_KEY_6 35
#define SBGL_KEY_7 36
#define SBGL_KEY_8 37
#define SBGL_KEY_9 38
#define SBGL_KEY_0 39

#define SBGL_KEY_RETURN 40
#define SBGL_KEY_ESCAPE 41
#define SBGL_KEY_BACKSPACE 42
#define SBGL_KEY_TAB 43
#define SBGL_KEY_SPACE 44
#define SBGL_KEY_RIGHT 79
#define SBGL_KEY_LEFT 80
#define SBGL_KEY_DOWN 81
#define SBGL_KEY_UP 82
#define SBGL_KEY_LSHIFT 225
#define SBGL_KEY_LCTRL 224
#define SBGL_KEY_LALT 226

#define SBGL_MOUSE_LEFT 0
#define SBGL_MOUSE_RIGHT 1
#define SBGL_MOUSE_MIDDLE 2

// --- API ---

/**
 * @brief Initializes the engine and opens a window.
 * 
 * This function sets up the internal HALs and the Vulkan 1.3 backend.
 * 
 * @param w Desired width of the window.
 * @param h Desired height of the window.
 * @param title Title displayed in the OS window manager.
 * @return An initialization result containing the context and any error code.
 */
sbgl_InitResult sbgl_Init(int w, int h, const char* title);

/**
 * @brief Gracefully shuts down the engine and releases all resources.
 * @param ctx The context to destroy.
 */
void          sbgl_Shutdown(sbgl_Context* ctx);

/**
 * @brief Checks if the user or OS has requested to close the window.
 * 
 * This flag is set by the OS (e.g., clicking the 'X' button or Alt+F4). 
 * The application must poll this flag in the main loop and manually 
 * initiate shutdown by calling sbgl_Shutdown() when it returns true.
 * 
 * @param ctx The active engine context.
 * @return True if a close request is pending, false otherwise.
 */
bool          sbgl_WindowShouldClose(sbgl_Context* ctx);

/**
 * @brief Retrieves the current window dimensions.
 * @param ctx The active engine context.
 * @param w Pointer to store the width.
 * @param h Pointer to store the height.
 */
void          sbgl_GetWindowSize(sbgl_Context* ctx, int* w, int* h);

/**
 * @brief Prepares the engine for a new frame of drawing.
 * 
 * This function handles event polling and Vulkan swapchain image acquisition.
 * Must be called before any clearing or drawing commands.
 * 
 * @param ctx The engine context.
 */
void sbgl_BeginDrawing(sbgl_Context* ctx);

/**
 * @brief Finalizes the current frame and presents it to the screen.
 * 
 * This function submits the recorded GPU commands and swaps the buffer.
 * 
 * @param ctx The engine context.
 */
void sbgl_EndDrawing(sbgl_Context* ctx);

/**
 * @brief Sets the clear color for the next frame.
 * 
 * @param ctx The engine context.
 * @param r Red component (0.0 to 1.0).
 * @param g Green component (0.0 to 1.0).
 * @param b Blue component (0.0 to 1.0).
 * @param a Alpha component (0.0 to 1.0).
 */
void sbgl_Clear(sbgl_Context* ctx, float r, float g, float b, float a);

/**
 * @brief Retrieves the complete input state for the current frame.
 * 
 * Adheres to Data-Oriented Design by providing a read-only view of all 
 * input arrays (keys, mouse) for efficient batch processing.
 * 
 * @param ctx The engine context.
 * @return A read-only pointer to the current input state. Never returns NULL.
 */
const sbgl_InputState* sbgl_GetInputState(sbgl_Context* ctx);

#endif // SBGL_H
