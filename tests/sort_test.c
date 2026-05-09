#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "../src/core/sbgl_sort.h"

static double get_time_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static void test_radix_sort_basic(void) {
	printf("Testing basic sort... ");
	uint32_t count = 5;
	sbgl_SortKey keys[] = { 50, 10, 40, 20, 30 };
	uint32_t values[] = { 0, 1, 2, 3, 4 };
	sbgl_SortKey temp_keys[5];
	uint32_t temp_values[5];

	sbgl_radix_sort(keys, values, count, temp_keys, temp_values);

	for (uint32_t i = 0; i < count - 1; ++i) {
		assert(keys[i] <= keys[i + 1]);
	}
    
    // Verify values are still associated
    for (uint32_t i = 0; i < count; ++i) {
        if (keys[i] == 10) assert(values[i] == 1);
        if (keys[i] == 20) assert(values[i] == 3);
        if (keys[i] == 30) assert(values[i] == 4);
        if (keys[i] == 40) assert(values[i] == 2);
        if (keys[i] == 50) assert(values[i] == 0);
    }
    printf("Passed.\n");
}

static void test_radix_sort_duplicates(void) {
	printf("Testing duplicates... ");
	uint32_t count = 6;
	sbgl_SortKey keys[] = { 10, 20, 10, 30, 20, 10 };
	uint32_t values[] = { 0, 1, 2, 3, 4, 5 };
	sbgl_SortKey temp_keys[6];
	uint32_t temp_values[6];

	sbgl_radix_sort(keys, values, count, temp_keys, temp_values);

	for (uint32_t i = 0; i < count - 1; ++i) {
		assert(keys[i] <= keys[i + 1]);
	}
    
    // Stable sort check: for key 10, values should be 0, 2, 5 in that order
    assert(keys[0] == 10 && values[0] == 0);
    assert(keys[1] == 10 && values[1] == 2);
    assert(keys[2] == 10 && values[2] == 5);
    
    // for key 20, values should be 1, 4
    assert(keys[3] == 20 && values[3] == 1);
    assert(keys[4] == 20 && values[4] == 4);
    
    assert(keys[5] == 30 && values[5] == 3);
    printf("Passed (Stable).\n");
}

static void test_radix_sort_reverse(void) {
	printf("Testing reverse sorted... ");
	uint32_t count = 100;
	sbgl_SortKey* keys = (sbgl_SortKey*)malloc(sizeof(sbgl_SortKey) * count);
	uint32_t* values = (uint32_t*)malloc(sizeof(uint32_t) * count);
	sbgl_SortKey* tk = (sbgl_SortKey*)malloc(sizeof(sbgl_SortKey) * count);
	uint32_t* tv = (uint32_t*)malloc(sizeof(uint32_t) * count);

	for (uint32_t i = 0; i < count; ++i) {
		keys[i] = (sbgl_SortKey)(count - i);
		values[i] = i;
	}

	sbgl_radix_sort(keys, values, count, tk, tv);

	for (uint32_t i = 0; i < count - 1; ++i) {
		assert(keys[i] <= keys[i + 1]);
		assert(keys[i] == (sbgl_SortKey)(i + 1));
	}

	free(keys);
	free(values);
	free(tk);
	free(tv);
	printf("Passed.\n");
}

static void test_radix_sort_large(void) {
	printf("Testing large random array (100,000 elements)... ");
	uint32_t count = 100000;
	sbgl_SortKey* keys = (sbgl_SortKey*)malloc(sizeof(sbgl_SortKey) * count);
	uint32_t* values = (uint32_t*)malloc(sizeof(uint32_t) * count);
	sbgl_SortKey* tk = (sbgl_SortKey*)malloc(sizeof(sbgl_SortKey) * count);
	uint32_t* tv = (uint32_t*)malloc(sizeof(uint32_t) * count);

	srand(42);
	for (uint32_t i = 0; i < count; ++i) {
		keys[i] = ((sbgl_SortKey)rand() << 32) | (sbgl_SortKey)rand();
		values[i] = i;
	}

	double start = get_time_ms();
	sbgl_radix_sort(keys, values, count, tk, tv);
	double end = get_time_ms();

	for (uint32_t i = 0; i < count - 1; ++i) {
		assert(keys[i] <= keys[i + 1]);
	}

	free(keys);
	free(values);
	free(tk);
	free(tv);
	printf("Passed. Time: %.3fms\n", end - start);
}

int main(void) {
    printf("--- SBgl Radix Sort Comprehensive Test ---\n");
    test_radix_sort_basic();
    test_radix_sort_duplicates();
    test_radix_sort_reverse();
    test_radix_sort_large();
    printf("All sort tests passed!\n");
    return 0;
}
