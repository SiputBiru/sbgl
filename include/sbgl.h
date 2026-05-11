/**
 * @file sbgl.h
 * @brief API for the SiputBiru Graphics Library (SBgl).
 *
 * This header defines the interface for window management,
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

#define SBGL_KEY_MINUS SBGL_SCANCODE_MINUS
#define SBGL_KEY_EQUAL SBGL_SCANCODE_EQUAL

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
 * This flag is set by the OS (for instance, clicking the 'X' button or Alt+F4).
 * The application polls this flag in the main loop and manually initiates
 * shutdown by calling sbgl_Shutdown() when it returns true.
 *
 * @param ctx The active engine context.
 * @return True if a close request is pending, false otherwise.
 */
bool sbgl_WindowShouldClose(sbgl_Context* ctx);

/**
 * @brief Retrieves the current monotonic system time in seconds.
 * 
 * This timer is high-resolution and suitable for frame-time calculation.
 * 
 * @param ctx The active engine context.
 * @return Monotonic time in seconds.
 */
double sbgl_GetTime(sbgl_Context* ctx);

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
 * It is called before clearing or drawing commands.
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
 * @brief Prepares the engine for compute operations before the main drawing pass.
 *
 * If a frame has not yet been started, this function initiates the frame lifecycle,
 * acquiring a swapchain image and starting the command buffer. It must be called
 * outside of a BeginDrawing/EndDrawing block.
 *
 * @param ctx The engine context.
 */
void sbgl_BeginCompute(sbgl_Context* ctx);

/**
 * @brief Finalizes the compute phase.
 *
 * This function is semantically symmetrical to sbgl_BeginCompute. It does not
 * submit the frame; that is still handled by sbgl_EndDrawing.
 *
 * @param ctx The engine context.
 */
void sbgl_EndCompute(sbgl_Context* ctx);

/**
 * @brief Synchronizes the CPU with the GPU, waiting for all commands to complete.
 *
 * This is called before destroying resources (Pipelines, Buffers, Shaders) 
 * to ensure they are no longer in use by the GPU. Failure to do so 
 * results in Vulkan validation errors and instability.
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
 * @brief Retrieves the input state for the current frame.
 *
 * Adheres to Data-Oriented Design by providing a read-only view of 
 * input arrays (keys, mouse) for batch processing.
 *
 * @param ctx The engine context.
 * @return A read-only pointer to the current input state. Never returns NULL.
 */
const sbgl_InputState* sbgl_GetInputState(sbgl_Context* ctx);

/**
 * @brief Sets the cursor behavior and visibility.
 * @param ctx The engine context.
 * @param mode The desired mouse mode.
 */
void sbgl_SetMouseMode(sbgl_Context* ctx, sbgl_MouseMode mode);

/**
 * @brief Retrieves the performance telemetry data for the previous frame.
 * @param ctx The active engine context.
 * @return A structure containing CPU and GPU timing data.
 */
sbgl_Telemetry sbgl_GetTelemetry(sbgl_Context* ctx);

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
 * All submitted GPU commands referencing this buffer must have 
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
 * @brief Helper function to load a shader directly from a SPIR-V file.
 * 
 * @param ctx The engine context.
 * @param stage The shader stage (Vertex/Fragment).
 * @param filename Path to the SPIR-V file.
 * @return A handle to the loaded shader, or SBGL_INVALID_HANDLE on failure.
 */
sbgl_Shader
sbgl_LoadShaderFromFile(sbgl_Context* ctx, sbgl_ShaderStage stage, const char* filename);

/**
 * @brief Destroys a shader module.
 * 
 * Shaders must not be in use by the GPU. Use sbgl_DeviceWaitIdle() 
 * before destruction during shutdown.
 *
 * @param ctx The engine context.
 * @param shader The shader handle.
 */
void sbgl_DestroyShader(sbgl_Context* ctx, sbgl_Shader shader);

/**
 * @brief Maps a GPU buffer into the CPU's address space.
 *
 * This allows for direct reading from and writing to GPU memory.
 * SBgl buffers are created as host-visible and coherent by default.
 *
 * @param ctx The engine context.
 * @param buffer The buffer handle.
 * @return A pointer to the mapped memory, or NULL on failure.
 */
void* sbgl_MapBuffer(sbgl_Context* ctx, sbgl_Buffer buffer);

/**
 * @brief Unmaps a previously mapped GPU buffer.
 * @param ctx The engine context.
 * @param buffer The buffer handle.
 */
void sbgl_UnmapBuffer(sbgl_Context* ctx, sbgl_Buffer buffer);

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
 * The pipeline must not be in use by any submitted command buffers.
 * Use sbgl_DeviceWaitIdle() to ensure safe destruction.
 *
 * @param ctx The engine context.
 * @param pipeline The pipeline handle.
 */
void sbgl_DestroyPipeline(sbgl_Context* ctx, sbgl_Pipeline pipeline);

/**
 * @brief Creates a compute pipeline.
 * @param ctx The engine context.
 * @param shader The compute shader handle.
 * @return A handle to the created compute pipeline.
 */
sbgl_ComputePipeline sbgl_CreateComputePipeline(sbgl_Context* ctx, sbgl_Shader shader);

/**
 * @brief Destroys a compute pipeline.
 * @param ctx The engine context.
 * @param pipeline The compute pipeline handle.
 */
void sbgl_DestroyComputePipeline(sbgl_Context* ctx, sbgl_ComputePipeline pipeline);

/**
 * @brief Binds a compute pipeline for subsequent dispatch calls.
 * @param ctx The engine context.
 * @param pipeline The compute pipeline handle.
 */
void sbgl_BindComputePipeline(sbgl_Context* ctx, sbgl_ComputePipeline pipeline);

/**
 * @brief Dispatches a compute workload.
 * @param ctx The engine context.
 * @param groupCountX Number of workgroups in X dimension.
 * @param groupCountY Number of workgroups in Y dimension.
 * @param groupCountZ Number of workgroups in Z dimension.
 */
void sbgl_DispatchCompute(sbgl_Context* ctx, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

/**
 * @brief Injects a memory barrier to synchronize compute and graphics operations.
 * @param ctx The engine context.
 * @param type The type of barrier to inject.
 */
void sbgl_MemoryBarrier(sbgl_Context* ctx, sbgl_BarrierType type);

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
 * @brief Retrieves the 64-bit GPU virtual address for a buffer.
 *
 * This address is used for Buffer Device Address (BDA) lookups in shaders.
 *
 * @param ctx The engine context.
 * @param buffer The buffer handle.
 * @return The 64-bit GPU address, or 0 on failure.
 */
uint64_t sbgl_GetBufferDeviceAddress(sbgl_Context* ctx, sbgl_Buffer buffer);

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
 * @brief Submits a batch of draw calls stored in a GPU buffer.
 *
 * @param ctx The engine context.
 * @param buffer Handle to the buffer containing an array of sbgl_IndirectCommand.
 * @param offset The byte offset into the buffer where the commands begin.
 * @param drawCount The number of commands to execute from the buffer.
 */
void sbgl_DrawIndirect(
  sbgl_Context* ctx,
  sbgl_Buffer buffer,
  size_t offset,
  uint32_t drawCount
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
 * Draw packets are stored in this queue and submitted to the 
 * backend for rendering. Multiple queues enable parallel 
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
 * The packets are cached in the queue until the next frame.
 *
 * @param queue The render queue to append to.
 * @param mesh The mesh identifier.
 * @param material The material identifier.
 * @param blendMode The blend mode identifier.
 * @param sidedness The sidedness flag (e.g., single/double sided).
 * @param tags Custom user tags for filtering.
 * @param key The sort key for minimizing state changes.
 * @param data Pointer to the per-instance data (transform, color, etc).
 */
void sbgl_SubmitDraw(
  sbgl_RenderQueue* queue,
  uint32_t mesh,
  uint32_t material,
  uint32_t blendMode,
  uint32_t sidedness,
  uint32_t tags,
  sbgl_SortKey key,
  const sbgl_InstanceData* data
);

/**
 * @brief Merges, sorts, and submits pending draw commands to the GPU.
 *
 * This function processes render queues in a batching operation. 
 * A stable radix sort is performed on the packets to minimize
 * state transitions, followed by baking them into indirect draw commands.
 *
 * @param ctx The engine context.
 * @param queues Array of pointers to the render queues to be processed.
 * @param queueCount Number of queues in the array.
 * @param viewProj Pointer to the view-projection matrix for the frame.
 */
void sbgl_RenderQueues(sbgl_Context* ctx, sbgl_RenderQueue** queues, uint32_t queueCount, const sbgl_Mat4* viewProj);

/**
 * @brief Extended version of sbgl_RenderQueues with user metadata.
 *
 * @param ctx The engine context.
 * @param queues Array of render queues.
 * @param queueCount Number of queues.
 * @param viewProj View-projection matrix.
 * @param userAddress A 64-bit GPU address for user-defined data (e.g., SSBO heightmap).
 */
void sbgl_RenderQueuesEx(sbgl_Context* ctx, sbgl_RenderQueue** queues, uint32_t queueCount, const sbgl_Mat4* viewProj, uint64_t userAddress);

#endif // SBGL_H
