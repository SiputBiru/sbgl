#include "sbgl.h"
#define SBL_ARENA_IMPLEMENTATION
#include "backend/sbgl_graphics_hal.h"
#include "core/sbgl_platform.h"
#include "core/sbl_arena.h"

/**
 * @brief Internal state for the engine context.
 *
 * This structure is hidden from the public API (opaque). It holds the
 * persistent memory arena, the native window handle, and the current
 * clear color state.
 */
typedef struct {
	SblArena arena;			 /**< Persistent memory for the lifetime of the context. */
	sbgl_Window* window;	 /**< Handle to the native OS window. */
	sbgl_GfxContext* gfx;	 /**< Handle to the graphics backend context. */
	float clearColor[4];	 /**< Current RGBA clear color. */
	bool isDrawing;			 /**< Internal flag to track frame acquisition success. */
	sbgl_InputState input;	 /**< Physical input state tracking. */
} sbgl_InternalContext;

sbgl_InitResult sbgl_Init(int w, int h, const char* title) {
	sbgl_InitResult res = { .ctx = NULL, .error = SBGL_SUCCESS };

	SblArena main_arena;
	if (!sbl_arena_init(&main_arena, 4 * 1024 * 1024)) { // 4MB default
		res.error = SBGL_ERROR_OUT_OF_MEMORY;
		return res;
	}

	sbgl_Context* ctx = SBL_ARENA_PUSH_STRUCT_ZERO(&main_arena, sbgl_Context);
	if (!ctx) {
		sbl_arena_free(&main_arena);
		res.error = SBGL_ERROR_OUT_OF_MEMORY;
		return res;
	}

	sbgl_InternalContext* inner = SBL_ARENA_PUSH_STRUCT_ZERO(&main_arena, sbgl_InternalContext);
	if (!inner) {
		sbl_arena_free(&main_arena);
		res.error = SBGL_ERROR_OUT_OF_MEMORY;
		return res;
	}

	inner->arena = main_arena;
	inner->clearColor[0] = 0.1f;
	inner->clearColor[1] = 0.2f;
	inner->clearColor[2] = 0.3f;
	inner->clearColor[3] = 1.0f;

	ctx->inner = inner;
	ctx->result = SBGL_SUCCESS;

	inner->window = sbgl_os_CreateWindow(&inner->arena, &inner->input, w, h, title);
	if (!inner->window) {
		res.error = SBGL_ERROR_WINDOW_CREATION_FAILED;
		sbl_arena_free(&inner->arena);
		return res;
	}

	inner->gfx = sbgl_gfx_Init(inner->window, &inner->arena);
	if (!inner->gfx) {
		res.error = SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
		sbgl_os_DestroyWindow(inner->window);
		sbl_arena_free(&inner->arena);
		return res;
	}

	res.ctx = ctx;
	return res;
}

void sbgl_Shutdown(sbgl_Context* ctx) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

	if (inner->gfx)
		sbgl_gfx_Shutdown(inner->gfx);
	if (inner->window)
		sbgl_os_DestroyWindow(inner->window);

	sbl_arena_free(&inner->arena);
}

bool sbgl_WindowShouldClose(sbgl_Context* ctx) {
	if (!ctx || !ctx->inner)
		return true;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	return sbgl_os_WindowShouldClose(inner->window);
}

void sbgl_GetWindowSize(sbgl_Context* ctx, int* w, int* h) {
	if (w) *w = 0;
	if (h) *h = 0;
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_os_GetWindowSize(inner->window, w, h);
}

void sbgl_BeginDrawing(sbgl_Context* ctx) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

	sbgl_os_PollEvents(inner->window);

	inner->isDrawing = sbgl_gfx_BeginFrame(
		inner->gfx,
		inner->clearColor[0],
		inner->clearColor[1],
		inner->clearColor[2],
		inner->clearColor[3]
	);

	ctx->result = inner->isDrawing ? SBGL_SUCCESS : SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
}

void sbgl_EndDrawing(sbgl_Context* ctx) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

	if (inner->isDrawing) {
		sbgl_gfx_EndFrame(inner->gfx);
		inner->isDrawing = false;
	}

	// Reset pressed states for next frame
	memset(inner->input.keysPressed, 0, sizeof(inner->input.keysPressed));
	ctx->result = SBGL_SUCCESS;
}

void sbgl_DeviceWaitIdle(sbgl_Context* ctx) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_DeviceWaitIdle(inner->gfx);
	ctx->result = SBGL_SUCCESS;
}

void sbgl_Clear(sbgl_Context* ctx, float r, float g, float b, float a) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	inner->clearColor[0] = r;
	inner->clearColor[1] = g;
	inner->clearColor[2] = b;
	inner->clearColor[3] = a;
	ctx->result = SBGL_SUCCESS;
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
	if (!ctx || !ctx->inner)
		return SBGL_INVALID_HANDLE;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_Buffer res = sbgl_gfx_CreateBuffer(inner->gfx, usage, size, data);
	ctx->result = (res != SBGL_INVALID_HANDLE) ? SBGL_SUCCESS : SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
	return res;
}

void sbgl_DestroyBuffer(sbgl_Context* ctx, sbgl_Buffer buffer) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_DestroyBuffer(inner->gfx, buffer);
	ctx->result = SBGL_SUCCESS;
}

sbgl_Shader
sbgl_LoadShader(sbgl_Context* ctx, sbgl_ShaderStage stage, const uint32_t* bytecode, size_t size) {
	if (!ctx || !ctx->inner)
		return SBGL_INVALID_HANDLE;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_Shader res = sbgl_gfx_LoadShader(inner->gfx, stage, bytecode, size);
	ctx->result = (res != SBGL_INVALID_HANDLE) ? SBGL_SUCCESS : SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
	return res;
}

void sbgl_DestroyShader(sbgl_Context* ctx, sbgl_Shader shader) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_DestroyShader(inner->gfx, shader);
	ctx->result = SBGL_SUCCESS;
}

sbgl_Pipeline sbgl_CreatePipeline(sbgl_Context* ctx, const sbgl_PipelineConfig* config) {
	if (!ctx || !ctx->inner)
		return SBGL_INVALID_HANDLE;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_Pipeline res = sbgl_gfx_CreatePipeline(inner->gfx, config);
	ctx->result = (res != SBGL_INVALID_HANDLE) ? SBGL_SUCCESS : SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
	return res;
}

void sbgl_DestroyPipeline(sbgl_Context* ctx, sbgl_Pipeline pipeline) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_DestroyPipeline(inner->gfx, pipeline);
	ctx->result = SBGL_SUCCESS;
}

void sbgl_BindPipeline(sbgl_Context* ctx, sbgl_Pipeline pipeline) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_BindPipeline(inner->gfx, pipeline);
	ctx->result = SBGL_SUCCESS;
}

void sbgl_BindBuffer(sbgl_Context* ctx, sbgl_Buffer buffer, sbgl_BufferUsage usage) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_BindBuffer(inner->gfx, buffer, usage);
	ctx->result = SBGL_SUCCESS;
}

void sbgl_Draw(sbgl_Context* ctx, uint32_t vertexCount, uint32_t firstVertex) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_Draw(inner->gfx, vertexCount, firstVertex);
	ctx->result = SBGL_SUCCESS;
}

void sbgl_DrawIndexed(
	sbgl_Context* ctx,
	uint32_t indexCount,
	uint32_t firstIndex,
	int32_t vertexOffset
) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_DrawIndexed(inner->gfx, indexCount, firstIndex, vertexOffset);
	ctx->result = SBGL_SUCCESS;
}

void sbgl_PushConstants(sbgl_Context* ctx, size_t size, const void* data) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_PushConstants(inner->gfx, size, data);
	ctx->result = SBGL_SUCCESS;
}
