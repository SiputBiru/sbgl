#ifndef SBGL_VOXEL_H
#define SBGL_VOXEL_H

/**
 * @file sbgl_voxel.h
 * @brief Public API for the SBgl voxel rendering system.
 */

#include "sbgl.h"

/**
 * @brief Opaque handle to the voxel system.
 * The internal structure is hidden to maintain a clean public API and 
 * facilitate backend-agnostic module development.
 */
typedef struct sbgl_VoxelSystem sbgl_VoxelSystem;

/**
 * @brief Configuration parameters for initializing the voxel system.
 */
typedef struct {
  /**
   * @brief Maximum number of chunk slots to manage in the GPU-side pool.
   * This value determines the total memory footprint of the voxel system.
   */
  uint32_t max_slots;

  /**
   * @brief Radius (in chunks) around the camera that should be maintained.
   * Chunks outside this radius are candidates for recycling.
   */
  uint32_t chunk_radius;

  /**
   * @brief Enables console output for performance metrics and visible chunk counts.
   */
  bool enable_telemetry;
} sbgl_VoxelConfig;

typedef struct {
  sbgl_Vec4 color;
  float roughness;
  float metalness;
  uint32_t textureID;
  uint32_t _pad;
} sbgl_Material;

/**
 * @brief Creates and initializes a new voxel system.
 * This operation allocates GPU resources and internal tracking structures.
 *
 * @param ctx Pointer to the active SBgl context.
 * @param config Pointer to the configuration parameters.
 * @return Pointer to the initialized voxel system, or NULL if creation failed.
 */
sbgl_VoxelSystem* sbgl_Voxel_Create(sbgl_Context* ctx, const sbgl_VoxelConfig* config);

/**
 * @brief Updates the voxel system state based on the current camera position.
 * This function handles chunk generation requests and manages the LRU cache.
 *
 * @param sys Pointer to the voxel system.
 * @param camera_pos The current world-space position of the camera.
 */
void sbgl_Voxel_Update(sbgl_VoxelSystem* sys, sbgl_Vec3 camera_pos);

/**
 * @brief Performs GPU-driven frustum culling on voxel chunks.
 * MUST be called BEFORE sbgl_BeginDrawing.
 *
 * @param sys Pointer to the voxel system.
 * @param view_proj The view-projection matrix for the current frame.
 */
void sbgl_Voxel_Cull(sbgl_VoxelSystem* sys, sbgl_Mat4 view_proj);

/**
 * @brief Issues the indirect draw calls for visible voxel chunks.
 * MUST be called BETWEEN sbgl_BeginDrawing and sbgl_EndDrawing.
 *
 * @param sys Pointer to the voxel system.
 */
void sbgl_Voxel_Render(sbgl_VoxelSystem* sys);

/**
 * @brief Destroys the voxel system and releases all associated resources.
 *
 * @param sys Pointer to the voxel system to destroy.
 */
void sbgl_Voxel_Destroy(sbgl_VoxelSystem* sys);

/** @brief Returns the device address of the AABB buffer. */
uint64_t sbgl_Voxel_GetAABBAddress(sbgl_VoxelSystem* sys);

/** @brief Returns the device address of the instance buffer. */
uint64_t sbgl_Voxel_GetInstanceAddress(sbgl_VoxelSystem* sys);

/** @brief Returns the device address of the material palette buffer. */
uint64_t sbgl_Voxel_GetPaletteAddress(sbgl_VoxelSystem* sys);

#endif // SBGL_VOXEL_H
