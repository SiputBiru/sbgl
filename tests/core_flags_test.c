#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "sbgl.h"
#include "sbgl_types.h"
#include "core/sbl_arena.h"
#include "core/sbgl_platform.h"
#include "backend/sbgl_graphics_hal.h"
#include "core/sbgl_internal_log.h"

/* Mock implementations for platform HAL. */

sbgl_Window* sbgl_os_CreateWindow(struct SblArena* arena, sbgl_InputState* input, int w, int h, const char* title) {
    (void)arena; (void)input; (void)w; (void)h; (void)title;
    return (sbgl_Window*)0x1;
}

void sbgl_os_DestroyWindow(sbgl_Window* window) {
    (void)window;
}

bool sbgl_os_WindowShouldClose(sbgl_Window* window) {
    (void)window;
    return false;
}

void sbgl_os_GetWindowSize(sbgl_Window* window, int* w, int* h) {
    (void)window;
    if (w) *w = 800;
    if (h) *h = 600;
}

void sbgl_os_PollEvents(sbgl_Window* window) {
    (void)window;
}

bool sbgl_os_IsWindowFocused(sbgl_Window* window) {
    (void)window;
    return true;
}

void sbgl_os_SetCursorVisible(sbgl_Window* window, bool visible) {
    (void)window; (void)visible;
}

void sbgl_os_SetCursorLocked(sbgl_Window* window, bool locked) {
    (void)window; (void)locked;
}

/* Mock implementations for graphics HAL. */

sbgl_GfxContext* sbgl_gfx_Init(sbgl_Window* window, struct SblArena* arena) {
    (void)window; (void)arena;
    return (sbgl_GfxContext*)0x1;
}

void sbgl_gfx_Shutdown(sbgl_GfxContext* gfx) {
    (void)gfx;
}

bool sbgl_gfx_BeginFrame(sbgl_GfxContext* gfx, float r, float g, float b, float a) {
    (void)gfx; (void)r; (void)g; (void)b; (void)a;
    return true;
}

void sbgl_gfx_EndFrame(sbgl_GfxContext* gfx) {
    (void)gfx;
}

void sbgl_gfx_DeviceWaitIdle(sbgl_GfxContext* gfx) {
    (void)gfx;
}

float sbgl_gfx_GetGpuTime(sbgl_GfxContext* gfx) {
    (void)gfx;
    return 0.5f;
}

sbgl_Buffer sbgl_gfx_CreateBuffer(sbgl_GfxContext* ctx, sbgl_BufferUsage usage, size_t size, const void* data) {
    (void)ctx; (void)usage; (void)size; (void)data;
    return (sbgl_Buffer)1;
}

void sbgl_gfx_DestroyBuffer(sbgl_GfxContext* ctx, sbgl_Buffer buffer) {
    (void)ctx; (void)buffer;
}

sbgl_Shader sbgl_gfx_LoadShader(sbgl_GfxContext* ctx, sbgl_ShaderStage stage, const uint32_t* bytecode, size_t size) {
    (void)ctx; (void)stage; (void)bytecode; (void)size;
    return (sbgl_Shader)1;
}

void sbgl_gfx_DestroyShader(sbgl_GfxContext* ctx, sbgl_Shader shader) {
    (void)ctx; (void)shader;
}

sbgl_Pipeline sbgl_gfx_CreatePipeline(sbgl_GfxContext* ctx, const sbgl_PipelineConfig* config) {
    (void)ctx; (void)config;
    return (sbgl_Pipeline)1;
}

void sbgl_gfx_DestroyPipeline(sbgl_GfxContext* ctx, sbgl_Pipeline pipeline) {
    (void)ctx; (void)pipeline;
}

void sbgl_gfx_BindPipeline(sbgl_GfxContext* ctx, sbgl_Pipeline pipeline) {
    (void)ctx; (void)pipeline;
}

void sbgl_gfx_BindBuffer(sbgl_GfxContext* ctx, sbgl_Buffer buffer, sbgl_BufferUsage usage) {
    (void)ctx; (void)buffer; (void)usage;
}

void sbgl_gfx_Draw(sbgl_GfxContext* ctx, uint32_t vertexCount, uint32_t firstVertex) {
    (void)ctx; (void)vertexCount; (void)firstVertex;
}

void sbgl_gfx_DrawIndexed(sbgl_GfxContext* ctx, uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset) {
    (void)ctx; (void)indexCount; (void)firstIndex; (void)vertexOffset;
}

void sbgl_gfx_DrawIndirect(
	sbgl_GfxContext* ctx,
	sbgl_Buffer buffer,
	size_t offset,
	uint32_t drawCount
) {
	(void)ctx;
	(void)buffer;
	(void)offset;
	(void)drawCount;
}

sbgl_GfxTransientAllocation
sbgl_gfx_AllocateTransient(sbgl_GfxContext* ctx, size_t size, uint32_t alignment) {
	(void)ctx;
	(void)alignment;
	sbgl_GfxTransientAllocation alloc = { .buffer = (sbgl_Buffer)1,
										  .offset = 0,
										  .size = (uint32_t)size,
										  .mapped = malloc(size),
										  .deviceAddress = 0x2000 };
	return alloc;
}

uint64_t sbgl_gfx_GetBufferDeviceAddress(sbgl_GfxContext* ctx, sbgl_Buffer buffer) {
    (void)ctx; (void)buffer;
    return 0x1000;
}

void sbgl_gfx_DestroyBufferDeferred(sbgl_GfxContext* ctx, sbgl_Buffer buffer) {
    (void)ctx; (void)buffer;
}

void sbgl_gfx_PushConstants(sbgl_GfxContext* ctx, size_t size, const void* data) {
    (void)ctx; (void)size; (void)data;
}

/* Internal logging mock. */
void sbgl_internal_log(sbgl_LogLevel level, const char* message) {
    (void)level; (void)message;
}

/* We include sort and batcher here instead of mocking them. */
#include "sbgl_sort.c"
#include "sbgl_batcher.c"

/* Include the implementation to test internal state. */
#define SBL_ARENA_IMPLEMENTATION
#include "sbgl_core.c"

static void test_flags_refactor(void) {
    printf("Testing flags refactor...\n");
    
    sbgl_InitResult res = sbgl_Init(800, 600, "Test");
    assert(res.error == SBGL_SUCCESS);
    sbgl_Context* ctx = res.ctx;
    sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

    /* Testing initial state */
    if (inner->state.isIdle != 1 || inner->state.isDrawing != 0) {
        printf("Initial state mismatch\n");
        assert(0);
    }
    
    sbgl_BeginDrawing(ctx);
    if (inner->state.isDrawing != 1) {
        printf("Drawing state mismatch after BeginDrawing\n");
        assert(0);
    }
    
    sbgl_EndDrawing(ctx);
    if (inner->state.isDrawing != 0 || inner->state.isIdle != 0) {
        printf("State mismatch after EndDrawing\n");
        assert(0);
    }
    
    sbgl_DeviceWaitIdle(ctx);
    assert(inner->state.isIdle == 1);

    sbgl_Shutdown(ctx);
    printf("PASS: test_flags_refactor\n");
}

int main(void) {
    test_flags_refactor();
    return 0;
}
