#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Mock some types from sbgl_types.h and other headers if needed, 
// or just include the headers and mock the functions.

#include "sbgl.h"

// We want to include sbgl_core.c but we need to mock its dependencies
// to avoid linking errors and to control behavior.

// Mocks for sbgl_platform.h
typedef struct sbgl_Window sbgl_Window;
sbgl_Window* sbgl_os_CreateWindow(struct SblArena* arena, sbgl_InputState* input, int w, int h, const char* title) { return (sbgl_Window*)0x1; }
void sbgl_os_DestroyWindow(sbgl_Window* window) {}
bool sbgl_os_WindowShouldClose(sbgl_Window* window) { return false; }
void sbgl_os_GetWindowSize(sbgl_Window* window, int* w, int* h) { if(w) *w=800; if(h) *h=600; }
void sbgl_os_PollEvents(sbgl_Window* window) {}
bool sbgl_os_IsWindowFocused(sbgl_Window* window) { return true; }
void sbgl_os_SetCursorVisible(sbgl_Window* window, bool visible) {}
void sbgl_os_SetCursorLocked(sbgl_Window* window, bool locked) {}

// Mocks for sbgl_graphics_hal.h
typedef struct sbgl_GfxContext sbgl_GfxContext;
sbgl_GfxContext* sbgl_gfx_Init(sbgl_Window* window, struct SblArena* arena) { return (sbgl_GfxContext*)0x1; }
void sbgl_gfx_Shutdown(sbgl_GfxContext* gfx) {}
bool sbgl_gfx_BeginFrame(sbgl_GfxContext* gfx, float r, float g, float b, float a) { return true; }
void sbgl_gfx_EndFrame(sbgl_GfxContext* gfx) {}
void sbgl_gfx_DeviceWaitIdle(sbgl_GfxContext* gfx) {}

// Mocks for other internal headers
void sbgl_internal_log(sbgl_LogLevel level, const char* message) {}
// We might need to mock more if sbgl_core.c calls them.

#include "sbgl_core.c"

static void test_flags_refactor(void) {
    printf("Testing flags refactor...\n");
    
    sbgl_InitResult res = sbgl_Init(800, 600, "Test");
    assert(res.error == SBGL_SUCCESS);
    sbgl_Context* ctx = res.ctx;
    sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

    // After refactor, these should be accessible via inner->state
    // For now, this will FAIL to compile if I use inner->state.isIdle
    
    // Testing initial state
    assert(inner->state.isIdle == 1);
    assert(inner->state.isDrawing == 0);
    
    sbgl_BeginDrawing(ctx);
    assert(inner->state.isDrawing == 1);
    
    sbgl_EndDrawing(ctx);
    assert(inner->state.isDrawing == 0);
    assert(inner->state.isIdle == 0);
    
    sbgl_DeviceWaitIdle(ctx);
    assert(inner->state.isIdle == 1);

    sbgl_Shutdown(ctx);
    printf("PASS: test_flags_refactor\n");
}

int main() {
    test_flags_refactor();
    return 0;
}
