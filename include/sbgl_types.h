#ifndef SBGL_TYPES_H
#define SBGL_TYPES_H

#include <stdbool.h>

#define SBGL_MAX_KEYS 512
#define SBGL_MAX_MOUSE_BUTTONS 8

/**
 * @brief Represents the real-time state of physical inputs.
 */
typedef struct sbgl_InputState {
    bool keysDown[SBGL_MAX_KEYS];
    bool keysPressed[SBGL_MAX_KEYS];
    bool mouseDown[SBGL_MAX_MOUSE_BUTTONS];
    int mouseX, mouseY;
    int mouseDeltaX, mouseDeltaY;
    int _internalMouseX, _internalMouseY; /**< Internal tracking for deltas. */
} sbgl_InputState;

/**
 * @brief Result codes for engine operations.
 */
typedef enum {
    SBGL_SUCCESS = 0,
    SBGL_ERROR_INITIALIZATION_FAILED = 1,
    SBGL_ERROR_WINDOW_CREATION_FAILED = 2,
    SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED = 3,
    SBGL_ERROR_OUT_OF_MEMORY = 4,
} sbgl_Result;

/**
 * @brief Primary engine context.
 * 
 * The context serves as the central handle for all SBgl operations. It utilizes
 * an opaque pointer pattern to encapsulate internal engine state, ensuring
 * that OS-specific handles and internal memory management are hidden from
 * the public API.
 */
typedef struct sbgl_Context {
    /** 
     * @brief Opaque pointer to the internal engine state. 
     * 
     * Points to the private `sbgl_InternalContext` structure, which manages 
     * the following internal subsystems:
     * - **Memory Arena**: The persistent `SblArena` used for context-local allocations.
     * - **Native Window**: The platform-specific `sbgl_Window` handle.
     * - **Graphics State**: Clear colors and frame acquisition flags.
     * - **Input State**: The real-time physical state of keys and mouse buttons.
     */
    void*           inner;
    
    /** 
     * @brief Status of the last major operation. 
     * 
     * Stores the result or error code from the most recent critical API 
     * call (e.g., initialization or frame acquisition).
     */
    sbgl_Result     result;
} sbgl_Context;

/**
 * @brief Result structure for initialization.
 */
typedef struct {
    sbgl_Context* ctx;
    sbgl_Result   error;
} sbgl_InitResult;

typedef struct sbgl_Window sbgl_Window;

#endif // SBGL_TYPES_H
