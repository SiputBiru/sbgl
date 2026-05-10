#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define CHUNK_SIZE 32
#define MASK_WORDS (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE / 32)

/**
 * Sets a bit in the 3D bitmask. Coordinates are validated against CHUNK_SIZE
 * to prevent out-of-bounds memory access.
 */
static void set_bit(uint32_t* mask, int x, int y, int z) {
  if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) {
    return;
  }
  int idx = z * (CHUNK_SIZE * CHUNK_SIZE) + y * CHUNK_SIZE + x;
  mask[idx / 32] |= (1u << (idx % 32));
}

/**
 * Retrieves the state of a bit in the 3D bitmask. Returns false for any
 * coordinates outside the valid chunk boundaries.
 */
static bool get_bit(const uint32_t* mask, int x, int y, int z) {
  if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) {
    return false;
  }
  int idx = z * (CHUNK_SIZE * CHUNK_SIZE) + y * CHUNK_SIZE + x;
  return (mask[idx / 32] & (1u << (idx % 32))) != 0;
}

int main(void) {
  uint32_t mask[MASK_WORDS] = {0};

  /* The system verifies that a bit can be set and retrieved at a specific coordinate. */
  set_bit(mask, 1, 1, 1);
  assert(get_bit(mask, 1, 1, 1) == true);
  assert(get_bit(mask, 1, 1, 2) == false);
  
  /* Boundary conditions at the minimum and maximum extents of the chunk are validated. */
  set_bit(mask, 0, 0, 0);
  assert(get_bit(mask, 0, 0, 0) == true);
  set_bit(mask, 31, 31, 31);
  assert(get_bit(mask, 31, 31, 31) == true);
  
  printf("Bitmask logic verified.\n");
  return 0;
}
