#include "sbgl.h"
#include "sbgl_math.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>

// Mocking the shader logic in C
int get_hX(int worldIX, int worldSize) {
    return ((worldIX % worldSize) + worldSize) % worldSize;
}

int main() {
    printf("Running Heightmap Coordinate Mapping Test...\n");

    const int WORLD_SIZE = 2048;

    // Test 1: Positive coordinates
    assert(get_hX(0, WORLD_SIZE) == 0);
    assert(get_hX(10, WORLD_SIZE) == 10);
    assert(get_hX(2047, WORLD_SIZE) == 2047);
    assert(get_hX(2048, WORLD_SIZE) == 0);
    assert(get_hX(2049, WORLD_SIZE) == 1);

    // Test 2: Negative coordinates (The "cliff" zone)
    assert(get_hX(-1, WORLD_SIZE) == 2047);
    assert(get_hX(-2048, WORLD_SIZE) == 0);
    assert(get_hX(-2049, WORLD_SIZE) == 2047);
    
    printf("Coordinate Wrapping Logic: PASSED\n");

    // Test 3: Chunk boundary transitions
    // Scenario: Moving from chunk 0 to chunk 1
    // Chunk 0, localX 31 -> WorldX 31
    // Chunk 1, localX 0  -> WorldX 32
    int camX = 0;
    int chunk0_offset = -5;
    int world_c0_start = (camX + chunk0_offset) * 32; // -160
    
    // Last block of chunk 0
    int world_c0_last = 31 + world_c0_start; // -129
    int idx_c0_last = get_hX(world_c0_last, WORLD_SIZE);
    
    // First block of chunk 1
    int chunk1_offset = -4;
    int world_c1_start = (camX + chunk1_offset) * 32; // -128
    int world_c1_first = 0 + world_c1_start; // -128
    int idx_c1_first = get_hX(world_c1_first, WORLD_SIZE);
    
    printf("Transition: %d -> %d\n", idx_c0_last, idx_c1_first);
    assert(idx_c1_first == (idx_c0_last + 1) % WORLD_SIZE);

    printf("Chunk Transition Logic: PASSED\n");

    printf("All Heightmap Tests PASSED!\n");
    return 0;
}
