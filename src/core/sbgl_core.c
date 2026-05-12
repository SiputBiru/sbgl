#include "sbgl.h"
#include "sbgl_types.h"

#define SBL_ARENA_IMPLEMENTATION
#include "core/sbl_arena.h"

#include "backend/sbgl_graphics_hal.h"
#include "core/sbgl_batcher.h"
#include "core/sbgl_internal_log.h"
#include "core/sbgl_platform.h"
#include "core/sbgl_sort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * @brief Internal storage for draw packets awaiting submission.
 *
 * The render queue manages a contiguous array of draw packets that are
 * sorted by their sort keys to minimize GPU state transitions during
 * command recording.
 */
struct sbgl_RenderQueue {
	sbgl_DrawPacket* packets;	  /**< Contiguous array of draw commands. */
	sbgl_InstanceData* instances; /**< Per-instance data (transforms, color). */
	uint32_t count;				  /**< Number of active packets in the queue. */
	uint32_t capacity;			  /**< Total number of packets the queue can hold. */
	struct SblArena* arena;		  /**< Arena used for the packet array allocation. */
};

/**
 * @brief Internal state for the engine context.
 *
 * This structure is hidden from the public API (opaque). It holds the
 * persistent memory arena, the native window handle, and the current
 * clear color state.
 */
typedef struct {
	SblArena arena;			 /**< Persistent memory for the lifetime of the context. */
	SblArena transientArena; /**< Memory reset every frame. */
	sbgl_Window* window;	 /**< Handle to the native OS window. */
	sbgl_GfxContext* gfx;	 /**< Handle to the graphics backend context. */
	float clearColor[4];	 /**< Current RGBA clear color. */
	struct {
		uint32_t isDrawing : 1;	  /**< Internal flag to track if we are in a render pass. */
		uint32_t hasStartedFrame : 1; /**< Internal flag to track if BeginFrame was called. */
		uint32_t isIdle : 1;	  /**< Internal flag to track if GPU is idle. */
		uint32_t wasFocused : 1;  /**< Tracking focus state for re-locking. */
		uint32_t mouseLocked : 1; /**< Add this for future use. */
	} state;
	sbgl_InputState input;	  /**< Physical input state tracking. */
	sbgl_MouseMode mouseMode; /**< Current intended mouse behavior. */
	sbgl_Telemetry lastFrame;
	sbgl_Telemetry currentFrame;
	uint64_t frameCount;
	uint64_t frameStartTicks;
	uint64_t sortStartTicks;
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
	sbl_arena_init(&inner->transientArena, 16 * 1024 * 1024);
	inner->clearColor[0] = 0.0f;
	inner->clearColor[1] = 0.0f;
	inner->clearColor[2] = 0.0f;
	inner->clearColor[3] = 0.0f;
	inner->state.isIdle = 1;
	inner->mouseMode = SBGL_MOUSE_MODE_NORMAL;
	inner->state.wasFocused = 0;

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

	sbgl_internal_log(SBGL_LOG_INFO, "Initiating shutdown...");

	if (!inner->state.isIdle) {
		/* If the GPU is still processing commands, the system performs a blocking wait
		   to ensure all resources can be safely released. */
		sbgl_DeviceWaitIdle(ctx);
	}

	if (inner->gfx)
		sbgl_gfx_Shutdown(inner->gfx);
	if (inner->window)
		sbgl_os_DestroyWindow(inner->window);

	sbl_arena_free(&inner->transientArena);
	sbl_arena_free(&inner->arena);
}

bool sbgl_WindowShouldClose(sbgl_Context* ctx) {
	if (!ctx || !ctx->inner)
		return true;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	return sbgl_os_WindowShouldClose(inner->window);
}

double sbgl_GetTime(sbgl_Context* ctx) {
	if (!ctx || !ctx->inner)
		return 0.0;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	return (double)sbgl_os_GetPerfCount(inner->window) / (double)sbgl_os_GetPerfFreq(inner->window);
}

void sbgl_GetWindowSize(sbgl_Context* ctx, int* w, int* h) {
	if (w)
		*w = 0;
	if (h)
		*h = 0;
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_os_GetWindowSize(inner->window, w, h);
}
void sbgl_BeginDrawing(sbgl_Context* ctx) {
    if (!ctx || !ctx->inner) return;
    sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

    // Reset compute handle to prevent cross-phase push constant delivery
    sbgl_gfx_BindComputePipeline(inner->gfx, SBGL_INVALID_HANDLE);

    if (!inner->state.hasStartedFrame) {
		sbgl_os_PollEvents(inner->window);

		// Detect focus transitions to re-apply mouse locking if necessary.
		bool currentlyFocused = sbgl_os_IsWindowFocused(inner->window);
		if (currentlyFocused && !inner->state.wasFocused) {
			if (inner->mouseMode == SBGL_MOUSE_MODE_CAPTURED) {
				sbgl_os_SetCursorVisible(inner->window, false);
				sbgl_os_SetCursorLocked(inner->window, true);
			}
		}
		inner->state.wasFocused = currentlyFocused;

		inner->frameStartTicks = sbgl_os_GetPerfCount(inner->window);

		// Performance counters are reset at the start of every frame
		inner->currentFrame.draw_calls = 0;
		inner->currentFrame.instance_count = 0;
		inner->currentFrame.cpu_sort_time = 0.0f;

		if (sbgl_gfx_BeginFrame(inner->gfx)) {
			inner->state.hasStartedFrame = 1;
			
			/* GPU performance data for the previous frame in this slot is now guaranteed 
			   to be ready. Telemetry is skipped for the very first frames to allow 
			   the pipeline to prime and avoid uninitialized query errors. */
			if (inner->frameCount > 2) {
				inner->currentFrame.gpu_render_time = sbgl_gfx_GetGpuTime(inner->gfx);
			}
		} else {
			ctx->result = SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
			return;
		}
	}

	sbgl_gfx_BeginRenderPass(
		inner->gfx,
		inner->clearColor[0],
		inner->clearColor[1],
		inner->clearColor[2],
		inner->clearColor[3]
	);
	inner->state.isDrawing = 1;
	ctx->result = SBGL_SUCCESS;
}

void sbgl_EndDrawing(sbgl_Context* ctx) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

	if (inner->state.isDrawing) {
		sbgl_gfx_EndRenderPass(inner->gfx);
		inner->state.isDrawing = 0;
	}

	if (inner->state.hasStartedFrame) {
		sbgl_gfx_EndFrame(inner->gfx);
		inner->state.hasStartedFrame = 0;
		inner->state.isIdle = 0;
		inner->frameCount++;

		uint64_t frameEndTicks = sbgl_os_GetPerfCount(inner->window);
		inner->currentFrame.cpu_frame_time =
			(float)((double)(frameEndTicks - inner->frameStartTicks) * 1000.0 / (double)sbgl_os_GetPerfFreq(inner->window));

		// The completed telemetry data is archived for user retrieval
		inner->lastFrame = inner->currentFrame;
	}

	// Reset pressed states for next frame
	memset(inner->input.keysPressed, 0, sizeof(inner->input.keysPressed));
	ctx->result = SBGL_SUCCESS;
}

void sbgl_BeginCompute(sbgl_Context* ctx) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

	// Reset graphics handle to prevent cross-phase push constant delivery
	sbgl_gfx_BindPipeline(inner->gfx, SBGL_INVALID_HANDLE);

	/* If a frame has not yet been initiated, the system handles event polling,
	   focus management, and starts the GPU command stream. */
	if (!inner->state.hasStartedFrame) {
		sbgl_os_PollEvents(inner->window);

		bool currentlyFocused = sbgl_os_IsWindowFocused(inner->window);
		if (currentlyFocused && !inner->state.wasFocused) {
			if (inner->mouseMode == SBGL_MOUSE_MODE_CAPTURED) {
				sbgl_os_SetCursorVisible(inner->window, false);
				sbgl_os_SetCursorLocked(inner->window, true);
			}
		}
		inner->state.wasFocused = currentlyFocused;

		inner->frameStartTicks = sbgl_os_GetPerfCount(inner->window);

		inner->currentFrame.draw_calls = 0;
		inner->currentFrame.instance_count = 0;
		inner->currentFrame.cpu_sort_time = 0.0f;

		if (sbgl_gfx_BeginFrame(inner->gfx)) {
			inner->state.hasStartedFrame = 1;
			if (inner->frameCount > 2) {
				inner->currentFrame.gpu_render_time = sbgl_gfx_GetGpuTime(inner->gfx);
			}
		} else {
			ctx->result = SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
			return;
		}
	}
	ctx->result = SBGL_SUCCESS;
}

void sbgl_EndCompute(sbgl_Context* ctx) {
	/* The compute phase finalize is currently a no-op at the core level,
	   as command buffer submission is deferred until the main frame end. */
	(void)ctx;
}

void sbgl_DeviceWaitIdle(sbgl_Context* ctx) {
	if (!ctx || !ctx->inner)
		return;

	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_DeviceWaitIdle(inner->gfx);
	inner->state.isIdle = 1;
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

void sbgl_SetMouseMode(sbgl_Context* ctx, sbgl_MouseMode mode) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	inner->mouseMode = mode;

	bool visible = (mode == SBGL_MOUSE_MODE_NORMAL);
	bool locked = (mode == SBGL_MOUSE_MODE_CAPTURED);

	sbgl_os_SetCursorVisible(inner->window, visible);
	sbgl_os_SetCursorLocked(inner->window, locked);
	ctx->result = SBGL_SUCCESS;
}

sbgl_Telemetry sbgl_GetTelemetry(sbgl_Context* ctx) {
	if (!ctx || !ctx->inner) {
		return (sbgl_Telemetry){ 0 };
	}
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	return inner->lastFrame;
}

sbgl_Buffer
sbgl_CreateBuffer(sbgl_Context* ctx, sbgl_BufferUsage usage, size_t size, const void* data) {
	if (!ctx || !ctx->inner)
		return SBGL_INVALID_HANDLE;

	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_Buffer res = sbgl_gfx_CreateBuffer(inner->gfx, usage, size, data);

	ctx->result =
		(res != SBGL_INVALID_HANDLE) ? SBGL_SUCCESS : SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
	return res;
}

void sbgl_DestroyBuffer(sbgl_Context* ctx, sbgl_Buffer buffer) {
	if (!ctx || !ctx->inner)
		return;

	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

	if (!inner->state.isIdle) {
		sbgl_internal_log(
			SBGL_LOG_WARN,
			"Buffer destroyed while GPU is busy! Missing sbgl_DeviceWaitIdle."
		);

		ctx->result = SBGL_ERROR_DEVICE_BUSY;

		// Force a wait to safely destroy it anyway and prevent driver crash
		sbgl_DeviceWaitIdle(ctx);
	}

	sbgl_gfx_DestroyBuffer(inner->gfx, buffer);
	ctx->result = SBGL_SUCCESS;
}

void sbgl_FillBuffer(sbgl_Context* ctx, sbgl_Buffer buffer, size_t offset, size_t size, uint32_t value) {
	if (!ctx || !ctx->inner)
		return;

	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_FillBuffer(inner->gfx, buffer, offset, size, value);
	ctx->result = SBGL_SUCCESS;
}

uint32_t sbgl_GetFrameIndex(sbgl_Context* ctx) {
	if (!ctx || !ctx->inner)
		return 0;

	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	return sbgl_gfx_GetFrameIndex(inner->gfx);
}

void* sbgl_MapBuffer(sbgl_Context* ctx, sbgl_Buffer buffer) {
	/* The system maps the specified GPU buffer into the CPU's address space,
	   allowing for direct memory access. This is essential for reading back
	   results from compute shaders or updating dynamic vertex data. */
	if (!ctx || !ctx->inner)
		return NULL;

	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	void* ptr = sbgl_gfx_MapBuffer(inner->gfx, buffer);

	ctx->result = (ptr != NULL) ? SBGL_SUCCESS : SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
	return ptr;
}

void sbgl_UnmapBuffer(sbgl_Context* ctx, sbgl_Buffer buffer) {
	/* The mapping between the CPU and GPU buffer is released, finalizing any
	   pending memory writes and ensuring synchronization for subsequent GPU operations. */
	if (!ctx || !ctx->inner)
		return;

	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_UnmapBuffer(inner->gfx, buffer);
	ctx->result = SBGL_SUCCESS;
}

sbgl_Shader
sbgl_LoadShader(sbgl_Context* ctx, sbgl_ShaderStage stage, const uint32_t* bytecode, size_t size) {
	if (!ctx || !ctx->inner)
		return SBGL_INVALID_HANDLE;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_Shader res = sbgl_gfx_LoadShader(inner->gfx, stage, bytecode, size);
	ctx->result =
		(res != SBGL_INVALID_HANDLE) ? SBGL_SUCCESS : SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
	return res;
}

sbgl_Shader
sbgl_LoadShaderFromFile(sbgl_Context* ctx, sbgl_ShaderStage stage, const char* filename) {
	FILE* file = fopen(filename, "rb");
	if (!file) {
		char fallback[256];
		// Try examples/shaders/ prefix
		snprintf(fallback, sizeof(fallback), "examples/%s", filename);
		file = fopen(fallback, "rb");
		if (!file) {
			// Try build/examples/ prefix
			snprintf(fallback, sizeof(fallback), "build/examples/%s", filename);
			file = fopen(fallback, "rb");
			if (!file) {
				sbgl_internal_log(SBGL_LOG_ERROR, "Failed to open shader file.");
				return SBGL_INVALID_HANDLE;
			}
		}
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	uint32_t* buffer = malloc(size);
	if (!buffer) {
		fclose(file);
		return SBGL_INVALID_HANDLE;
	}

	size_t read = fread(buffer, 1, size, file);
	fclose(file);

	if (read != size) {
		free(buffer);
		return SBGL_INVALID_HANDLE;
	}

	sbgl_Shader shader = sbgl_LoadShader(ctx, stage, buffer, size);
	free(buffer);
	return shader;
}

void sbgl_DestroyShader(sbgl_Context* ctx, sbgl_Shader shader) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

	if (!inner->state.isIdle) {
		sbgl_internal_log(
			SBGL_LOG_ERROR,
			"Destroy Shader called while GPU is busy! Missing sbgl_DeviceWaitIdle."
		);

		ctx->result = SBGL_ERROR_DEVICE_BUSY;

		// Force a wait to safely destroy it anyway and prevent driver crash
		sbgl_DeviceWaitIdle(ctx);
	}

	sbgl_gfx_DestroyShader(inner->gfx, shader);
	ctx->result = SBGL_SUCCESS;
}

sbgl_Pipeline sbgl_CreatePipeline(sbgl_Context* ctx, const sbgl_PipelineConfig* config) {
	if (!ctx || !ctx->inner)
		return SBGL_INVALID_HANDLE;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_Pipeline res = sbgl_gfx_CreatePipeline(inner->gfx, config);
	ctx->result =
		(res != SBGL_INVALID_HANDLE) ? SBGL_SUCCESS : SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
	return res;
}

void sbgl_DestroyPipeline(sbgl_Context* ctx, sbgl_Pipeline pipeline) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

	if (!inner->state.isIdle) {
		sbgl_internal_log(
			SBGL_LOG_ERROR,
			"Destroy Pipeline called while GPU is busy! Missing sbgl_DeviceWaitIdle."
		);

		ctx->result = SBGL_ERROR_DEVICE_BUSY;

		// Force a wait to safely destroy it anyway and prevent driver crash
		sbgl_DeviceWaitIdle(ctx);
	}

	sbgl_gfx_DestroyPipeline(inner->gfx, pipeline);
	ctx->result = SBGL_SUCCESS;
}

sbgl_ComputePipeline sbgl_CreateComputePipeline(sbgl_Context* ctx, sbgl_Shader shader) {
	if (!ctx || !ctx->inner)
		return SBGL_INVALID_HANDLE;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_ComputePipeline res = sbgl_gfx_CreateComputePipeline(inner->gfx, shader);
	ctx->result =
		(res != SBGL_INVALID_HANDLE) ? SBGL_SUCCESS : SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
	return res;
}

void sbgl_DestroyComputePipeline(sbgl_Context* ctx, sbgl_ComputePipeline pipeline) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

	if (!inner->state.isIdle) {
		sbgl_internal_log(
			SBGL_LOG_ERROR,
			"Destroy Compute Pipeline called while GPU is busy! Missing sbgl_DeviceWaitIdle."
		);

		ctx->result = SBGL_ERROR_DEVICE_BUSY;
		sbgl_DeviceWaitIdle(ctx);
	}

	sbgl_gfx_DestroyComputePipeline(inner->gfx, pipeline);
	ctx->result = SBGL_SUCCESS;
}

void sbgl_BindComputePipeline(sbgl_Context* ctx, sbgl_ComputePipeline pipeline) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_BindComputePipeline(inner->gfx, pipeline);
	ctx->result = SBGL_SUCCESS;
}

void sbgl_DispatchCompute(sbgl_Context* ctx, uint32_t x, uint32_t y, uint32_t z) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_DispatchCompute(inner->gfx, x, y, z);
	ctx->result = SBGL_SUCCESS;
}

void sbgl_MemoryBarrier(sbgl_Context* ctx, sbgl_BarrierType type) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_MemoryBarrier(inner->gfx, type);
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

uint64_t sbgl_GetBufferDeviceAddress(sbgl_Context* ctx, sbgl_Buffer buffer) {
	if (!ctx || !ctx->inner)
		return 0;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	uint64_t addr = sbgl_gfx_GetBufferDeviceAddress(inner->gfx, buffer);
	ctx->result = (addr != 0) ? SBGL_SUCCESS : SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED;
	return addr;
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

void sbgl_DrawIndirect(
	sbgl_Context* ctx,
	sbgl_Buffer buffer,
	size_t offset,
	uint32_t drawCount
) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_DrawIndirect(inner->gfx, buffer, offset, drawCount);
	ctx->result = SBGL_SUCCESS;
}

void sbgl_PushConstants(sbgl_Context* ctx, size_t size, const void* data) {
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;
	sbgl_gfx_PushConstants(inner->gfx, size, data);
	ctx->result = SBGL_SUCCESS;
}

// --- Automated Batching API ---

sbgl_RenderQueue* sbgl_CreateRenderQueue(sbgl_Context* ctx, SblArena* arena) {
	if (!ctx || !arena) {
		return NULL;
	}

	// Allocate the queue structure from the provided arena
	sbgl_RenderQueue* queue = SBL_ARENA_PUSH_STRUCT_ZERO(arena, sbgl_RenderQueue);
	if (!queue) {
		ctx->result = SBGL_ERROR_OUT_OF_MEMORY;
		return NULL;
	}

	// Initialize the queue with a default capacity of 16,384 packets
	/* Initialize the queue with a high capacity to support massive batching (e.g., voxels). */
	queue->capacity = 131072;
	queue->count = 0;
	queue->arena = arena;
	queue->packets = SBL_ARENA_PUSH_ARRAY_ZERO(arena, sbgl_DrawPacket, queue->capacity);
	queue->instances = SBL_ARENA_PUSH_ARRAY_ZERO(arena, sbgl_InstanceData, queue->capacity);

	if (!queue->packets || !queue->instances) {
		ctx->result = SBGL_ERROR_OUT_OF_MEMORY;
		return NULL;
	}

	ctx->result = SBGL_SUCCESS;
	return queue;
}

void sbgl_SubmitDraw(
	sbgl_RenderQueue* queue,
	uint32_t mesh,
	uint32_t material,
	uint32_t blendMode,
	uint32_t sidedness,
	uint32_t tags,
	sbgl_SortKey key,
	const sbgl_InstanceData* data
) {
	if (!queue) {
		return;
	}

	// Verify the queue has not reached maximum capacity
	if (queue->count >= queue->capacity) {
		return;
	}

	// The system appends the draw packet to the contiguous array
	uint32_t index = queue->count++;
	sbgl_DrawPacket* packet = &queue->packets[index];
	packet->key = key;
	packet->header = SBGL_PACK_HEADER(mesh, material, blendMode, sidedness, tags);

	// Per-instance data is copied into the parallel array
	if (data) {
		queue->instances[index] = *data;
	} else {
		// An identity transform and white color are used if no data is provided
		memset(&queue->instances[index], 0, sizeof(sbgl_InstanceData));
		queue->instances[index].transform = sbgl_Mat4Identity();
		queue->instances[index].color = sbgl_Vec4Set(1, 1, 1, 1);
	}
}

void sbgl_RenderQueues(
	sbgl_Context* ctx,
	sbgl_RenderQueue** queues,
	uint32_t queueCount,
	const sbgl_Mat4* viewProj
) {
	sbgl_RenderQueuesEx(ctx, queues, queueCount, viewProj, 0);
}

void sbgl_RenderQueuesEx(
	sbgl_Context* ctx,
	sbgl_RenderQueue** queues,
	uint32_t queueCount,
	const sbgl_Mat4* viewProj,
	uint64_t userAddress
) {
	if (!ctx || !ctx->inner || !queues || queueCount == 0) {
		return;
	}

	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

	// Total packet count is calculated across all queues to determine buffer requirements
	uint32_t totalPackets = 0;
	for (uint32_t i = 0; i < queueCount; ++i) {
		if (queues[i]) {
			totalPackets += queues[i]->count;
		}
	}

	if (totalPackets == 0) {
		return;
	}

	// The transient arena is reset for this frame's batching work
	sbl_arena_reset(&inner->transientArena);

	inner->sortStartTicks = sbgl_os_GetPerfCount(inner->window);

	// Temporary host memory is allocated for merging and sorting from the transient arena
	sbgl_DrawPacket* mergedPackets =
		SBL_ARENA_PUSH_ARRAY(&inner->transientArena, sbgl_DrawPacket, totalPackets);
	sbgl_InstanceData* mergedInstances =
		SBL_ARENA_PUSH_ARRAY(&inner->transientArena, sbgl_InstanceData, totalPackets);
	sbgl_SortKey* keys = SBL_ARENA_PUSH_ARRAY(&inner->transientArena, sbgl_SortKey, totalPackets);
	uint32_t* indices = SBL_ARENA_PUSH_ARRAY(&inner->transientArena, uint32_t, totalPackets);

	if (!mergedPackets || !mergedInstances || !keys || !indices) {
		ctx->result = SBGL_ERROR_OUT_OF_MEMORY;
		return;
	}

	// Packets and instance data are merged from all queues
	uint32_t current = 0;
	for (uint32_t i = 0; i < queueCount; ++i) {
		if (!queues[i])
			continue;
		for (uint32_t j = 0; j < queues[i]->count; ++j) {
			mergedPackets[current] = queues[i]->packets[j];
			mergedInstances[current] = queues[i]->instances[j];
			keys[current] = mergedPackets[current].key;
			indices[current] = current;
			current++;
		}
		// The queue is cleared for the next frame
		queues[i]->count = 0;
	}

	// A stable radix sort is performed on the packets based on sort keys
	sbgl_SortKey* temp_keys =
		SBL_ARENA_PUSH_ARRAY(&inner->transientArena, sbgl_SortKey, totalPackets);
	uint32_t* temp_indices = SBL_ARENA_PUSH_ARRAY(&inner->transientArena, uint32_t, totalPackets);
	if (!temp_keys || !temp_indices) {
		ctx->result = SBGL_ERROR_OUT_OF_MEMORY;
		return;
	}

	sbgl_radix_sort(keys, indices, totalPackets, temp_keys, temp_indices);

	// Packets and instances are reordered according to the sorted indices
	sbgl_DrawPacket* sortedPackets =
		SBL_ARENA_PUSH_ARRAY(&inner->transientArena, sbgl_DrawPacket, totalPackets);
	sbgl_InstanceData* sortedInstances =
		SBL_ARENA_PUSH_ARRAY(&inner->transientArena, sbgl_InstanceData, totalPackets);
	if (!sortedPackets || !sortedInstances) {
		ctx->result = SBGL_ERROR_OUT_OF_MEMORY;
		return;
	}

	for (uint32_t i = 0; i < totalPackets; ++i) {
		sortedPackets[i] = mergedPackets[indices[i]];
		sortedInstances[i] = mergedInstances[indices[i]];
	}

	// Sorted packets are baked into optimized indirect draw commands
	sbgl_IndirectCommand* commands =
		SBL_ARENA_PUSH_ARRAY(&inner->transientArena, sbgl_IndirectCommand, totalPackets);
	if (!commands) {
		ctx->result = SBGL_ERROR_OUT_OF_MEMORY;
		return;
	}

	uint32_t commandCount = sbgl_bake_commands(sortedPackets, totalPackets, commands, totalPackets);

	uint64_t sortEndTicks = sbgl_os_GetPerfCount(inner->window);
	inner->currentFrame.cpu_sort_time +=
		(float)((double)(sortEndTicks - inner->sortStartTicks) * 1000.0 / (double)sbgl_os_GetPerfFreq(inner->window));

	inner->currentFrame.draw_calls += commandCount;
	inner->currentFrame.instance_count += totalPackets;

	if (commandCount > 0) {
		// Allocate transient GPU space for packed instance data
		sbgl_GfxTransientAllocation instanceAlloc = sbgl_gfx_AllocateTransient(
			inner->gfx,
			sizeof(sbgl_InstanceData) * totalPackets,
			256 // Storage buffer alignment
		);

		if (instanceAlloc.buffer != SBGL_INVALID_HANDLE) {
			memcpy(instanceAlloc.mapped, sortedInstances, sizeof(sbgl_InstanceData) * totalPackets);

			// Retrieve BDA address and update push constants
			uint64_t bdaAddress = instanceAlloc.deviceAddress;

			struct {
				sbgl_Mat4 viewProj;
				uint64_t instanceAddress;
				uint64_t userAddress;
			} pc;
			pc.viewProj = viewProj ? *viewProj : sbgl_Mat4Identity();
			pc.instanceAddress = bdaAddress;
			pc.userAddress = userAddress;

			sbgl_gfx_PushConstants(inner->gfx, sizeof(pc), &pc);

			// Allocate transient GPU space for baked commands and dispatch MDI
			sbgl_GfxTransientAllocation indirectAlloc = sbgl_gfx_AllocateTransient(
				inner->gfx,
				sizeof(sbgl_IndirectCommand) * commandCount,
				16 // Indirect buffer alignment
			);

			if (indirectAlloc.buffer != SBGL_INVALID_HANDLE) {
				memcpy(indirectAlloc.mapped, commands, sizeof(sbgl_IndirectCommand) * commandCount);
				sbgl_gfx_DrawIndirect(
					inner->gfx,
					indirectAlloc.buffer,
					indirectAlloc.offset,
					commandCount
				);
			}
		}
	}

	ctx->result = SBGL_SUCCESS;
}
