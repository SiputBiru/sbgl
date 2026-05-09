#include "sbgl_sort.h"
#include <stdlib.h>
#include <string.h>

/**
 * @file sbgl_sort.c
 * @brief Implementation of radix sort for 64-bit keys.
 */

void sbgl_radix_sort(
	sbgl_SortKey* keys,
	uint32_t* values,
	uint32_t count,
	sbgl_SortKey* temp_keys,
	uint32_t* temp_values
) {
	if (count <= 1 || !temp_keys || !temp_values) {
		return;
	}

	sbgl_SortKey* src_keys = keys;
	sbgl_SortKey* dst_keys = temp_keys;
	uint32_t* src_values = values;
	uint32_t* dst_values = temp_values;

	// The system performs 8 passes, each processing 8 bits of the 64-bit key.
	// This uses 256 buckets per pass, significantly reducing cache misses and stack zeroing.
	for (int pass = 0; pass < 8; ++pass) {
		uint32_t counts[256] = { 0 };
		uint32_t offsets[256] = { 0 };
		int shift = pass * 8;

		// Build histogram for the current 8-bit digit.
		for (uint32_t i = 0; i < count; ++i) {
			uint8_t bucket = (uint8_t)((src_keys[i] >> shift) & 0xFF);
			counts[bucket]++;
		}

		// Compute prefix sums to find starting offsets for each bucket.
		uint32_t total_offset = 0;
		for (uint32_t i = 0; i < 256; ++i) {
			offsets[i] = total_offset;
			total_offset += counts[i];
		}

		// Scatter elements into the destination buffer based on the current digit.
		for (uint32_t i = 0; i < count; ++i) {
			uint8_t bucket = (uint8_t)((src_keys[i] >> shift) & 0xFF);
			uint32_t pos = offsets[bucket]++;
			dst_keys[pos] = src_keys[i];
			dst_values[pos] = src_values[i];
		}

		// Swap source and destination pointers for the next pass.
		sbgl_SortKey* tk = src_keys;
		src_keys = dst_keys;
		dst_keys = tk;

		uint32_t* tv = src_values;
		src_values = dst_values;
		dst_values = tv;
	}

	// Since 8 passes are performed, the pointers naturally swap back to the original buffers.
	// This confirms src_keys points to 'keys' and src_values points to 'values'.
}
