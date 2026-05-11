#include "voxel_pool.h"
#include <string.h>

/**
 * The VoxelPool uses a simple linear search for coordinate mapping.
 * For small capacities (like 512 slots), this is extremely cache-friendly
 * and avoids the complexity of a hash map.
 */

VoxelPool* VoxelPool_Init(SblArena* arena, uint32_t capacity) {
  VoxelPool* pool = (VoxelPool*)sbl_arena_alloc_zero(arena, sizeof(VoxelPool));
  if (!pool) return NULL;
  pool->capacity = capacity;
  
  // Allocate arrays for slot data on the arena to maintain contiguous memory.
  pool->positions = (sbgl_ivec3*)sbl_arena_alloc_zero(arena, sizeof(sbgl_ivec3) * capacity);
  pool->last_used_frames = (uint64_t*)sbl_arena_alloc_zero(arena, sizeof(uint64_t) * capacity);
  pool->active = (uint8_t*)sbl_arena_alloc_zero(arena, sizeof(uint8_t) * capacity);
  
  if (!pool->positions || !pool->last_used_frames || !pool->active) return NULL;
  
  return pool;
}

int32_t VoxelPool_AcquireSlot(VoxelPool* pool, sbgl_ivec3 pos, bool* is_new) {
  int32_t lru_index = 0;
  uint64_t min_frame = UINT64_MAX;

  // Search for an existing slot with matching coordinates or find an empty slot.
  for (uint32_t i = 0; i < pool->capacity; ++i) {
    if (pool->active[i]) {
      if (pool->positions[i].x == pos.x && 
          pool->positions[i].y == pos.y && 
          pool->positions[i].z == pos.z) {
        // Found an existing slot, update its frame for LRU.
        pool->last_used_frames[i] = pool->current_frame;
        if (is_new) *is_new = false;
        return (int32_t)i;
      }
    } else {
      // Found an inactive slot, assign it immediately.
      pool->active[i] = 1;
      pool->positions[i] = pos;
      pool->last_used_frames[i] = pool->current_frame;
      if (is_new) *is_new = true;
      return (int32_t)i;
    }

    // Keep track of the oldest used slot in case we need to recycle.
    if (pool->last_used_frames[i] < min_frame) {
      min_frame = pool->last_used_frames[i];
      lru_index = (int32_t)i;
    }
  }

  // No existing or inactive slots available; recycle the least recently used one.
  pool->positions[lru_index] = pos;
  pool->last_used_frames[lru_index] = pool->current_frame;
  if (is_new) *is_new = true;
  return lru_index;
}

void VoxelPool_UpdateFrame(VoxelPool* pool, uint64_t frame) {
  pool->current_frame = frame;
}
