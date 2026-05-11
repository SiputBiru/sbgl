/**
 * @file voxel3D_main.c
 * @brief Zero-Sync Infinite Voxel Engine Flagship Example.
 */

#include <sbgl.h>
#include <sbgl_camera.h>
#include <sbgl_math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "voxel_pool.h"

#define MAX_SLOTS 512
#define CHUNK_RADIUS 4
#define CHUNK_SIZE 64
#define VOXEL_CHUNK_SIZE 256.0f // 64 voxels * 4.0 scale
#define VOXELS_PER_CHUNK (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)
#define INSTANCE_CAPACITY 65536

typedef struct { sbgl_Vec4 min; sbgl_Vec4 max; } AABB;

typedef struct {
  uint64_t maskAddress;
  sbgl_Vec4 offset;
  float seed;
} GenPushConstants;

typedef struct {
  uint64_t maskAddress;
  uint64_t instanceAddress;
  uint64_t counterAddress;
} ShellPushConstants;

typedef struct {
  sbgl_Mat4 viewProj;
  uint64_t aabbAddress;
  uint64_t commandAddress;
  uint64_t countsAddress;
} CullPushConstants;

typedef struct {
  sbgl_Mat4 viewProj;
  uint64_t aabbAddress;
  uint64_t voxelDataAddress;
} RenderPushConstants;

typedef struct { uint32_t indexCount; uint32_t instanceCount; uint32_t firstIndex; int32_t vertexOffset; uint32_t firstInstance; } IndirectCommand;

int main(void) {
  sbgl_InitResult res = sbgl_Init(1280, 720, "SBgl Zero-Sync Infinite Voxels");
  if (res.error != SBGL_SUCCESS) return 1;
  sbgl_Context* ctx = res.ctx;

  SblArena arena;
  sbl_arena_init(&arena, 64 * 1024 * 1024);

  VoxelPool* pool = VoxelPool_Init(&arena, MAX_SLOTS);
  if (!pool) {
    fprintf(stderr, "Failed to initialize VoxelPool\n");
    sbl_arena_free(&arena);
    sbgl_Shutdown(ctx);
    return 1;
  }

  // Load Shaders
  sbgl_Shader genShader = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_COMPUTE, "shaders/voxel_gen.comp.spv");
  sbgl_Shader shellShader = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_COMPUTE, "shaders/voxel_shell.comp.spv");
  sbgl_Shader cullShader = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_COMPUTE, "shaders/voxel_cull.comp.spv");
  sbgl_Shader vertShader = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_VERTEX, "shaders/voxel3D.vert.spv");
  sbgl_Shader fragShader = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_FRAGMENT, "shaders/voxel3D.frag.spv");

  if (genShader == SBGL_INVALID_HANDLE || shellShader == SBGL_INVALID_HANDLE || cullShader == SBGL_INVALID_HANDLE ||
      vertShader == SBGL_INVALID_HANDLE || fragShader == SBGL_INVALID_HANDLE) {
    fprintf(stderr, "Failed to load shaders\n");
    sbl_arena_free(&arena);
    sbgl_Shutdown(ctx);
    return 1;
  }

  sbgl_ComputePipeline genPipe = sbgl_CreateComputePipeline(ctx, genShader);
  sbgl_ComputePipeline shellPipe = sbgl_CreateComputePipeline(ctx, shellShader);
  sbgl_ComputePipeline cullPipe = sbgl_CreateComputePipeline(ctx, cullShader);
  sbgl_Pipeline renderPipe = sbgl_CreatePipeline(ctx, &(sbgl_PipelineConfig){ .vertexShader = vertShader, .fragmentShader = fragShader });

  if (renderPipe == SBGL_INVALID_HANDLE) {
    fprintf(stderr, "Failed to create graphics pipeline\n");
    sbl_arena_free(&arena);
    sbgl_Shutdown(ctx);
    return 1;
  }

  // Buffers
  sbgl_Buffer maskBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, MAX_SLOTS * 65536, NULL);
  sbgl_Buffer instBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, MAX_SLOTS * 65536 * sizeof(uint32_t), NULL);
  sbgl_Buffer shellCountsBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, MAX_SLOTS * sizeof(uint32_t), NULL);
  sbgl_Buffer aabbBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, MAX_SLOTS * sizeof(AABB), NULL);
  sbgl_Buffer indirectCmdBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE | SBGL_BUFFER_USAGE_INDIRECT, MAX_SLOTS * sizeof(IndirectCommand), NULL);

  // Initial zeroing of control buffers
  {
    uint32_t* cPtr = (uint32_t*)sbgl_MapBuffer(ctx, shellCountsBuf);
    memset(cPtr, 0, MAX_SLOTS * sizeof(uint32_t));
    sbgl_UnmapBuffer(ctx, shellCountsBuf);

    IndirectCommand* iPtr = (IndirectCommand*)sbgl_MapBuffer(ctx, indirectCmdBuf);
    memset(iPtr, 0, MAX_SLOTS * sizeof(IndirectCommand));
    sbgl_UnmapBuffer(ctx, indirectCmdBuf);
  }

  uint32_t idxs[36]; for (uint32_t i = 0; i < 36; i++) idxs[i] = i;
  sbgl_Buffer iBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_INDEX, sizeof(idxs), idxs);

  sbgl_Camera camera = sbgl_CameraPerspective(0.8f, 1280.0f / 720.0f, 0.1f, 10000.0f);
  camera.position = sbgl_Vec3Set(0.0f, 400.0f, -800.0f);
  camera.target = sbgl_Vec3Set(0.0f, 128.0f, 0.0f); 

  float pitch = -0.4f, yaw = SBGL_PI / 2.0f;
  bool mouseLocked = false;
  float moveSpeed = 400.0f, sensitivity = 0.005f;

  uint64_t frameCounter = 0;
  double lastTime = sbgl_GetTime(ctx);
  float telemetryTimer = 0.0f;
  int fpsFrames = 0;
  uint32_t visibleCount = 0;

  printf("--- Voxel Controls ---\n");
  printf("W/A/S/D: Move\n");
  printf("Q/E: Vertical Move\n");
  printf("TAB: Lock/Unlock Mouse\n");
  printf("ESC: Exit\n");
  printf("----------------------\n");

  while (!sbgl_WindowShouldClose(ctx)) {
    double currentTime = sbgl_GetTime(ctx);
    float dt = (float)(currentTime - lastTime);
    lastTime = currentTime;

    sbgl_BeginCompute(ctx);

    const sbgl_InputState* input = sbgl_GetInputState(ctx);
    if (input->keysDown[SBGL_KEY_ESCAPE]) break;

    if (input->keysPressed[SBGL_KEY_TAB]) {
      printf("[Input] TAB Pressed! Toggling Mouse Lock...\n");
      mouseLocked = !mouseLocked;
      sbgl_SetMouseMode(ctx, mouseLocked ? SBGL_MOUSE_MODE_CAPTURED : SBGL_MOUSE_MODE_NORMAL);
    }

    if (mouseLocked) {
      yaw += (float)input->mouseDeltaX * sensitivity;
      pitch -= (float)input->mouseDeltaY * sensitivity;
      if (pitch > 1.5f) pitch = 1.5f;
      if (pitch < -1.5f) pitch = -1.5f;
    }

    sbgl_Vec3 front = sbgl_Vec3Normalize(sbgl_Vec3Set(
      cosf(yaw) * cosf(pitch),
      sinf(pitch),
      sinf(yaw) * cosf(pitch)
    ));
    sbgl_Vec3 right = sbgl_Vec3Normalize(sbgl_Vec3Cross(front, sbgl_Vec3Set(0, 1, 0)));
    float velocity = moveSpeed * dt;

    if (input->keysDown[SBGL_KEY_W]) camera.position = sbgl_Vec3Add(camera.position, sbgl_Vec3Mul(front, velocity));
    if (input->keysDown[SBGL_KEY_S]) camera.position = sbgl_Vec3Sub(camera.position, sbgl_Vec3Mul(front, velocity));
    if (input->keysDown[SBGL_KEY_A]) camera.position = sbgl_Vec3Sub(camera.position, sbgl_Vec3Mul(right, velocity));
    if (input->keysDown[SBGL_KEY_D]) camera.position = sbgl_Vec3Add(camera.position, sbgl_Vec3Mul(right, velocity));
    if (input->keysDown[SBGL_KEY_Q]) camera.position.y -= velocity;
    if (input->keysDown[SBGL_KEY_E]) camera.position.y += velocity;

    camera.target = sbgl_Vec3Add(camera.position, front);
    camera.up = sbgl_Vec3Normalize(sbgl_Vec3Cross(right, front));

    int width, height;
    sbgl_GetWindowSize(ctx, &width, &height);
    camera.aspect = (float)width / (float)height;

    sbgl_Mat4 viewProj = sbgl_Mat4Mul(sbgl_CameraGetProjection(&camera), sbgl_CameraGetView(&camera));

    VoxelPool_UpdateFrame(pool, frameCounter);

    int camCX = (int)floorf(camera.position.x / VOXEL_CHUNK_SIZE);
    int camCY = (int)floorf(camera.position.y / VOXEL_CHUNK_SIZE);
    int camCZ = (int)floorf(camera.position.z / VOXEL_CHUNK_SIZE);

    uint32_t* mPtr = (uint32_t*)sbgl_MapBuffer(ctx, maskBuf);
    uint32_t* cPtr = (uint32_t*)sbgl_MapBuffer(ctx, shellCountsBuf);
    AABB* aabbPtr = (AABB*)sbgl_MapBuffer(ctx, aabbBuf);
    IndirectCommand* cmdPtr = (IndirectCommand*)sbgl_MapBuffer(ctx, indirectCmdBuf);

    visibleCount = 0;
    for (uint32_t i = 0; i < MAX_SLOTS; i++) {
      if (cmdPtr[i].instanceCount > 0) visibleCount++;
    }

    // Scan radius around camera
    for (int dz = -CHUNK_RADIUS; dz <= CHUNK_RADIUS; dz++) {
      for (int dx = -CHUNK_RADIUS; dx <= CHUNK_RADIUS; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
          sbgl_ivec3 pos = { camCX + dx, camCY + dy, camCZ + dz };
          int32_t slot = -1;
          for (uint32_t i = 0; i < pool->capacity; i++) {
            if (pool->active[i] && pool->positions[i].x == pos.x && pool->positions[i].y == pos.y && pool->positions[i].z == pos.z) {
              slot = i;
              pool->last_used_frames[i] = pool->current_frame;
              break;
            }
          }
          
          if (slot == -1) {
            slot = VoxelPool_AcquireSlot(pool, pos);
            uint64_t maskAddr = sbgl_GetBufferDeviceAddress(ctx, maskBuf) + (slot * 65536);
            uint64_t instAddr = sbgl_GetBufferDeviceAddress(ctx, instBuf) + (slot * 65536 * 4);
            uint64_t countAddr = sbgl_GetBufferDeviceAddress(ctx, shellCountsBuf) + (slot * 4);
            
            memset(mPtr + (slot * 16384), 0, 65536);
            cPtr[slot] = 0;
            memset(&cmdPtr[slot], 0, sizeof(IndirectCommand));

            sbgl_BindComputePipeline(ctx, genPipe);
            GenPushConstants gpc = {
              .maskAddress = maskAddr,
              .offset = { { (float)pos.x * 64.0f, (float)pos.y * 64.0f, (float)pos.z * 64.0f, 0.0f } },
              .seed = 42.0f
            };
            sbgl_PushConstants(ctx, sizeof(gpc), &gpc);
            sbgl_DispatchCompute(ctx, 4096, 1, 1);
            sbgl_MemoryBarrier(ctx, SBGL_BARRIER_COMPUTE_TO_COMPUTE);
            
            sbgl_BindComputePipeline(ctx, shellPipe);
            ShellPushConstants spc = { .maskAddress = maskAddr, .instanceAddress = instAddr, .counterAddress = countAddr };
            sbgl_PushConstants(ctx, sizeof(spc), &spc);
            sbgl_DispatchCompute(ctx, 4096, 1, 1);
            
            AABB aabb;
            aabb.min = sbgl_Vec4Set((float)pos.x * VOXEL_CHUNK_SIZE, (float)pos.y * VOXEL_CHUNK_SIZE, (float)pos.z * VOXEL_CHUNK_SIZE, 1.0f);
            aabb.max = sbgl_Vec4Set(aabb.min.x + VOXEL_CHUNK_SIZE, aabb.min.y + VOXEL_CHUNK_SIZE, aabb.min.z + VOXEL_CHUNK_SIZE, 1.0f);
            memcpy(aabbPtr + slot, &aabb, sizeof(AABB));
          }
        }
      }
    }
    
    sbgl_UnmapBuffer(ctx, indirectCmdBuf);
    sbgl_UnmapBuffer(ctx, aabbBuf);
    sbgl_UnmapBuffer(ctx, shellCountsBuf);
    sbgl_UnmapBuffer(ctx, maskBuf);
    
    sbgl_MemoryBarrier(ctx, SBGL_BARRIER_COMPUTE_TO_COMPUTE);
    
    sbgl_BindComputePipeline(ctx, cullPipe);
    CullPushConstants cpc = {
      .viewProj = viewProj,
      .aabbAddress = sbgl_GetBufferDeviceAddress(ctx, aabbBuf),
      .commandAddress = sbgl_GetBufferDeviceAddress(ctx, indirectCmdBuf),
      .countsAddress = sbgl_GetBufferDeviceAddress(ctx, shellCountsBuf)
    };
    sbgl_PushConstants(ctx, sizeof(cpc), &cpc);
    sbgl_DispatchCompute(ctx, 8, 1, 1); 
    
    sbgl_MemoryBarrier(ctx, SBGL_BARRIER_COMPUTE_TO_INDIRECT);
    sbgl_MemoryBarrier(ctx, SBGL_BARRIER_COMPUTE_TO_GRAPHICS);
    
    sbgl_EndCompute(ctx);
    
    sbgl_Clear(ctx, 0.5f, 0.7f, 1.0f, 1.0f);
    sbgl_BeginDrawing(ctx);
    sbgl_BindPipeline(ctx, renderPipe);
    RenderPushConstants rpc = {
      .viewProj = viewProj,
      .aabbAddress = sbgl_GetBufferDeviceAddress(ctx, aabbBuf),
      .voxelDataAddress = sbgl_GetBufferDeviceAddress(ctx, instBuf)
    };
    sbgl_PushConstants(ctx, sizeof(rpc), &rpc);
    sbgl_BindBuffer(ctx, iBuf, SBGL_BUFFER_USAGE_INDEX);
    sbgl_DrawIndirect(ctx, indirectCmdBuf, 0, MAX_SLOTS);
    sbgl_EndDrawing(ctx);
    
    frameCounter++;
    fpsFrames++;
    telemetryTimer += dt;
    if (telemetryTimer >= 1.0f) {
      printf("FPS: %d | GPU: %.2fms | Visible: %d/%d\n", fpsFrames, sbgl_GetTelemetry(ctx).gpu_render_time, visibleCount, MAX_SLOTS);
      fpsFrames = 0;
      telemetryTimer = 0.0f;
    }
  }

  sbgl_DeviceWaitIdle(ctx);
  sbgl_DestroyComputePipeline(ctx, genPipe);
  sbgl_DestroyComputePipeline(ctx, shellPipe);
  sbgl_DestroyComputePipeline(ctx, cullPipe);
  sbgl_DestroyPipeline(ctx, renderPipe);
  sbgl_DestroyShader(ctx, genShader);
  sbgl_DestroyShader(ctx, shellShader);
  sbgl_DestroyShader(ctx, cullShader);
  sbgl_DestroyShader(ctx, vertShader);
  sbgl_DestroyShader(ctx, fragShader);
  sbgl_DestroyBuffer(ctx, maskBuf);
  sbgl_DestroyBuffer(ctx, instBuf);
  sbgl_DestroyBuffer(ctx, shellCountsBuf);
  sbgl_DestroyBuffer(ctx, aabbBuf);
  sbgl_DestroyBuffer(ctx, indirectCmdBuf);
  sbgl_DestroyBuffer(ctx, iBuf);
  sbl_arena_free(&arena);
  sbgl_Shutdown(ctx);
  return 0;
}
