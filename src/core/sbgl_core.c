#include "sbgl.h"
#define SBL_ARENA_IMPLEMENTATION
#include "core/sbl_arena.h"
#include "core/sbgl_platform.h"
#include "core/sbgl_input.h"
#include "backend/sbgl_graphics_hal.h"
#include <stdlib.h>

/**
 * @brief Internal state for the engine context.
 * 
 * This structure is hidden from the public API (opaque). It holds the
 * persistent memory arena, the native window handle, and the current
 * clear color state.
 */
typedef struct {
    SblArena     arena;        /**< Persistent memory for the lifetime of the context. */
    sbgl_Window* window;       /**< Handle to the native OS window. */
    float        clearColor[4]; /**< Current RGBA clear color. */
} sbgl_InternalContext;

sbgl_InitResult sbgl_Init(int w, int h, const char* title) {
    sbgl_InitResult res = { .ctx = NULL, .error = SBGL_SUCCESS };

    // Setup the basic context shell
    sbgl_Context* ctx = malloc(sizeof(sbgl_Context));
    if (!ctx) {
        res.error = SBGL_ERROR_OUT_OF_MEMORY;
        return res;
    }

    // Setup internal state using an arena
    sbgl_InternalContext* inner = malloc(sizeof(sbgl_InternalContext));
    if (!inner) {
        free(ctx);
        res.error = SBGL_ERROR_OUT_OF_MEMORY;
        return res;
    }
    
    sbl_arena_init(&inner->arena, 4 * 1024 * 1024); // 4MB default
    inner->clearColor[0] = 0.1f;
    inner->clearColor[1] = 0.2f;
    inner->clearColor[2] = 0.3f;
    inner->clearColor[3] = 1.0f;

    ctx->inner = inner;
    ctx->result = SBGL_SUCCESS;
    res.ctx = ctx;

    // Initialize Platform
    inner->window = sbgl_os_CreateWindow(&inner->arena, w, h, title);
    if (!inner->window) {
        ctx->result = SBGL_ERROR_WINDOW_CREATION_FAILED;
        res.error = ctx->result;
        return res;
    }

    // Initialize Graphics
    if (!sbgl_gfx_Init(inner->window)) {
        ctx->result = SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
        res.error = ctx->result;
        return res;
    }

    return res;
}

void sbgl_Shutdown(sbgl_Context* ctx) {
    if (!ctx) return;
    sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
    
    sbgl_gfx_Shutdown();
    if (inner->window) sbgl_os_DestroyWindow(inner->window);
    
    sbl_arena_free(&inner->arena);
    free(inner);
    free(ctx);
}

bool sbgl_WindowShouldClose(sbgl_Context* ctx) {
    if (!ctx) return true;
    sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
    return sbgl_os_WindowShouldClose(inner->window);
}

void sbgl_BeginDrawing(sbgl_Context* ctx) {
    if (!ctx) return;
    sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
    
    // Ensure we poll events so Wayland can map the window
    sbgl_os_PollEvents(inner->window);
    
    sbgl_gfx_BeginFrame(inner->clearColor[0], inner->clearColor[1], inner->clearColor[2], inner->clearColor[3]);
}

void sbgl_EndDrawing(sbgl_Context* ctx) {
    if (!ctx) return;
    sbgl_gfx_EndFrame();
}

void sbgl_Clear(sbgl_Context* ctx, float r, float g, float b, float a) {
    if (!ctx) return;
    sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
    inner->clearColor[0] = r;
    inner->clearColor[1] = g;
    inner->clearColor[2] = b;
    inner->clearColor[3] = a;
}

bool sbgl_IsKeyDown(sbgl_Context* ctx, int scancode) {
    (void)ctx;
    return sbgl_os_IsKeyDown((SBGL_Scancode)scancode);
}

bool sbgl_IsKeyPressed(sbgl_Context* ctx, int scancode) {
    (void)ctx;
    return sbgl_os_IsKeyPressed((SBGL_Scancode)scancode);
}

bool sbgl_IsMouseButtonDown(sbgl_Context* ctx, int button) {
    (void)ctx;
    return sbgl_os_IsMouseButtonDown((SBGL_MouseButton)button);
}

void sbgl_GetMousePos(sbgl_Context* ctx, int* x, int* y) {
    (void)ctx;
    sbgl_os_GetMousePos(x, y);
}

void sbgl_GetMouseDelta(sbgl_Context* ctx, int* dx, int* dy) {
    (void)ctx;
    sbgl_os_GetMouseDelta(dx, dy);
}
