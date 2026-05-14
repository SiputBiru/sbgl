#ifndef SBGL_TYPES_H
#define SBGL_TYPES_H

#include <stdbool.h>

#include <stdint.h>

#define SBGL_INVALID_HANDLE 0
#define SBGL_INVALID_OFFSET ((uint32_t)-1)

#include "sbgl_math.h"

/**
 * @brief Per-instance data for automated batching.
 *
 * This structure is uploaded to the GPU as an SSBO and accessed
 * via Buffer Device Address (BDA) in the vertex shader.
 */
typedef struct {
  sbgl_Mat4 transform; /**< World transformation matrix. */
  sbgl_Vec4 color;   /**< Per-instance color tint. */
} sbgl_InstanceData;

/**
 * @brief Standard vertex structure for basic geometry rendering.
 * Optimized for cache density (16 bytes).
 */
typedef struct {
  int16_t position[4]; /**< SNORM position [3] + 1 padding. (8 bytes) */
  uint32_t color;     /**< Packed RGBA8 color. (4 bytes) */
  uint32_t _padding;   /**< Alignment padding. (4 bytes) */
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
 * @brief Handle for a compute pipeline.
 */
typedef uint32_t sbgl_ComputePipeline;

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

/* Voxel instance layout: 
   Word 0: 18 bits VoxelID, 8 bits Y-Scale, 6 bits MaterialID
   Word 1: 16 bits AO (8 corners * 2 bits), 16 bits Reserved */
#define SBGL_VOXEL_ID_MASK 0x3FFFF
#define SBGL_VOXEL_SCALE_MASK 0xFF
#define SBGL_VOXEL_SCALE_SHIFT 18
#define SBGL_VOXEL_MAT_MASK 0x3F
#define SBGL_VOXEL_MAT_SHIFT 26

#define SBGL_PACK_VOXEL_INSTANCE(id, scale, mat) \
  (((id) & SBGL_VOXEL_ID_MASK) | (((scale) & SBGL_VOXEL_SCALE_MASK) << SBGL_VOXEL_SCALE_SHIFT) | (((mat) & SBGL_VOXEL_MAT_MASK) << SBGL_VOXEL_MAT_SHIFT))

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
 * @brief Standard Vulkan Indirect Draw command layout.
 */
typedef struct {
  uint32_t indexCount;    /**< Number of vertices (or indices) to draw. */
  uint32_t instanceCount; /**< Number of instances to draw. */
  uint32_t firstIndex;    /**< Offset into the index buffer. */
  int32_t vertexOffset;   /**< Value added to vertex indices. */
  uint32_t firstInstance; /**< ID of the first instance. */
} sbgl_IndirectCommand;

/**
 * @brief Buffer usage flags.
 */
typedef enum {
  SBGL_BUFFER_USAGE_VERTEX = 0x01,
  SBGL_BUFFER_USAGE_INDEX = 0x02,
  SBGL_BUFFER_USAGE_STORAGE = 0x04,
  SBGL_BUFFER_USAGE_INDIRECT = 0x08,
  SBGL_BUFFER_USAGE_TRANSFER_DST = 0x10,
} sbgl_BufferUsage;

/**
 * @brief Shader stage flags.
 */
typedef enum {
  SBGL_SHADER_STAGE_VERTEX = 0x01,
  SBGL_SHADER_STAGE_FRAGMENT = 0x02,
  SBGL_SHADER_STAGE_COMPUTE = 0x04,
} sbgl_ShaderStage;

/**
 * @brief Memory barrier types for compute synchronization.
 */
typedef enum {
  SBGL_BARRIER_COMPUTE_TO_COMPUTE = 0,
  SBGL_BARRIER_COMPUTE_TO_INDIRECT = 1,
  SBGL_BARRIER_COMPUTE_TO_GRAPHICS = 2,
  SBGL_BARRIER_GRAPHICS_TO_COMPUTE = 3,
  SBGL_BARRIER_HOST_TO_COMPUTE = 4,
  SBGL_BARRIER_HOST_TO_GRAPHICS = 5,
} sbgl_BarrierType;

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
  uint32_t location;  /**< Shader input location. */
  uint32_t offset;  /**< Byte offset within the vertex structure. */
  sbgl_Format format; /**< Data format of the attribute. */
} sbgl_VertexAttribute;

/**
 * @brief Performance telemetry data for a single frame.
 */
typedef struct {
  float cpu_frame_time;    /**< Total frame duration (ms). */
  float cpu_sort_time;     /**< Time spent in sort/bake (ms). */
  float gpu_render_time;   /**< Actual GPU execution time (ms). */
  uint32_t draw_calls;     /**< Total indirect batches submitted. */
  uint32_t instance_count; /**< Total instances rendered. */
} sbgl_Telemetry;

/**
 * @brief Vertex input layout definition.
 */
typedef struct {
  uint32_t stride;
  uint32_t attributeCount;
  const sbgl_VertexAttribute* attributes;
} sbgl_VertexLayout;

/**
 * @brief Blending modes for pipeline color attachments.
 */
typedef enum {
  SBGL_BLEND_MODE_NONE = 0,
  SBGL_BLEND_MODE_ALPHA = 1,
  SBGL_BLEND_MODE_ADDITIVE = 2,
} sbgl_BlendMode;

/**
 * @brief Configuration for creating a graphics pipeline.
 */
typedef struct {
  sbgl_Shader vertexShader;
  sbgl_Shader fragmentShader;
  sbgl_VertexLayout vertexLayout;
  sbgl_BlendMode blendMode;
} sbgl_PipelineConfig;

/**
 * @brief Result codes for engine operations.
 */
typedef enum {
  SBGL_SUCCESS = 0,
  SBGL_ERROR_NULL_CONTEXT = 1,
  SBGL_ERROR_INVALID_ARGUMENT = 2,
  SBGL_ERROR_INITIALIZATION_FAILED = 3,
  SBGL_ERROR_WINDOW_CREATION_FAILED = 4,
  SBGL_ERROR_GRAPHICS_FAILED = 5,
  SBGL_ERROR_OUT_OF_MEMORY = 6,
  SBGL_ERROR_DEVICE_BUSY = 7,
  SBGL_ERROR_INVALID_HANDLE = 8,
  SBGL_ERROR_SWAPCHAIN_FAILED = 9,
  SBGL_ERROR_SHADER_FAILED = 10,
  SBGL_ERROR_PIPELINE_FAILED = 11,
  SBGL_ERROR_PLATFORM_FAILED = 12,
} sbgl_Result;

/**
 * @brief Result codes for platform layer operations.
 */
typedef enum {
  SBGL_PLATFORM_SUCCESS = 0,
  SBGL_PLATFORM_ERROR_WINDOW_FAILED = 1,
  SBGL_PLATFORM_ERROR_NO_DISPLAY = 2,
  SBGL_PLATFORM_ERROR_INIT_FAILED = 3,
} sbgl_platform_Result;

/**
 * @brief Backend types supported by the engine.
 */
typedef enum {
  SBGL_BACKEND_VULKAN = 0,
} sbgl_BackendType;

/**
 * @brief Detailed error information for debugging.
 *
 * Provides both core error codes and backend-specific error details
 * for comprehensive error inspection.
 */
typedef struct {
  sbgl_BackendType type;  /**< Active backend type. */
  sbgl_Result      coreResult;  /**< Core error code. */
  int32_t          vkResult;    /**< Raw VkResult (0 if not Vulkan). */
  uint32_t         extension;    /**< Platform-specific extension code. */
} sbgl_ErrorDetail;

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
 * @brief Resource limits for engine initialization.
 *
 * Configures the maximum number of GPU resources that can be allocated
 * simultaneously. These limits affect memory usage and should be tuned
 * based on application requirements.
 */
typedef struct {
  uint32_t maxBuffers;   /**< Maximum GPU buffers (default: 1024). */
  uint32_t maxShaders;   /**< Maximum shader modules (default: 256). */
  uint32_t maxPipelines; /**< Maximum graphics/compute pipelines (default: 256). */
} sbgl_ResourceLimits;

/**
 * @brief Configuration for engine initialization.
 *
 * Provides explicit control over initialization parameters including
 * resource limits and runtime behavior flags.
 */
typedef struct {
  uint32_t windowWidth;        /**< Initial window width in pixels. */
  uint32_t windowHeight;       /**< Initial window height in pixels. */
  const char* windowTitle;     /**< Window title string (optional, may be NULL). */
  sbgl_ResourceLimits limits;  /**< Resource allocation limits. */
  bool enableValidation;       /**< Enable Vulkan validation layers. */
} sbgl_InitConfig;

/**
 * @brief Default initialization configuration values.
 *
 * Use this as a starting point for custom configurations:
 * @code
 * sbgl_InitConfig config = sbgl_DefaultInitConfig;
 * config.windowWidth = 1920;
 * config.windowHeight = 1080;
 * config.limits.maxBuffers = 4096;
 * sbgl_InitResult res = sbgl_InitWithConfig(&config);
 * @endcode
 */
#define sbgl_DefaultInitConfig ((sbgl_InitConfig){ \
  .windowWidth = 800, \
  .windowHeight = 600, \
  .windowTitle = "SBgl Application", \
  .limits = { .maxBuffers = 1024, .maxShaders = 256, .maxPipelines = 256 }, \
  .enableValidation = true \
})

/**
 * @brief Result structure for initialization.
 */
typedef struct {
  sbgl_Context* ctx;
  sbgl_Result error;
} sbgl_InitResult;

typedef struct sbgl_Window sbgl_Window;

#endif // SBGL_TYPES_H
