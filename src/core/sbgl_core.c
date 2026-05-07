#include "sbgl.h"
#include "sbgl_types.h"
#define SBL_ARENA_IMPLEMENTATION
#include "backend/sbgl_graphics_hal.h"
#include "core/sbgl_internal_log.h"
#include "core/sbgl_platform.h"
#include "core/sbl_arena.h"
#include "core/sbgl_sort.h"
#include "core/sbgl_batcher.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * @brief Internal storage for draw packets awaiting submission.
 *
 * The render queue manages a contiguous array of draw packets that are
 * sorted by their sort keys to minimize GPU state transitions during
 * command recording.
 */
struct sbgl_RenderQueue {
    sbgl_DrawPacket* packets;   /**< Contiguous array of draw commands. */
    sbgl_InstanceData* instances; /**< Per-instance data (transforms, color). */
    uint32_t count;             /**< Number of active packets in the queue. */
    uint32_t capacity;          /**< Total number of packets the queue can hold. */
    struct SblArena* arena;     /**< Arena used for the packet array allocation. */
};


/**
 * @brief Internal state for the engine context.
 *
 * This structure is hidden from the public API (opaque). It holds the
 * persistent memory arena, the native window handle, and the current
 * clear color state.
 */
typedef struct {
	SblArena arena;		  /**< Persistent memory for the lifetime of the context. */
	sbgl_Window* window;  /**< Handle to the native OS window. */
	sbgl_GfxContext* gfx; /**< Handle to the graphics backend context. */
	float clearColor[4];  /**< Current RGBA clear color. */
	bool isDrawing;		  /**< Internal flag to track frame acquisition success. */
	bool isIdle; /**< Internal flag to track if sbgl_DeviceWaitIdle was called after drawing */
	sbgl_InputState input; /**< Physical input state tracking. */
	sbgl_MouseMode mouseMode; /**< Current intended mouse behavior. */
	bool wasFocused;          /**< Tracking focus state for re-locking. */
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
	inner->clearColor[0] = 0.0f;
	inner->clearColor[1] = 0.0f;
	inner->clearColor[2] = 0.0f;
	inner->clearColor[3] = 0.0f;
	inner->isIdle = true;
	inner->mouseMode = SBGL_MOUSE_MODE_NORMAL;
	inner->wasFocused = false;

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

	if (!inner->isIdle) {
		sbgl_internal_log(
			SBGL_LOG_ERROR,
			"Shutdown called while GPU is busy! Missing sbgl_DeviceWaitIdle."
		);

		ctx->result = SBGL_ERROR_DEVICE_BUSY;

		// Force a wait to safely destroy it anyway and prevent driver crash
		sbgl_DeviceWaitIdle(ctx);
	}

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
	if (!ctx || !ctx->inner)
		return;
	sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

	sbgl_os_PollEvents(inner->window);

	// Detect focus transitions to re-apply mouse locking if necessary.
	bool currentlyFocused = sbgl_os_IsWindowFocused(inner->window);
	if (currentlyFocused && !inner->wasFocused) {
		if (inner->mouseMode == SBGL_MOUSE_MODE_CAPTURED) {
			sbgl_os_SetCursorVisible(inner->window, false);
			sbgl_os_SetCursorLocked(inner->window, true);
		}
	}
	inner->wasFocused = currentlyFocused;

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
		inner->isIdle = false;
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
	inner->isIdle = true;
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

	if (!inner->isIdle) {
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

sbgl_Shader sbgl_LoadShaderFromFile(sbgl_Context* ctx, sbgl_ShaderStage stage, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        char fallback[256];
        snprintf(fallback, sizeof(fallback), "build/examples/%s", filename);
        file = fopen(fallback, "rb");
        if (!file) {
            sbgl_internal_log(SBGL_LOG_ERROR, "Failed to open shader file.");
            return SBGL_INVALID_HANDLE;
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

	if (!inner->isIdle) {
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

	if (!inner->isIdle) {
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
    queue->capacity = 16384;
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

void sbgl_SubmitDraw(sbgl_RenderQueue* queue, uint32_t mesh, uint32_t material, sbgl_SortKey key, const sbgl_InstanceData* data) {
    if (!queue) {
        return;
    }

    // Check if the queue has reached its maximum capacity
    if (queue->count >= queue->capacity) {
        return;
    }

    // Append the draw packet to the contiguous array
    uint32_t index = queue->count++;
    sbgl_DrawPacket* packet = &queue->packets[index];
    packet->key = key;
    packet->meshId = mesh;
    packet->materialId = material;
    packet->instanceId = index;

    // Copy the per-instance data into the queue's parallel array
    if (data) {
        queue->instances[index] = *data;
    } else {
        // Use default identity if no data provided
        memset(&queue->instances[index], 0, sizeof(sbgl_InstanceData));
        queue->instances[index].transform = sbgl_Mat4Identity();
        queue->instances[index].color = sbgl_Vec4Set(1, 1, 1, 1);
    }
}

void sbgl_RenderQueues(sbgl_Context* ctx, sbgl_RenderQueue** queues, uint32_t queueCount, const sbgl_Mat4* viewProj) {
    if (!ctx || !ctx->inner || !queues || queueCount == 0) {
        return;
    }

    sbgl_InternalContext* inner = (sbgl_InternalContext*)ctx->inner;

    // Calculate total number of packets across all queues to determine buffer sizes
    uint32_t totalPackets = 0;
    for (uint32_t i = 0; i < queueCount; ++i) {
        if (queues[i]) {
            totalPackets += queues[i]->count;
        }
    }

    if (totalPackets == 0) {
        return;
    }

    // Allocate temporary host memory for merging and sorting
    sbgl_DrawPacket* mergedPackets = malloc(sizeof(sbgl_DrawPacket) * totalPackets);
    sbgl_InstanceData* mergedInstances = malloc(sizeof(sbgl_InstanceData) * totalPackets);
    sbgl_SortKey* keys = malloc(sizeof(sbgl_SortKey) * totalPackets);
    uint32_t* indices = malloc(sizeof(uint32_t) * totalPackets);

    if (!mergedPackets || !mergedInstances || !keys || !indices) {
        if (mergedPackets) free(mergedPackets);
        if (mergedInstances) free(mergedInstances);
        if (keys) free(keys);
        if (indices) free(indices);
        ctx->result = SBGL_ERROR_OUT_OF_MEMORY;
        return;
    }

    // Merge packets and instance data from all queues
    uint32_t current = 0;
    for (uint32_t i = 0; i < queueCount; ++i) {
        if (!queues[i]) continue;
        for (uint32_t j = 0; j < queues[i]->count; ++j) {
            mergedPackets[current] = queues[i]->packets[j];
            mergedInstances[current] = queues[i]->instances[j];
            keys[current] = mergedPackets[current].key;
            indices[current] = current;
            current++;
        }
        // Clear the queue for the next frame
        queues[i]->count = 0;
    }

    // Perform a stable radix sort on the packets based on their sort keys
    sbgl_radix_sort(keys, indices, totalPackets);

    // Reorder packets and instances according to the sorted indices
    sbgl_DrawPacket* sortedPackets = malloc(sizeof(sbgl_DrawPacket) * totalPackets);
    sbgl_InstanceData* sortedInstances = malloc(sizeof(sbgl_InstanceData) * totalPackets);
    if (!sortedPackets || !sortedInstances) {
        free(mergedPackets); free(mergedInstances); free(keys); free(indices);
        if (sortedPackets) free(sortedPackets);
        if (sortedInstances) free(sortedInstances);
        ctx->result = SBGL_ERROR_OUT_OF_MEMORY;
        return;
    }

    for (uint32_t i = 0; i < totalPackets; ++i) {
        sortedPackets[i] = mergedPackets[indices[i]];
        sortedInstances[i] = mergedInstances[indices[i]];
    }

    // Bake the sorted packets into optimized indirect draw commands
    sbgl_IndirectCommand* commands = malloc(sizeof(sbgl_IndirectCommand) * totalPackets);
    if (!commands) {
        free(mergedPackets); free(mergedInstances); free(keys); free(indices);
        free(sortedPackets); free(sortedInstances);
        ctx->result = SBGL_ERROR_OUT_OF_MEMORY;
        return;
    }

    uint32_t commandCount = sbgl_bake_commands(sortedPackets, totalPackets, commands, totalPackets);

    if (commandCount > 0) {
        // 1. Upload packed instance data to a GPU storage buffer
        sbgl_Buffer instanceBuffer = sbgl_gfx_CreateBuffer(
            inner->gfx,
            SBGL_BUFFER_USAGE_STORAGE,
            sizeof(sbgl_InstanceData) * totalPackets,
            sortedInstances
        );

        if (instanceBuffer != SBGL_INVALID_HANDLE) {
            // 2. Retrieve BDA address and update push constants
            uint64_t bdaAddress = sbgl_gfx_GetBufferDeviceAddress(inner->gfx, instanceBuffer);
            
            struct {
                sbgl_Mat4 viewProj;
                uint64_t instanceAddress;
            } pc;
            pc.viewProj = viewProj ? *viewProj : sbgl_Mat4Identity();
            pc.instanceAddress = bdaAddress;

            sbgl_gfx_PushConstants(inner->gfx, sizeof(pc), &pc);

            // 3. Upload baked commands and dispatch MDI
            sbgl_Buffer indirectBuffer = sbgl_gfx_CreateBuffer(
                inner->gfx, 
                SBGL_BUFFER_USAGE_INDIRECT, 
                sizeof(sbgl_IndirectCommand) * commandCount, 
                commands
            );
            
            if (indirectBuffer != SBGL_INVALID_HANDLE) {
                sbgl_gfx_DrawIndirect(inner->gfx, indirectBuffer, commandCount);
                sbgl_gfx_DestroyBufferDeferred(inner->gfx, indirectBuffer);
            }
            
            // Queue instance buffer for destruction after frame
            sbgl_gfx_DestroyBufferDeferred(inner->gfx, instanceBuffer);
        }
    }

    // Release temporary host resources
    free(mergedPackets);
    free(mergedInstances);
    free(keys);
    free(indices);
    free(sortedPackets);
    free(sortedInstances);
    free(commands);

    ctx->result = SBGL_SUCCESS;
}
