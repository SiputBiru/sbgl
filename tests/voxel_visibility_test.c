#include <sbgl.h>
#include <sbgl_camera.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "sbl_arena.h"

/**
 * @brief Push constants for the voxel culling compute shader.
 */
typedef struct {
    sbgl_Mat4 viewProj;
    uint64_t aabbAddress;
    uint64_t commandAddress;
    uint64_t counterAddress;
    uint64_t countsAddress;
} CullPushConstants;

/**
 * Verifies the voxel culling shader by checking visibility from different camera angles.
 */
static void test_voxel_visibility(void) {
  printf("Starting voxel visibility test: Culling and Telemetry...\n");

  /* The system initializes the engine context for compute execution. */
  sbgl_InitResult res = sbgl_Init(100, 100, "Voxel Visibility Test");
  if (res.error != SBGL_SUCCESS) {
    fprintf(stderr, "Failed to initialize SBgl\n");
    exit(1);
  }
  sbgl_Context* ctx = res.ctx;

  /* Loading the culling shader. */
  sbgl_Shader cullShader = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_COMPUTE, "shaders/voxel_cull.comp.spv");
  if (cullShader == SBGL_INVALID_HANDLE) {
    fprintf(stderr, "Failed to load voxel_cull.comp.spv\n");
    sbgl_Shutdown(ctx);
    exit(1);
  }

  /* Creating compute pipeline. */
  sbgl_ComputePipeline cullPipeline = sbgl_CreateComputePipeline(ctx, cullShader);
  if (cullPipeline == SBGL_INVALID_HANDLE) {
    fprintf(stderr, "Failed to create compute pipeline\n");
    exit(1);
  }

  /* Setup slot data. We use 512 slots. */
  uint32_t slotCount = 512;
  SblArena* arena = sbl_get_thread_arena();
  sbgl_AABB* aabbs = SBL_ARENA_PUSH_ARRAY_ZERO(arena, sbgl_AABB, slotCount);
  uint32_t* shellCounts = SBL_ARENA_PUSH_ARRAY_ZERO(arena, uint32_t, slotCount);

  /* Slot 0 is at (10, 10, 10) to (20, 20, 20) with 100 shells. */
  aabbs[0].min = sbgl_Vec3Set(10.0f, 10.0f, 10.0f);
  aabbs[0].max = sbgl_Vec3Set(20.0f, 20.0f, 20.0f);
  shellCounts[0] = 100;

  sbgl_Buffer aabbBuffer = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, slotCount * sizeof(sbgl_AABB), aabbs);
  sbgl_Buffer countsBuffer = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, slotCount * sizeof(uint32_t), shellCounts);

  /* Indirect command buffer. */
  sbgl_Buffer commandBuffer = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE | SBGL_BUFFER_USAGE_INDIRECT, slotCount * sizeof(sbgl_IndirectCommand), NULL);
  
  /* Counter buffer. */
  uint32_t zero = 0;
  sbgl_Buffer counterBuffer = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, sizeof(uint32_t), &zero);

  /* Camera setup. */
  sbgl_Camera cam = sbgl_CameraPerspective(45.0f * (SBGL_PI / 180.0f), 1.0f, 0.1f, 1000.0f);

  CullPushConstants push = {
    .aabbAddress = sbgl_GetBufferDeviceAddress(ctx, aabbBuffer),
    .commandAddress = sbgl_GetBufferDeviceAddress(ctx, commandBuffer),
    .counterAddress = sbgl_GetBufferDeviceAddress(ctx, counterBuffer),
    .countsAddress = sbgl_GetBufferDeviceAddress(ctx, countsBuffer)
  };

  // --- Test Case 1: Looking at the chunk ---
  printf("Test Case 1: Looking at the chunk...\n");
  cam.position = sbgl_Vec3Set(0, 0, 0);
  cam.target = sbgl_Vec3Set(15, 15, 15); // Look towards the chunk
  
  push.viewProj = sbgl_Mat4Mul(sbgl_CameraGetProjection(&cam), sbgl_CameraGetView(&cam));

  sbgl_BeginCompute(ctx);
  sbgl_BindComputePipeline(ctx, cullPipeline);
  sbgl_PushConstants(ctx, sizeof(push), &push);
  sbgl_DispatchCompute(ctx, slotCount / 64, 1, 1);
  sbgl_EndCompute(ctx);
  sbgl_EndDrawing(ctx);
  sbgl_DeviceWaitIdle(ctx);

  uint32_t* countPtr = (uint32_t*)sbgl_MapBuffer(ctx, counterBuffer);
  uint32_t visibleCount = *countPtr;
  sbgl_UnmapBuffer(ctx, counterBuffer);

  printf("Visible slots: %u\n", visibleCount);
  assert(visibleCount == 1);

  sbgl_IndirectCommand* commands = (sbgl_IndirectCommand*)sbgl_MapBuffer(ctx, commandBuffer);
  printf("Command 0 instance count: %u\n", commands[0].instanceCount);
  assert(commands[0].instanceCount == 100);
  sbgl_UnmapBuffer(ctx, commandBuffer);

  // --- Test Case 2: Looking away from the chunk ---
  printf("Test Case 2: Looking away from the chunk...\n");
  cam.position = sbgl_Vec3Set(0, 0, 0);
  cam.target = sbgl_Vec3Set(-15, -15, -15); // Look away
  
  push.viewProj = sbgl_Mat4Mul(sbgl_CameraGetProjection(&cam), sbgl_CameraGetView(&cam));

  /* Reset counter. */
  uint32_t* resetCounter = (uint32_t*)sbgl_MapBuffer(ctx, counterBuffer);
  *resetCounter = 0;
  sbgl_UnmapBuffer(ctx, counterBuffer);

  sbgl_BeginCompute(ctx);
  sbgl_BindComputePipeline(ctx, cullPipeline);
  sbgl_PushConstants(ctx, sizeof(push), &push);
  sbgl_DispatchCompute(ctx, slotCount / 64, 1, 1);
  sbgl_EndCompute(ctx);
  sbgl_EndDrawing(ctx);
  sbgl_DeviceWaitIdle(ctx);

  countPtr = (uint32_t*)sbgl_MapBuffer(ctx, counterBuffer);
  visibleCount = *countPtr;
  sbgl_UnmapBuffer(ctx, counterBuffer);

  printf("Visible slots: %u\n", visibleCount);
  assert(visibleCount == 0);

  printf("Voxel visibility verification successful.\n");

  /* Cleanup. */
  sbgl_DestroyBuffer(ctx, aabbBuffer);
  sbgl_DestroyBuffer(ctx, countsBuffer);
  sbgl_DestroyBuffer(ctx, commandBuffer);
  sbgl_DestroyBuffer(ctx, counterBuffer);
  sbgl_DestroyComputePipeline(ctx, cullPipeline);
  sbgl_DestroyShader(ctx, cullShader);
  
  sbgl_Shutdown(ctx);
}

int main(void) {
  test_voxel_visibility();
  return 0;
}
