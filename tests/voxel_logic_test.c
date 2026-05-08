#include <assert.h>
#include <stdio.h>

// Function to be implemented
int calculate_instance_count(int radius) {
    int width = (radius * 2 + 1);
    return width * width;
}

int update_radius(int current, int delta) {
    int next = current + delta;
    if (next < 1) return 1;
    if (next > 50) return 50;
    return next;
}

int main() {
    printf("Running Voxel Logic Tests...\n");

    // Test 1: Instance count calculation
    // Radius 5 -> (5*2 + 1)^2 = 121
    assert(calculate_instance_count(5) == 121);
    // Radius 10 -> (10*2 + 1)^2 = 441
    assert(calculate_instance_count(10) == 441);
    printf("Instance Count Calculation: PASSED\n");

    // Test 2: Radius bounds and updates
    // Increase radius
    assert(update_radius(5, 1) == 6);
    // Decrease radius
    assert(update_radius(5, -1) == 4);
    // Min bound (1)
    assert(update_radius(1, -1) == 1);
    // Max bound (50)
    assert(update_radius(50, 1) == 50);
    printf("Radius Update Logic: PASSED\n");

    printf("All Voxel Logic Tests PASSED!\n");
    return 0;
}
