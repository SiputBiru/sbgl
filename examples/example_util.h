#ifndef SBGL_EXAMPLE_UTIL_H
#define SBGL_EXAMPLE_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * Reads a file into a buffer.
 * Performs a fallback check in build/examples/ if the file is not found in the current directory.
 */
static inline uint32_t* read_file(const char* filename, size_t* out_size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        // Fallback for running from project root
        char fallback[256];
        snprintf(fallback, sizeof(fallback), "build/examples/%s", filename);
        file = fopen(fallback, "rb");
        if (!file) {
            fprintf(stderr, "Error: Could not open file '%s' (or '%s')\n", filename, fallback);
            return NULL;
        }
    }
    fseek(file, 0, SEEK_END);
    *out_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint32_t* buffer = malloc(*out_size);
    if (buffer) {
        size_t read = fread(buffer, 1, *out_size, file);
        if (read != *out_size) {
            free(buffer);
            fclose(file);
            return NULL;
        }
    }
    fclose(file);
    return buffer;
}

#endif // SBGL_EXAMPLE_UTIL_H
