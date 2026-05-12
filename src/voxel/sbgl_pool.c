#include "sbgl_pool.h"
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
  // Search for an existing slot with matching coordinates to avoid duplicates.
  // This first pass ensures that active chunks are not accidentally overwritten.
  for (uint32_t i = 0; i < pool->capacity; ++i) {
    if (pool->active[i]) {
      if (pool->positions[i].x == pos.x && 
          pool->positions[i].y == pos.y && 
          pool->positions[i].z == pos.z) {
        // The slot is already in the pool; update its timestamp and return.
        pool->last_used_frames[i] = pool->current_frame;
        if (is_new) *is_new = false;
        return (int32_t)i;
      }
    }
  }

  // If no matching slot was found, search for an available or least recently used slot.
  int32_t lru_index = -1;
  int32_t empty_index = -1;
  uint64_t min_frame = UINT64_MAX;

  for (uint32_t i = 0; i < pool->capacity; ++i) {
    if (!pool->active[i]) {
      // Locate the first available empty slot for immediate assignment.
      if (empty_index == -1) {
        empty_index = (int32_t)i;
      }
    } else {
      // Track the oldest active slot to serve as a candidate for eviction.
      if (pool->last_used_frames[i] < min_frame) {
        min_frame = pool->last_used_frames[i];
        lru_index = (int32_t)i;
      }
    }
  }

  // Select the empty slot if one exists; otherwise, recycle the LRU candidate.
  int32_t target_index = (empty_index != -1) ? empty_index : lru_index;

  // In the event that no slot can be identified, return an invalid index.
  if (target_index == -1) {
    return -1;
  }

  // Finalize the acquisition by updating the slot's state and position metadata.
  pool->active[target_index] = 1;
  pool->positions[target_index] = pos;
  pool->last_used_frames[target_index] = pool->current_frame;
  if (is_new) *is_new = true;

  return target_index;
}

void VoxelPool_UpdateFrame(VoxelPool* pool, uint64_t frame) {
  pool->current_frame = frame;
}
