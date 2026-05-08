#ifndef SBGL_TYPES_H
#define SBGL_TYPES_H

#include <stdbool.h>

#include <stdint.h>

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
	sbgl_Vec4 color;	 /**< Per-instance color tint. */
} sbgl_InstanceData;

/**
 * @brief Standard vertex structure for basic geometry rendering.
 * Optimized for cache density (16 bytes).
 */
typedef struct {
	int16_t position[4]; /**< SNORM position [3] + 1 padding. (8 bytes) */
	uint32_t color;		 /**< Packed RGBA8 color. (4 bytes) */
	uint32_t _padding;	 /**< Alignment padding. (4 bytes) */
} sbgl_Vertex;

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

#define SBGL_PACKET_MESH_MASK 0x7FF
#define SBGL_PACKET_MAT_MASK 0x7FF
#define SBGL_PACKET_BLEND_MASK 0x3
#define SBGL_PACKET_SIDED_MASK 0x1
#define SBGL_PACKET_TAGS_MASK 0x7F

#define SBGL_PACKET_MAT_SHIFT 11
#define SBGL_PACKET_BLEND_SHIFT 22
#define SBGL_PACKET_SIDED_SHIFT 24
#define SBGL_PACKET_TAGS_SHIFT 25

#define SBGL_PACK_HEADER(mesh, mat, blend, sided, tags)                                            \
	(((mesh) & SBGL_PACKET_MESH_MASK) |                                                            \
	 (((mat) & SBGL_PACKET_MAT_MASK) << SBGL_PACKET_MAT_SHIFT) |                                   \
	 (((blend) & SBGL_PACKET_BLEND_MASK) << SBGL_PACKET_BLEND_SHIFT) |                             \
	 (((sided) & SBGL_PACKET_SIDED_MASK) << SBGL_PACKET_SIDED_SHIFT) |                             \
	 (((tags) & SBGL_PACKET_TAGS_MASK) << SBGL_PACKET_TAGS_SHIFT))

#define SBGL_GET_MESH_ID(h) ((h) & SBGL_PACKET_MESH_MASK)
#define SBGL_GET_MAT_ID(h) (((h) >> SBGL_PACKET_MAT_SHIFT) & SBGL_PACKET_MAT_MASK)
#define SBGL_GET_BLEND_MODE(h) (((h) >> SBGL_PACKET_BLEND_SHIFT) & SBGL_PACKET_BLEND_MASK)

/**
 * @brief Encapsulates all data required to submit a single draw call.
 * Optimized for cache density (16 bytes).
 */
typedef struct {
	sbgl_SortKey key;  /**< Sorting key based on material, mesh, and depth. (8 bytes) */
	uint32_t header;   /**< Packed MeshID, MaterialID, and rendering flags. (4 bytes) */
	uint32_t _padding; /**< Explicit padding for 16-byte alignment. (4 bytes) */
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
 * @brief Data formats for vertex attributes.
 */
typedef enum {
	SBGL_FORMAT_R32_SFLOAT = 0,
	SBGL_FORMAT_R32G32_SFLOAT = 1,
	SBGL_FORMAT_R32G32B32_SFLOAT = 2,
	SBGL_FORMAT_R32G32B32A32_SFLOAT = 3,
	SBGL_FORMAT_R16G16B16A16_SNORM = 4,
	SBGL_FORMAT_R8G8B8A8_UNORM = 5,
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
 * @brief Engine context.
 *
 * The context serves as the handle for all SBgl operations. It utilizes
 * an opaque pointer pattern to encapsulate internal engine state, ensuring
 * that OS-specific handles and internal memory management are hidden from
 * the public API.
 */
typedef struct sbgl_Context {
	/**
	 * @brief Opaque pointer to the internal engine state.
	 *
	 * Points to the private `sbgl_InternalContext` structure, which manages
	 * internal subsystems including the persistent `SblArena` for context-local
	 * allocations, the platform-specific `sbgl_Window` handle, graphics
	 * state such as clear colors and frame acquisition flags, and the
	 * real-time physical state of keys and mouse buttons.
	 */
	void* inner;

	/**
	 * @brief Status of the last major operation.
	 *
	 * Stores the result or error code from the most recent critical API
	 * call (for instance, initialization or frame acquisition).
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
