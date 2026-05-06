#include "sbgl_sort.h"
#include <stdlib.h>
#include <string.h>

/**
 * @file sbgl_sort.c
 * @brief Implementation of radix sort for 64-bit keys.
 */

void sbgl_radix_sort(sbgl_SortKey* keys, uint32_t* values, uint32_t count) {
    if (count <= 1) {
        return;
    }

    // Allocate temporary buffers for double buffering during passes.
    sbgl_SortKey* temp_keys = (sbgl_SortKey*)malloc(sizeof(sbgl_SortKey) * count);
    uint32_t* temp_values = (uint32_t*)malloc(sizeof(uint32_t) * count);
    
    if (!temp_keys || !temp_values) {
        if (temp_keys) free(temp_keys);
        if (temp_values) free(temp_values);
        return;
    }

    sbgl_SortKey* src_keys = keys;
    sbgl_SortKey* dst_keys = temp_keys;
    uint32_t* src_values = values;
    uint32_t* dst_values = temp_values;

    // We perform 4 passes, each processing 16 bits of the 64-bit key.
    // This uses 65536 buckets per pass.
    for (int pass = 0; pass < 4; ++pass) {
        uint32_t counts[65536] = {0};
        uint32_t offsets[65536] = {0};
        int shift = pass * 16;

        // Build histogram for the current 16-bit digit.
        for (uint32_t i = 0; i < count; ++i) {
            uint16_t bucket = (uint16_t)((src_keys[i] >> shift) & 0xFFFF);
            counts[bucket]++;
        }

        // Compute prefix sums to find starting offsets for each bucket.
        uint32_t total_offset = 0;
        for (uint32_t i = 0; i < 65536; ++i) {
            offsets[i] = total_offset;
            total_offset += counts[i];
        }

        // Scatter elements into the destination buffer based on the current digit.
        for (uint32_t i = 0; i < count; ++i) {
            uint16_t bucket = (uint16_t)((src_keys[i] >> shift) & 0xFFFF);
            uint32_t pos = offsets[bucket]++;
            dst_keys[pos] = src_keys[i];
            dst_values[pos] = src_values[i];
        }

        // Swap source and destination buffers for the next pass.
        sbgl_SortKey* tk = src_keys;
        src_keys = dst_keys;
        dst_keys = tk;

        uint32_t* tv = src_values;
        src_values = dst_values;
        dst_values = tv;
    }

    // After 4 passes, the data might be in the temp buffers or the original ones.
    // Since we swapped 4 times, src_keys should point back to the original keys array.
    // However, to ensure correctness against changes in pass count, we check and copy if necessary.
    if (src_keys != keys) {
        memcpy(keys, src_keys, sizeof(sbgl_SortKey) * count);
        memcpy(values, src_values, sizeof(uint32_t) * count);
    }

    free(temp_keys);
    free(temp_values);
}
