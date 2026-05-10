#ifndef VOXEL_POOL_H
#define VOXEL_POOL_H

#include "sbgl_math.h"
#include "core/sbl_arena.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 3D integer vector for chunk coordinates.
 */
typedef struct {
  int32_t x, y, z;
} sbgl_ivec3;

/**
 * @brief VoxelPool manages a fixed number of slots for voxel chunks.
 * It uses a flat array layout to maintain cache efficiency.
 */
typedef struct {
  sbgl_ivec3* positions;      /**< Chunk world coordinates for each slot. */
  uint64_t* last_used_frames; /**< Frame index when the slot was last accessed. */
  uint8_t* active;            /**< Flag indicating if the slot is currently in use. */
  uint32_t capacity;          /**< Total number of managed slots. */
  uint64_t current_frame;     /**< Current frame counter for LRU tracking. */
} VoxelPool;

/**
 * @brief Initializes a VoxelPool on the provided arena.
 * @param arena Pointer to the memory arena to allocate from.
 * @param capacity Maximum number of slots in the pool.
 * @return Pointer to the initialized VoxelPool.
 */
VoxelPool* VoxelPool_Init(SblArena* arena, uint32_t capacity);

/**
 * @brief Acquires a slot for a given chunk position.
 * Returns the slot index (0 to capacity-1).
 * If the position is already in a slot, returns that index and updates LRU.
 * If not, recycles the least recently used slot.
 * @param pool Pointer to the VoxelPool.
 * @param pos The world coordinates of the chunk.
 * @return Index of the acquired slot.
 */
int32_t VoxelPool_AcquireSlot(VoxelPool* pool, sbgl_ivec3 pos);

/**
 * @brief Advances the pool's internal frame counter for LRU tracking.
 * @param pool Pointer to the VoxelPool.
 * @param frame The current frame index.
 */
void VoxelPool_UpdateFrame(VoxelPool* pool, uint64_t frame);

#endif // VOXEL_POOL_H
