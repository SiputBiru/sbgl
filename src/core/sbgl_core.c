#include "sbgl.h"
#define SBL_ARENA_IMPLEMENTATION
#include "backend/sbgl_graphics_hal.h"
#include "core/sbgl_platform.h"
#include "core/sbl_arena.h"
#include <stdlib.h>

/**
 * @brief Internal state for the engine context.
 *
 * This structure is hidden from the public API (opaque). It holds the
 * persistent memory arena, the native window handle, and the current
 * clear color state.
 */
typedef struct {
	SblArena arena;		   /**< Persistent memory for the lifetime of the context. */
	sbgl_Window* window;   /**< Handle to the native OS window. */
	float clearColor[4];   /**< Current RGBA clear color. */
	bool isDrawing;		   /**< Internal flag to track frame acquisition success. */
	sbgl_InputState input; /**< Physical input state tracking. */
} sbgl_InternalContext;

sbgl_InitResult sbgl_Init(int w, int h, const char* title) {
	sbgl_InitResult res = { .ctx = NULL, .error = SBGL_SUCCESS };

	// The primary arena is initialized to manage all persistent engine state
	SblArena main_arena;
	sbl_arena_init(&main_arena, 4 * 1024 * 1024); // 4MB default

	// The public context shell is allocated and zeroed from the arena
	sbgl_Context* ctx = SBL_ARENA_PUSH_STRUCT_ZERO(&main_arena, sbgl_Context);
	if (!ctx) {
		sbl_arena_free(&main_arena);
		res.error = SBGL_ERROR_OUT_OF_MEMORY;
		return res;
	}

	// Internal state tracking is likewise pushed onto the arena
	sbgl_InternalContext* inner = SBL_ARENA_PUSH_STRUCT_ZERO(&main_arena, sbgl_InternalContext);
	if (!inner) {
		sbl_arena_free(&main_arena);
		res.error = SBGL_ERROR_OUT_OF_MEMORY;
		return res;
	}

	// Ownership of the arena is transferred to the internal context for the app lifetime
	inner->arena = main_arena;
	inner->clearColor[0] = 0.1f;
	inner->clearColor[1] = 0.2f;
	inner->clearColor[2] = 0.3f;
	inner->clearColor[3] = 1.0f;

	ctx->inner = inner;
	ctx->result = SBGL_SUCCESS;
	res.ctx = ctx;

	// The native platform window is created using the same arena for its internal tracking
	inner->window = sbgl_os_CreateWindow(&inner->arena, &inner->input, w, h, title);
	if (!inner->window) {
		ctx->result = SBGL_ERROR_WINDOW_CREATION_FAILED;
		res.error = ctx->result;
		return res;
	}

	// The graphics backend is initialized with a reference to the shared arena
	if (!sbgl_gfx_Init(inner->window, &inner->arena)) {
		ctx->result = SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
		res.error = ctx->result;
		return res;
	}

	return res;
}

void sbgl_Shutdown(sbgl_Context* ctx) {
	if (!ctx)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

	sbgl_gfx_Shutdown();
	if (inner->window)
		sbgl_os_DestroyWindow(inner->window);

	// Freeing the arena automatically releases the context, internal state, and window state
	sbl_arena_free(&inner->arena);
}

bool sbgl_WindowShouldClose(sbgl_Context* ctx) {
	if (!ctx)
		return true;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	return sbgl_os_WindowShouldClose(inner->window);
}

void sbgl_GetWindowSize(sbgl_Context* ctx, int* w, int* h) {
	if (!ctx)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_os_GetWindowSize(inner->window, w, h);
}

void sbgl_BeginDrawing(sbgl_Context* ctx) {
	if (!ctx)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

	// Ensure events are polled so Wayland can map the window
	sbgl_os_PollEvents(inner->window);

	inner->isDrawing = sbgl_gfx_BeginFrame(
		inner->clearColor[0],
		inner->clearColor[1],
		inner->clearColor[2],
		inner->clearColor[3]
	);
}

void sbgl_EndDrawing(sbgl_Context* ctx) {
	if (!ctx)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

	if (inner->isDrawing) {
		sbgl_gfx_EndFrame();
		inner->isDrawing = false;
	}

	// Reset pressed states for next frame
	memset(inner->input.keysPressed, 0, sizeof(inner->input.keysPressed));
}

void sbgl_Clear(sbgl_Context* ctx, float r, float g, float b, float a) {
	if (!ctx)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	inner->clearColor[0] = r;
	inner->clearColor[1] = g;
	inner->clearColor[2] = b;
	inner->clearColor[3] = a;
}

const sbgl_InputState* sbgl_GetInputState(sbgl_Context* ctx) {
	static const sbgl_InputState dummy = { 0 };
	if (!ctx || !ctx->inner)
		return &dummy;

	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	return &inner->input;
}

sbgl_Buffer
sbgl_CreateBuffer(sbgl_Context* ctx, sbgl_BufferUsage usage, size_t size, const void* data) {
	(void)ctx; // Context check if needed
	return sbgl_gfx_CreateBuffer(usage, size, data);
}

void sbgl_DestroyBuffer(sbgl_Context* ctx, sbgl_Buffer buffer) {
	(void)ctx;
	sbgl_gfx_DestroyBuffer(buffer);
}

sbgl_Shader
sbgl_LoadShader(sbgl_Context* ctx, sbgl_ShaderStage stage, const uint32_t* bytecode, size_t size) {
	(void)ctx;
	return sbgl_gfx_LoadShader(stage, bytecode, size);
}

void sbgl_DestroyShader(sbgl_Context* ctx, sbgl_Shader shader) {
	(void)ctx;
	sbgl_gfx_DestroyShader(shader);
}

sbgl_Pipeline sbgl_CreatePipeline(sbgl_Context* ctx, const sbgl_PipelineConfig* config) {
	(void)ctx;
	return sbgl_gfx_CreatePipeline(config);
}

void sbgl_DestroyPipeline(sbgl_Context* ctx, sbgl_Pipeline pipeline) {
	(void)ctx;
	sbgl_gfx_DestroyPipeline(pipeline);
}

void sbgl_BindPipeline(sbgl_Context* ctx, sbgl_Pipeline pipeline) {
	(void)ctx;
	sbgl_gfx_BindPipeline(pipeline);
}

void sbgl_BindBuffer(sbgl_Context* ctx, sbgl_Buffer buffer, sbgl_BufferUsage usage) {
	(void)ctx;
	sbgl_gfx_BindBuffer(buffer, usage);
}

void sbgl_Draw(sbgl_Context* ctx, uint32_t vertexCount, uint32_t firstVertex) {
	(void)ctx;
	sbgl_gfx_Draw(vertexCount, firstVertex);
}

void sbgl_DrawIndexed(
	sbgl_Context* ctx,
	uint32_t indexCount,
	uint32_t firstIndex,
	int32_t vertexOffset
) {
	(void)ctx;
	sbgl_gfx_DrawIndexed(indexCount, firstIndex, vertexOffset);
}

void sbgl_PushConstants(sbgl_Context* ctx, size_t size, const void* data) {
	(void)ctx;
	sbgl_gfx_PushConstants(size, data);
}
