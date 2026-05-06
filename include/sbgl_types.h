#ifndef SBGL_TYPES_H
#define SBGL_TYPES_H

#include <stdbool.h>

#include <stdint.h>

#include "sbgl_input.h"

#define SBGL_INVALID_HANDLE 0

#include "sbgl_math.h"

/**
 * @brief Per-instance data for automated batching.
 *
 * This structure is uploaded to the GPU as an SSBO and accessed
 * via Buffer Device Address (BDA) in the vertex shader.
 */
typedef struct {
    sbgl_Mat4 transform; /**< World transformation matrix. */
    sbgl_Vec4 color;     /**< Per-instance color tint. */
} sbgl_InstanceData;

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
 * @brief Bit-packed key used for sorting draw commands to minimize state changes.
 */
typedef uint64_t sbgl_SortKey;

/**
 * @brief Encapsulates all data required to submit a single draw call.
 */
typedef struct {
    sbgl_SortKey key;      /**< Sorting key based on material, mesh, and depth. */
    uint32_t instanceId;   /**< Index into the per-instance data buffer. */
    uint32_t meshId;       /**< Identifier for the geometry to be rendered. */
    uint32_t materialId;   /**< Identifier for the material parameters. */
} sbgl_DrawPacket;

/**
 * @brief Internal queue used to batch and sort draw packets before submission.
 */
typedef struct sbgl_RenderQueue sbgl_RenderQueue;

/**
 * @brief Buffer usage flags.
 */
typedef enum {
	SBGL_BUFFER_USAGE_VERTEX = 0x01,
	SBGL_BUFFER_USAGE_INDEX = 0x02,
	SBGL_BUFFER_USAGE_STORAGE = 0x04,
	SBGL_BUFFER_USAGE_INDIRECT = 0x08,
} sbgl_BufferUsage;

/**
 * @brief Shader stage flags.
 */
typedef enum {
	SBGL_SHADER_STAGE_VERTEX = 0x01,
	SBGL_SHADER_STAGE_FRAGMENT = 0x02,
} sbgl_ShaderStage;

/**
 * @brief Standard data formats for vertex attributes.
 */
typedef enum {
	SBGL_FORMAT_R32_SFLOAT = 0,
	SBGL_FORMAT_R32G32_SFLOAT = 1,
	SBGL_FORMAT_R32G32B32_SFLOAT = 2,
	SBGL_FORMAT_R32G32B32A32_SFLOAT = 3,
} sbgl_Format;

/**
 * @brief Vertex attribute definition.
 */
typedef struct {
	uint32_t location;	/**< Shader input location. */
	uint32_t offset;	/**< Byte offset within the vertex structure. */
	sbgl_Format format; /**< Data format of the attribute. */
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
 * @brief Result codes for engine operations.
 */
typedef enum {
	SBGL_SUCCESS = 0,
	SBGL_ERROR_INITIALIZATION_FAILED = 1,
	SBGL_ERROR_WINDOW_CREATION_FAILED = 2,
	SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED = 3,
	SBGL_ERROR_OUT_OF_MEMORY = 4,
	SBGL_ERROR_DEVICE_BUSY = 5,
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
