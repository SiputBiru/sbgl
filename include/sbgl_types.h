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
 */
typedef struct sbgl_Context {
    void*           inner;
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
