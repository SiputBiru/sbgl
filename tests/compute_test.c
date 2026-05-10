#include <sbgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/**
 * @brief Simple push constant structure matching the compute shader.
 */
typedef struct {
  uint64_t inputA;
  uint64_t inputB;
  uint64_t outputBuffer;
  uint32_t count;
} ComputePushConstants;

/**
 * Verifies the compute API by performing a vector addition on the GPU.
 */
static void test_compute_vector_add(void) {
  printf("Starting compute functional test: Vector Addition...\n");

  /* The system initializes the engine context with a hidden window to enable
     Vulkan compute execution without requiring a visible display. */
  sbgl_InitResult res = sbgl_Init(100, 100, "Compute Test");
  if (res.error != SBGL_SUCCESS) {
    fprintf(stderr, "Failed to initialize SBgl\n");
    exit(1);
  }
  sbgl_Context* ctx = res.ctx;

  /* The compute shader is loaded from the compiled SPIR-V binary. */
  sbgl_Shader computeShader = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_COMPUTE, "shaders/compute_test.comp.spv");
  if (computeShader == SBGL_INVALID_HANDLE) {
    fprintf(stderr, "Failed to load compute shader\n");
    sbgl_Shutdown(ctx);
    exit(1);
  }

  /* A compute pipeline is created using the loaded shader module. */
  sbgl_ComputePipeline pipeline = sbgl_CreateComputePipeline(ctx, computeShader);
  if (pipeline == SBGL_INVALID_HANDLE) {
    fprintf(stderr, "Failed to create compute pipeline\n");
    sbgl_DestroyShader(ctx, computeShader);
    sbgl_Shutdown(ctx);
    exit(1);
  }

  /* The test prepares three buffers for the input and output data. */
  const uint32_t count = 1024;
  float* dataA = (float*)malloc(count * sizeof(float));
  float* dataB = (float*)malloc(count * sizeof(float));
  for (uint32_t i = 0; i < count; i++) {
    dataA[i] = (float)i;
    dataB[i] = (float)i * 2.0f;
  }

  sbgl_Buffer bufA = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, count * sizeof(float), dataA);
  sbgl_Buffer bufB = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, count * sizeof(float), dataB);
  sbgl_Buffer bufOut = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, count * sizeof(float), NULL);

  /* The GPU virtual addresses are retrieved for Buffer Device Address (BDA) access. */
  ComputePushConstants push = {
    .inputA = sbgl_GetBufferDeviceAddress(ctx, bufA),
    .inputB = sbgl_GetBufferDeviceAddress(ctx, bufB),
    .outputBuffer = sbgl_GetBufferDeviceAddress(ctx, bufOut),
    .count = count
  };

  /* The system records and submits the compute workload within a standard frame lifecycle. */
  sbgl_BeginDrawing(ctx);
  
  sbgl_BindComputePipeline(ctx, pipeline);
  sbgl_PushConstants(ctx, sizeof(push), &push);
  
  /* Dispatching with 16 workgroups of 64 threads each to cover all 1024 elements. */
  sbgl_DispatchCompute(ctx, (count + 63) / 64, 1, 1);
  
  /* A memory barrier ensures that compute writes are visible to the host before readback. */
  sbgl_MemoryBarrier(ctx, SBGL_BARRIER_COMPUTE_TO_COMPUTE);
  
  sbgl_EndDrawing(ctx);

  /* The system synchronizes with the GPU to ensure all commands have finished. */
  sbgl_DeviceWaitIdle(ctx);

  /* The results are read back from the output buffer and verified for correctness. */
  float* mappedOut = (float*)sbgl_MapBuffer(ctx, bufOut);
  if (!mappedOut) {
    fprintf(stderr, "Failed to map output buffer\n");
    exit(1);
  }

  for (uint32_t i = 0; i < count; i++) {
    float expected = dataA[i] + dataB[i];
    if (mappedOut[i] != expected) {
      fprintf(stderr, "Verification failed at index %u: expected %f, got %f\n", i, expected, mappedOut[i]);
      sbgl_UnmapBuffer(ctx, bufOut);
      exit(1);
    }
  }

  printf("Compute verification successful: 1024 additions verified.\n");

  sbgl_UnmapBuffer(ctx, bufOut);

  /* All resources are released before shutdown. */
  sbgl_DestroyBuffer(ctx, bufA);
  sbgl_DestroyBuffer(ctx, bufB);
  sbgl_DestroyBuffer(ctx, bufOut);
  sbgl_DestroyComputePipeline(ctx, pipeline);
  sbgl_DestroyShader(ctx, computeShader);
  
  free(dataA);
  free(dataB);

  sbgl_Shutdown(ctx);
}

int main(void) {
  test_compute_vector_add();
  printf("All compute tests passed.\n");
  return 0;
}
