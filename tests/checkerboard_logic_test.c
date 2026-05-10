#include <assert.h>
#include <stdio.h>
#include <stdint.h>

// Logic from voxel3D_main.c to be tested
static uint32_t get_voxel_color(int x, int y, int z) {
    if ((x + y + z) % 2 == 0) {
        // Vary color based on height (Y)
        uint8_t r = (uint8_t)(y * 8);
        uint8_t g = (uint8_t)(255 - y * 4);
        uint8_t b = (uint8_t)(128 + x * 4);
        return (r << 16) | (g << 8) | b;
    } else {
        return 0; // Empty
    }
}

int main(void) {
    printf("Running Checkerboard Logic Tests...\n");

    // Test (0,0,0) - should be solid (even sum)
    if (get_voxel_color(0, 0, 0) == 0) {
        printf("Fail: (0,0,0) should be solid\n");
        assert(0);
    }

    // Test (1,0,0) - should be empty (odd sum)
    if (get_voxel_color(1, 0, 0) != 0) {
        printf("Fail: (1,0,0) should be empty\n");
        assert(0);
    }

    // Test (0,1,0) - should be empty (odd sum)
    if (get_voxel_color(0, 1, 0) != 0) {
        printf("Fail: (0,1,0) should be empty\n");
        assert(0);
    }

    // Test (1,1,1) - should be empty (odd sum)
    if (get_voxel_color(1, 1, 1) != 0) {
        printf("Fail: (1,1,1) should be empty\n");
        assert(0);
    }

    // Test (1,1,0) - should be solid (even sum)
    if (get_voxel_color(1, 1, 0) == 0) {
        printf("Fail: (1,1,0) should be solid\n");
        assert(0);
    }

    printf("Checkerboard Logic: PASSED\n");
    return 0;
}
