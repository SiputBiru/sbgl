#ifndef SBGL_TYPES_H
#define SBGL_TYPES_H

#include <stdbool.h>

#include <stdint.h>

#define SBGL_MAX_KEYS 512
#define SBGL_MAX_MOUSE_BUTTONS 8

#define SBGL_INVALID_HANDLE 0

/**
 * @brief Handle for a GPU-side buffer.
 */
typedef uint32_t sbgl_Buffer;

/**
 * @brief Handle for a shader module.
 */
typedef uint32_t sbgl_Shader;

/**
 * @brief Handle for a graphics pipeline.
 */
typedef uint32_t sbgl_Pipeline;

/**
 * @brief Buffer usage flags.
 */
typedef enum {
	SBGL_BUFFER_USAGE_VERTEX = 0x01,
	SBGL_BUFFER_USAGE_INDEX = 0x02,
} sbgl_BufferUsage;

/**
 * @brief Shader stage flags.
 */
typedef enum {
	SBGL_SHADER_STAGE_VERTEX = 0x01,
	SBGL_SHADER_STAGE_FRAGMENT = 0x02,
} sbgl_ShaderStage;

/**
 * @brief Vertex attribute definition.
 */
typedef struct {
	uint32_t location;
	uint32_t offset;
	// For simplicity, we assume float components for now.
	// In a full implementation, we'd have a format enum.
} sbgl_VertexAttribute;

/**
 * @brief Vertex input layout definition.
 */
typedef struct {
	uint32_t stride;
	uint32_t attributeCount;
	const sbgl_VertexAttribute* attributes;
} sbgl_VertexLayout;

/**
 * @brief Configuration for creating a graphics pipeline.
 */
typedef struct {
	sbgl_Shader vertexShader;
	sbgl_Shader fragmentShader;
	sbgl_VertexLayout vertexLayout;
} sbgl_PipelineConfig;

/**
 * @brief Represents the real-time state of physical inputs.
 */
typedef struct sbgl_InputState {
	bool keysDown[SBGL_MAX_KEYS];
	bool keysPressed[SBGL_MAX_KEYS];
	bool mouseDown[SBGL_MAX_MOUSE_BUTTONS];
	int mouseX, mouseY;
	int mouseDeltaX, mouseDeltaY;
	int _internalMouseX, _internalMouseY; /**< Internal tracking for deltas. */
} sbgl_InputState;

/**
 * @brief Parameters for orthographic projection.
 */
typedef struct {
	float left, right;
	float bottom, top;
	float near_p, far_p;
} sbgl_OrthoParams;

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
 *
 * The context serves as the central handle for all SBgl operations. It utilizes
 * an opaque pointer pattern to encapsulate internal engine state, ensuring
 * that OS-specific handles and internal memory management are hidden from
 * the public API.
 */
typedef struct sbgl_Context {
	/**
	 * @brief Opaque pointer to the internal engine state.
	 *
	 * Points to the private `sbgl_InternalContext` structure, which manages
	 * the following internal subsystems:
	 * - **Memory Arena**: The persistent `SblArena` used for context-local allocations.
	 * - **Native Window**: The platform-specific `sbgl_Window` handle.
	 * - **Graphics State**: Clear colors and frame acquisition flags.
	 * - **Input State**: The real-time physical state of keys and mouse buttons.
	 */
	void* inner;

	/**
	 * @brief Status of the last major operation.
	 *
	 * Stores the result or error code from the most recent critical API
	 * call (e.g., initialization or frame acquisition).
	 */
	sbgl_Result result;
} sbgl_Context;

/**
 * @brief Result structure for initialization.
 */
typedef struct {
	sbgl_Context* ctx;
	sbgl_Result error;
} sbgl_InitResult;

typedef struct sbgl_Window sbgl_Window;

#endif // SBGL_TYPES_H
