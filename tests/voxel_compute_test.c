#include <sbgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "sbl_arena.h"

/**
 * @brief Push constants for the voxel generation compute shader.
 */
typedef struct {
  uint64_t maskAddress;
  float offset[3];
  float seed;
} GenPushConstants;

/**
 * @brief Push constants for the voxel shell extraction compute shader.
 */
typedef struct {
  uint64_t maskAddress;
  uint64_t instanceAddress;
  uint64_t counterAddress;
} ShellPushConstants;

/**
 * Verifies the voxel compute shaders by generating a chunk and extracting its shell.
 */
static void test_voxel_compute(void) {
  printf("Starting voxel compute test: Generation and Shell Extraction...\n");

  /* The system initializes the engine context for compute execution. */
  sbgl_InitResult res = sbgl_Init(100, 100, "Voxel Compute Test");
  if (res.error != SBGL_SUCCESS) {
    fprintf(stderr, "Failed to initialize SBgl\n");
    exit(1);
  }
  sbgl_Context* ctx = res.ctx;

  /* Loading the generation shader. */
  sbgl_Shader genShader = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_COMPUTE, "shaders/voxel_gen.comp.spv");
  if (genShader == SBGL_INVALID_HANDLE) {
    fprintf(stderr, "Failed to load voxel_gen.comp.spv\n");
    sbgl_Shutdown(ctx);
    exit(1);
  }

  /* Loading the shell extraction shader. */
  sbgl_Shader shellShader = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_COMPUTE, "shaders/voxel_shell.comp.spv");
  if (shellShader == SBGL_INVALID_HANDLE) {
    fprintf(stderr, "Failed to load voxel_shell.comp.spv\n");
    sbgl_DestroyShader(ctx, genShader);
    sbgl_Shutdown(ctx);
    exit(1);
  }

  /* Creating compute pipelines. */
  sbgl_ComputePipeline genPipeline = sbgl_CreateComputePipeline(ctx, genShader);
  sbgl_ComputePipeline shellPipeline = sbgl_CreateComputePipeline(ctx, shellShader);
  if (genPipeline == SBGL_INVALID_HANDLE || shellPipeline == SBGL_INVALID_HANDLE) {
    fprintf(stderr, "Failed to create compute pipelines\n");
    exit(1);
  }

  /* Allocating buffers. A 32x32x32 bitmask requires 32768 bits = 4096 bytes. */
  uint32_t maskSize = 4096;
  uint8_t* zeroMask = calloc(1, maskSize);
  sbgl_Buffer maskBuffer = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, maskSize, zeroMask);
  free(zeroMask);
  
  /* Instance buffer should be large enough to hold all potential voxels. */
  uint32_t maxInstances = 32 * 32 * 32;
  sbgl_Buffer instanceBuffer = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, maxInstances * sizeof(uint32_t), NULL);
  
  /* Counter buffer for atomic addition. */
  uint32_t zero = 0;
  sbgl_Buffer counterBuffer = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, sizeof(uint32_t), &zero);

  GenPushConstants genPush = {
    .maskAddress = sbgl_GetBufferDeviceAddress(ctx, maskBuffer),
    .offset = {0.0f, 0.0f, 0.0f},
    .seed = 123.456f
  };

  ShellPushConstants shellPush = {
    .maskAddress = sbgl_GetBufferDeviceAddress(ctx, maskBuffer),
    .instanceAddress = sbgl_GetBufferDeviceAddress(ctx, instanceBuffer),
    .counterAddress = sbgl_GetBufferDeviceAddress(ctx, counterBuffer)
  };

  /* The system records and submits the generation workload using a compute-only frame lifecycle. */
  sbgl_BeginCompute(ctx);
  
  sbgl_BindComputePipeline(ctx, genPipeline);
  sbgl_PushConstants(ctx, sizeof(genPush), &genPush);
  sbgl_DispatchCompute(ctx, 32768 / 64, 1, 1);
  
  sbgl_MemoryBarrier(ctx, SBGL_BARRIER_COMPUTE_TO_COMPUTE);
  
  sbgl_BindComputePipeline(ctx, shellPipeline);
  sbgl_PushConstants(ctx, sizeof(shellPush), &shellPush);
  sbgl_DispatchCompute(ctx, 32768 / 64, 1, 1);
  
  sbgl_MemoryBarrier(ctx, SBGL_BARRIER_COMPUTE_TO_COMPUTE);
  
  sbgl_EndCompute(ctx);
  sbgl_EndDrawing(ctx);

  sbgl_DeviceWaitIdle(ctx);

  /* Verifying the results. */
  uint32_t* countPtr = (uint32_t*)sbgl_MapBuffer(ctx, counterBuffer);
  uint32_t instanceCount = *countPtr;
  sbgl_UnmapBuffer(ctx, counterBuffer);

  printf("Generated %u shell instances.\n", instanceCount);

  /* Reasonable check: it shouldn't be 0 if the seed/noise allows for solid voxels, 
     and it shouldn't be all voxels (32768) because some should be internal. */
  if (instanceCount == 0) {
    fprintf(stderr, "Error: 0 instances generated. Generation might have failed or noise threshold too high.\n");
    exit(1);
  }
  
  if (instanceCount >= 32768) {
    fprintf(stderr, "Error: %u instances generated. Shell extraction failed (too many).\n", instanceCount);
    exit(1);
  }

  printf("Voxel compute verification successful.\n");

  /* Cleanup. */
  sbgl_DestroyBuffer(ctx, maskBuffer);
  sbgl_DestroyBuffer(ctx, instanceBuffer);
  sbgl_DestroyBuffer(ctx, counterBuffer);
  sbgl_DestroyComputePipeline(ctx, genPipeline);
  sbgl_DestroyComputePipeline(ctx, shellPipeline);
  sbgl_DestroyShader(ctx, genShader);
  sbgl_DestroyShader(ctx, shellShader);
  
  sbgl_Shutdown(ctx);
}

int main(void) {
  test_voxel_compute();
  return 0;
}
