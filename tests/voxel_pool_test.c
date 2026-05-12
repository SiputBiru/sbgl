#include "sbgl_pool.h"
#include "core/sbl_arena.h"
#include <assert.h>
#include <stdio.h>

static void test_voxel_pool_basic(void) {
  printf("Running test_voxel_pool_basic...\n");
  
  SblArena arena;
  sbl_arena_init(&arena, 1024 * 1024);

  VoxelPool* pool = VoxelPool_Init(&arena, 4); // Small capacity for testing LRU
  
  sbgl_ivec3 p1 = {0, 0, 0};
  sbgl_ivec3 p2 = {1, 0, 0};
  sbgl_ivec3 p3 = {0, 1, 0};
  sbgl_ivec3 p4 = {0, 0, 1};
  sbgl_ivec3 p5 = {1, 1, 1};

  // Fill the pool
  bool is_new;
  int32_t i1 = VoxelPool_AcquireSlot(pool, p1, &is_new); assert(is_new);
  int32_t i2 = VoxelPool_AcquireSlot(pool, p2, &is_new); assert(is_new);
  int32_t i3 = VoxelPool_AcquireSlot(pool, p3, &is_new); assert(is_new);
  int32_t i4 = VoxelPool_AcquireSlot(pool, p4, &is_new); assert(is_new);

  assert(i1 == 0);
  assert(i2 == 1);
  assert(i3 == 2);
  assert(i4 == 3);
  printf("Initial fill successful.\n");

  // Re-acquire p1, should update LRU but stay at index 0, is_new should be false
  VoxelPool_UpdateFrame(pool, 10);
  int32_t i1_again = VoxelPool_AcquireSlot(pool, p1, &is_new);
  assert(!is_new);
  assert(i1_again == 0);
  assert(pool->last_used_frames[0] == 10);
  printf("Re-acquisition successful.\n");

  // Acquire p5, should recycle p2 (index 1) because p1 was just used (frame 10) 
  // and p2, p3, p4 are still at frame 0. p2 is the first oldest one found.
  VoxelPool_UpdateFrame(pool, 20);
  int32_t i5 = VoxelPool_AcquireSlot(pool, p5, &is_new);
  assert(is_new);
  assert(i5 == 1);
  assert(pool->positions[1].x == 1);
  assert(pool->positions[1].y == 1);
  assert(pool->positions[1].z == 1);
  printf("LRU recycling successful.\n");

  sbl_arena_free(&arena);
  printf("test_voxel_pool_basic passed!\n");
}

static void test_voxel_pool_two_pass(void) {
  printf("Running test_voxel_pool_two_pass...\n");

  SblArena arena;
  sbl_arena_init(&arena, 1024 * 1024);

  VoxelPool* pool = VoxelPool_Init(&arena, 4);

  sbgl_ivec3 p1 = {0, 0, 0};
  sbgl_ivec3 p2 = {1, 0, 0};

  bool is_new;
  // Fill slot 0 with p1
  VoxelPool_AcquireSlot(pool, p1, &is_new);
  // Fill slot 1 with p2
  VoxelPool_AcquireSlot(pool, p2, &is_new);

  // Deactivate slot 0 manually to simulate an empty slot
  pool->active[0] = 0;

  // Now, try to acquire p2. 
  // A single-pass implementation that prioritizes empty slots might return index 0 as "new".
  // A two-pass implementation should find p2 at index 1 and return it as NOT new.
  int32_t index = VoxelPool_AcquireSlot(pool, p2, &is_new);
  assert(index == 1);
  assert(!is_new);

  printf("Match after empty successful.\n");

  sbl_arena_free(&arena);
  printf("test_voxel_pool_two_pass passed!\n");
}

int main(void) {
  test_voxel_pool_basic();
  test_voxel_pool_two_pass();
  return 0;
}
