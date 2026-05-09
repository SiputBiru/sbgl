#include <assert.h>
#include <stdio.h>
#include <time.h>

#include "sbgl_batcher.h"

static double get_time_us(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000000.0 + ts.tv_nsec / 1000.0;
}

/**
 * Verifies the baking logic by providing a set of sorted draw packets.
 *
 * The test defines packets with varying mesh, material, and sort keys to
 * ensure that grouping occurs only when all batching criteria are met.
 */
static void test_bake_commands(void) {
	sbgl_DrawPacket packets[5];

	// Group 1: Two identical packets
	packets[0] = (sbgl_DrawPacket){ .key = 10, .header = SBGL_PACK_HEADER(1, 1, 0, 0, 0) };
	packets[1] = (sbgl_DrawPacket){ .key = 10, .header = SBGL_PACK_HEADER(1, 1, 0, 0, 0) };

	// Group 2: Different mesh
	packets[2] = (sbgl_DrawPacket){ .key = 10, .header = SBGL_PACK_HEADER(2, 1, 0, 0, 0) };

	// Group 3: Different material
	packets[3] = (sbgl_DrawPacket){ .key = 10, .header = SBGL_PACK_HEADER(2, 2, 0, 0, 0) };

	// Group 4: Different key
	packets[4] = (sbgl_DrawPacket){ .key = 11, .header = SBGL_PACK_HEADER(2, 2, 0, 0, 0) };

	sbgl_IndirectCommand commands[5];
	uint32_t count = sbgl_bake_commands(packets, 5, commands, 5);

	// The system should identify 4 distinct command groups based on state changes.
	assert(count == 4);

	// Group 1 verification (2 instances, mesh 1 -> 36 indices)
	assert(commands[0].instanceCount == 2);
	assert(commands[0].indexCount == 36);
	assert(commands[0].firstInstance == 0);

	// Group 2 verification (1 instance, mesh 2 -> 18 indices)
	assert(commands[1].instanceCount == 1);
	assert(commands[1].indexCount == 18);
	assert(commands[1].firstInstance == 2);

	// Group 3 verification (1 instance, mesh 2, new material)
	assert(commands[2].instanceCount == 1);
	assert(commands[2].indexCount == 18);
	assert(commands[2].firstInstance == 3);

	// Group 4 verification (1 instance, mesh 2, new sort key)
	assert(commands[3].instanceCount == 1);
	assert(commands[3].indexCount == 18);
	assert(commands[3].firstInstance == 4);

	printf("PASS: test_bake_commands (Correctness)\n");

	// Benchmark baking logic
	printf("Benchmarking baking logic (100,000 iterations)... ");
	double start = get_time_us();
	uint32_t iterations = 100000;
	for (uint32_t i = 0; i < iterations; i++) {
		sbgl_bake_commands(packets, 5, commands, 5);
	}
	double end = get_time_us();
	printf("Time: %.3f us/call\n", (end - start) / iterations);
}

int main(void) {
	test_bake_commands();
	return 0;
}
