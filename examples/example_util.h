#ifndef SBGL_EXAMPLE_UTIL_H
#define SBGL_EXAMPLE_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sbgl.h>

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

/**
 * Helper to load a shader from a SPIR-V file.
 */
static inline sbgl_Shader example_load_shader(sbgl_Context* ctx, sbgl_ShaderStage stage, const char* filename) {
    size_t size;
    uint32_t* code = read_file(filename, &size);
    if (!code) return SBGL_INVALID_HANDLE;

    sbgl_Shader shader = sbgl_LoadShader(ctx, stage, code, size);
    free(code);
    return shader;
}

/**
 * Structure to hold common example state and reduce boilerplate.
 */
typedef struct {
    sbgl_Context* ctx;
    uint32_t width;
    uint32_t height;
} ExampleApp;

/**
 * Initializes the example app context and window.
 */
static inline bool example_app_init(ExampleApp* app, uint32_t w, uint32_t h, const char* title) {
    sbgl_InitResult res = sbgl_Init(w, h, title);
    if (res.error != SBGL_SUCCESS) return false;
    app->ctx = res.ctx;
    app->width = w;
    app->height = h;
    return true;
}

/**
 * Shuts down the example app.
 */
static inline void example_app_shutdown(ExampleApp* app) {
    if (app->ctx) {
        sbgl_DeviceWaitIdle(app->ctx);
        sbgl_Shutdown(app->ctx);
    }
}

#endif // SBGL_EXAMPLE_UTIL_H
