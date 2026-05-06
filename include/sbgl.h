/**
 * @file sbgl.h
 * @brief Public API for the SiputBiru Graphics Library (SBgl).
 *
 * This header defines the primary interface for window management,
 * frame lifecycle, and input handling.
 */

#ifndef SBGL_H
#define SBGL_H

#include "sbgl_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sbgl_input.h"

// --- Scancodes (Subset of physical keys) ---

#define SBGL_KEY_UNKNOWN SBGL_SCANCODE_UNKNOWN
#define SBGL_KEY_A SBGL_SCANCODE_A
#define SBGL_KEY_B SBGL_SCANCODE_B
#define SBGL_KEY_C SBGL_SCANCODE_C
#define SBGL_KEY_D SBGL_SCANCODE_D
#define SBGL_KEY_E SBGL_SCANCODE_E
#define SBGL_KEY_F SBGL_SCANCODE_F
#define SBGL_KEY_G SBGL_SCANCODE_G
#define SBGL_KEY_H SBGL_SCANCODE_H
#define SBGL_KEY_I SBGL_SCANCODE_I
#define SBGL_KEY_J SBGL_SCANCODE_J
#define SBGL_KEY_K SBGL_SCANCODE_K
#define SBGL_KEY_L SBGL_SCANCODE_L
#define SBGL_KEY_M SBGL_SCANCODE_M
#define SBGL_KEY_N SBGL_SCANCODE_N
#define SBGL_KEY_O SBGL_SCANCODE_O
#define SBGL_KEY_P SBGL_SCANCODE_P
#define SBGL_KEY_Q SBGL_SCANCODE_Q
#define SBGL_KEY_R SBGL_SCANCODE_R
#define SBGL_KEY_S SBGL_SCANCODE_S
#define SBGL_KEY_T SBGL_SCANCODE_T
#define SBGL_KEY_U SBGL_SCANCODE_U
#define SBGL_KEY_V SBGL_SCANCODE_V
#define SBGL_KEY_W SBGL_SCANCODE_W
#define SBGL_KEY_X SBGL_SCANCODE_X
#define SBGL_KEY_Y SBGL_SCANCODE_Y
#define SBGL_KEY_Z SBGL_SCANCODE_Z

#define SBGL_KEY_1 SBGL_SCANCODE_1
#define SBGL_KEY_2 SBGL_SCANCODE_2
#define SBGL_KEY_3 SBGL_SCANCODE_3
#define SBGL_KEY_4 SBGL_SCANCODE_4
#define SBGL_KEY_5 SBGL_SCANCODE_5
#define SBGL_KEY_6 SBGL_SCANCODE_6
#define SBGL_KEY_7 SBGL_SCANCODE_7
#define SBGL_KEY_8 SBGL_SCANCODE_8
#define SBGL_KEY_9 SBGL_SCANCODE_9
#define SBGL_KEY_0 SBGL_SCANCODE_0

#define SBGL_KEY_RETURN SBGL_SCANCODE_RETURN
#define SBGL_KEY_ESCAPE SBGL_SCANCODE_ESCAPE
#define SBGL_KEY_BACKSPACE SBGL_SCANCODE_BACKSPACE
#define SBGL_KEY_TAB SBGL_SCANCODE_TAB
#define SBGL_KEY_SPACE SBGL_SCANCODE_SPACE
#define SBGL_KEY_RIGHT SBGL_SCANCODE_RIGHT
#define SBGL_KEY_LEFT SBGL_SCANCODE_LEFT
#define SBGL_KEY_DOWN SBGL_SCANCODE_DOWN
#define SBGL_KEY_UP SBGL_SCANCODE_UP
#define SBGL_KEY_LSHIFT SBGL_SCANCODE_LSHIFT
#define SBGL_KEY_LCTRL SBGL_SCANCODE_LCTRL
#define SBGL_KEY_LALT SBGL_SCANCODE_LALT

#define SBGL_MOUSE_LEFT SBGL_MOUSE_BUTTON_LEFT
#define SBGL_MOUSE_RIGHT SBGL_MOUSE_BUTTON_RIGHT
#define SBGL_MOUSE_MIDDLE SBGL_MOUSE_BUTTON_MIDDLE

// --- API ---

/**
 * @brief Initializes the engine and opens a window.
 *
 * This function sets up the internal HALs and the Vulkan 1.3 backend.
 *
 * @param w Desired width of the window.
 * @param h Desired height of the window.
 * @param title Title displayed in the OS window manager.
 * @return An initialization result containing the context and any error code.
 */
sbgl_InitResult sbgl_Init(int w, int h, const char* title);

/**
 * @brief Gracefully shuts down the engine and releases all resources.
 * @param ctx The context to destroy.
 */
void sbgl_Shutdown(sbgl_Context* ctx);

/**
 * @brief Checks if the user or OS has requested to close the window.
 *
 * This flag is set by the OS (e.g., clicking the 'X' button or Alt+F4).
 * The application must poll this flag in the main loop and manually
 * initiate shutdown by calling sbgl_Shutdown() when it returns true.
 *
 * @param ctx The active engine context.
 * @return True if a close request is pending, false otherwise.
 */
bool sbgl_WindowShouldClose(sbgl_Context* ctx);

/**
 * @brief Retrieves the current window dimensions.
 * @param ctx The active engine context.
 * @param w Pointer to store the width.
 * @param h Pointer to store the height.
 */
void sbgl_GetWindowSize(sbgl_Context* ctx, int* w, int* h);

/**
 * @brief Prepares the engine for a new frame of drawing.
 *
 * This function handles event polling and Vulkan swapchain image acquisition.
 * Must be called before any clearing or drawing commands.
 *
 * @param ctx The engine context.
 */
void sbgl_BeginDrawing(sbgl_Context* ctx);

/**
 * @brief Finalizes the current frame and presents it to the screen.
 *
 * This function submits the recorded GPU commands and swaps the buffer.
 *
 * @param ctx The engine context.
 */
void sbgl_EndDrawing(sbgl_Context* ctx);

/**
 * @brief Synchronizes the CPU with the GPU, waiting for all commands to complete.
 *
 * This must be called before destroying resources (Pipelines, Buffers, Shaders) 
 * to ensure they are no longer in use by the GPU. Failure to do so will 
 * result in Vulkan validation errors and potential instability.
 *
 * @param ctx The engine context.
 */
void sbgl_DeviceWaitIdle(sbgl_Context* ctx);

/**
 * @brief Sets the clear color for the next frame.
 *
 * @param ctx The engine context.
 * @param r Red component (0.0 to 1.0).
 * @param g Green component (0.0 to 1.0).
 * @param b Blue component (0.0 to 1.0).
 * @param a Alpha component (0.0 to 1.0).
 */
void sbgl_Clear(sbgl_Context* ctx, float r, float g, float b, float a);

/**
 * @brief Retrieves the complete input state for the current frame.
 *
 * Adheres to Data-Oriented Design by providing a read-only view of all
 * input arrays (keys, mouse) for efficient batch processing.
 *
 * @param ctx The engine context.
 * @return A read-only pointer to the current input state. Never returns NULL.
 */
const sbgl_InputState* sbgl_GetInputState(sbgl_Context* ctx);

// --- Rendering API ---

/**
 * @brief Creates a GPU buffer.
 * @param ctx The engine context.
 * @param usage Intended usage of the buffer (Vertex/Index).
 * @param size Size in bytes.
 * @param data Optional initial data. If NULL, the buffer is created empty.
 * @return A handle to the created buffer.
 */
sbgl_Buffer
sbgl_CreateBuffer(sbgl_Context* ctx, sbgl_BufferUsage usage, size_t size, const void* data);

/**
 * @brief Destroys a GPU buffer.
 * 
 * @warning All submitted GPU commands referencing this buffer must have 
 * completed before calling this. Use sbgl_DeviceWaitIdle() to synchronize.
 *
 * @param ctx The engine context.
 * @param buffer The buffer handle.
 */
void sbgl_DestroyBuffer(sbgl_Context* ctx, sbgl_Buffer buffer);

/**
 * @brief Loads a shader from SPIR-V bytecode.
 * @param ctx The engine context.
 * @param stage The shader stage (Vertex/Fragment).
 * @param bytecode Pointer to the SPIR-V bytecode.
 * @param size Size of the bytecode in bytes.
 * @return A handle to the loaded shader.
 */
sbgl_Shader
sbgl_LoadShader(sbgl_Context* ctx, sbgl_ShaderStage stage, const uint32_t* bytecode, size_t size);

/**
 * @brief Destroys a shader module.
 * 
 * @warning Shaders must not be in use by the GPU. Use sbgl_DeviceWaitIdle() 
 * before bulk destruction during shutdown.
 *
 * @param ctx The engine context.
 * @param shader The shader handle.
 */
void sbgl_DestroyShader(sbgl_Context* ctx, sbgl_Shader shader);

/**
 * @brief Creates a graphics pipeline.
 * @param ctx The engine context.
 * @param config Configuration for the pipeline.
 * @return A handle to the created pipeline.
 */
sbgl_Pipeline sbgl_CreatePipeline(sbgl_Context* ctx, const sbgl_PipelineConfig* config);

/**
 * @brief Destroys a graphics pipeline.
 * 
 * @warning The pipeline must not be in use by any submitted command buffers.
 * Use sbgl_DeviceWaitIdle() to ensure safe destruction.
 *
 * @param ctx The engine context.
 * @param pipeline The pipeline handle.
 */
void sbgl_DestroyPipeline(sbgl_Context* ctx, sbgl_Pipeline pipeline);

/**
 * @brief Binds a graphics pipeline for subsequent draw calls.
 * @param ctx The engine context.
 * @param pipeline The pipeline handle.
 */
void sbgl_BindPipeline(sbgl_Context* ctx, sbgl_Pipeline pipeline);

/**
 * @brief Binds a buffer to the pipeline.
 * @param ctx The engine context.
 * @param buffer The buffer handle.
 * @param usage The usage to bind it as (Vertex/Index).
 */
void sbgl_BindBuffer(sbgl_Context* ctx, sbgl_Buffer buffer, sbgl_BufferUsage usage);

/**
 * @brief Submits a non-indexed draw command.
 * @param ctx The engine context.
 * @param vertexCount Number of vertices to draw.
 * @param firstVertex Offset to the first vertex.
 */
void sbgl_Draw(sbgl_Context* ctx, uint32_t vertexCount, uint32_t firstVertex);

/**
 * @brief Submits an indexed draw command.
 * @param ctx The engine context.
 * @param indexCount Number of indices to draw.
 * @param firstIndex Offset to the first index.
 * @param vertexOffset Value added to each index before indexing into vertex buffers.
 */
void sbgl_DrawIndexed(
	sbgl_Context* ctx,
	uint32_t indexCount,
	uint32_t firstIndex,
	int32_t vertexOffset
);

/**
 * @brief Updates push constants for the currently bound pipeline.
 * @param ctx The engine context.
 * @param size Size of the data to push.
 * @param data Pointer to the data.
 */
void sbgl_PushConstants(sbgl_Context* ctx, size_t size, const void* data);

// --- Automated Batching API ---

struct SblArena;

/**
 * @brief Creates a thread-local render queue for collecting draw commands.
 * 
 * Draw packets are stored in this queue and must be submitted to the 
 * backend for rendering. Using multiple queues allows for parallel 
 * command recording.
 *
 * @param ctx The engine context.
 * @param arena The arena to use for queue allocations.
 * @return A pointer to the newly created render queue.
 */
sbgl_RenderQueue* sbgl_CreateRenderQueue(sbgl_Context* ctx, struct SblArena* arena);

/**
 * @brief Appends a draw command to the render queue.
 *
 * This is a high-performance operation. The packets are cached in the 
 * queue until the next frame.
 *
 * @param queue The render queue to append to.
 * @param mesh The mesh identifier.
 * @param material The material identifier.
 * @param key The sort key for minimizing state changes.
 * @param data Pointer to the per-instance data (transform, color, etc).
 */
void sbgl_SubmitDraw(sbgl_RenderQueue* queue, uint32_t mesh, uint32_t material, sbgl_SortKey key, const sbgl_InstanceData* data);

/**
 * @brief Merges, sorts, and submits all pending draw commands to the GPU.
 *
 * This function processes multiple render queues in a single batching 
 * operation. It performs a stable radix sort on the packets to minimize
 * state transitions and then bakes them into indirect draw commands.
 *
 * @param ctx The engine context.
 * @param queues Array of pointers to the render queues to be processed.
 * @param queueCount Number of queues in the array.
 * @param viewProj Pointer to the view-projection matrix for the frame.
 */
void sbgl_RenderQueues(sbgl_Context* ctx, sbgl_RenderQueue** queues, uint32_t queueCount, const sbgl_Mat4* viewProj);

#endif // SBGL_H
