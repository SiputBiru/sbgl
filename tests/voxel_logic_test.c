#include <assert.h>
#include <stdio.h>

// Function to be implemented
static int calculate_instance_count(int radius) {
    int width = (radius * 2 + 1);
    return width * width;
}

static int update_radius(int current, int delta) {
    int next = current + delta;
    if (next < 1) return 1;
    if (next > 50) return 50;
    return next;
}

int main(void) {
    printf("Running Voxel Logic Tests...\n");

    // Test 1: Instance count calculation
    // Radius 5 -> (5*2 + 1)^2 = 121
    if (calculate_instance_count(5) != 121) {
        printf("Instance count failed for radius 5\n");
        assert(0);
    }
    // Radius 10 -> (10*2 + 1)^2 = 441
    if (calculate_instance_count(10) != 441) {
        printf("Instance count failed for radius 10\n");
        assert(0);
    }
    printf("Instance Count Calculation: PASSED\n");

    // Test 2: Radius bounds and updates
    // Increase radius
    if (update_radius(5, 1) != 6) {
        printf("Radius update failed (increase)\n");
        assert(0);
    }
    // Decrease radius
    if (update_radius(5, -1) != 4) {
        printf("Radius update failed (decrease)\n");
        assert(0);
    }
    // Min bound (1)
    if (update_radius(1, -1) != 1) {
        printf("Radius update failed (min bound)\n");
        assert(0);
    }
    // Max bound (50)
    if (update_radius(50, 1) != 50) {
        printf("Radius update failed (max bound)\n");
        assert(0);
    }
    printf("Radius Update Logic: PASSED\n");

    printf("All Voxel Logic Tests PASSED!\n");
    return 0;
}
