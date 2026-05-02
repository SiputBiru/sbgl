#ifndef SBGL_TYPES_H
#define SBGL_TYPES_H

/**
 * SBGL Shared Opaque Types (Private)
 */

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
    void*       inner;
    sbgl_Result result;
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
