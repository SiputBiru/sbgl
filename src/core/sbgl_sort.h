/**
 * @file sbgl_sort.h
 * @brief Sorting utilities for the SBgl batching system.
 */

#ifndef SBGL_SORT_H
#define SBGL_SORT_H

#include "sbgl_types.h"
#include <stdint.h>

/**
 * @brief Performs a stable radix sort on an array of 64-bit keys and associated 32-bit values.
 *
 * This implementation uses a Least Significant Digit (LSD) approach with 8 passes
 * of 8-bit buckets to achieve O(N) complexity for 64-bit keys with low memory overhead.
 *
 * @param keys        Pointer to the array of sorting keys. Modified in place.
 * @param values      Pointer to the array of associated indices or values. Modified in place.
 * @param count       The number of elements to sort.
 * @param temp_keys   Workspace buffer for keys. Must be at least 'count' in size.
 * @param temp_values Workspace buffer for values. Must be at least 'count' in size.
 */
void sbgl_radix_sort(
	sbgl_SortKey* keys,
	uint32_t* values,
	uint32_t count,
	sbgl_SortKey* temp_keys,
	uint32_t* temp_values
);

#endif // SBGL_SORT_H
