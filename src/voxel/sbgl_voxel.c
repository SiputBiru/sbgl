/**
 * @file sbgl_voxel.c
 * @brief Internal implementation of the voxel rendering system.
 */

#include "sbgl_voxel.h"
#include "sbgl_pool.h"
#include "core/sbl_arena.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/**
 * @brief Axis-Aligned Bounding Box for frustum culling.
 */
typedef struct {
  sbgl_Vec4 min;
  sbgl_Vec4 max;
} sbgl_AABB;

/**
 * @brief Push constants for the voxel generation compute shader.
 */
typedef struct {
  uint64_t maskAddress;
  sbgl_Vec4 offset;
  float seed;
} sbgl_GenPushConstants;

/**
 * @brief Push constants for the shell extraction compute shader.
 */
typedef struct {
  uint64_t maskAddress;
  uint64_t instanceAddress;
  uint64_t counterAddress;
} sbgl_ShellPushConstants;

/**
 * @brief Push constants for the frustum culling and indirect draw compute shader.
 */
typedef struct {
  sbgl_Mat4 viewProj;
  uint64_t aabbAddress;
  uint64_t commandAddress;
  uint64_t countsAddress;
  sbgl_Vec3 cameraPos;
  float maxDistance;
  uint32_t _padding[3];
} sbgl_CullPushConstants;

/**
 * @brief Internal state for the voxel system.
 * This structure adheres to Data-Oriented Design principles by maintaining
 * contiguous arrays and minimizing pointer indirection.
 */
struct sbgl_VoxelSystem {
  /** @brief Pointer to the active engine context. */
  sbgl_Context* ctx;

  /** @brief Arena for the CPU-side management pool for chunk slots. */
  SblArena poolArena;

  /** @brief Pointer to the CPU-side management pool for chunk slots. */
  VoxelPool* pool;

  /** @brief Radius (in chunks) for visibility and generation logic. */
  uint32_t chunk_radius;

  /** @brief Enables console output for performance metrics. */
  bool enable_telemetry;

  /** @brief Compute pipeline for initial chunk generation. */
  sbgl_ComputePipeline genPipe;

  /** @brief Compute pipeline for shell/boundary processing. */
  sbgl_ComputePipeline shellPipe;

  /** @brief Compute pipeline for frustum culling and indirect command generation. */
  sbgl_ComputePipeline cullPipe;

  /** @brief GPU buffer storing the presence mask for all active chunks. */
  sbgl_Buffer maskBuf;

  /** @brief GPU buffer containing per-instance data for visible chunks. */
  sbgl_Buffer instBuf;

  /** @brief GPU buffer storing Axis-Aligned Bounding Boxes for culling. */
  sbgl_Buffer aabbBuf;

  /** 
   * @brief Double-buffered GPU storage for indirect draw commands. 
   * Double buffering prevents CPU-GPU synchronization stalls during command recording.
   */
  sbgl_Buffer indirectCmdBuf[2];

  /** 
   * @brief GPU storage for shell/active instance counts. 
   */
  sbgl_Buffer shellCountsBuf;

  /** @brief World-space chunk coordinates of the camera in the previous frame. */
  sbgl_ivec3 last_cam_chunk;

  /** @brief Current world-space position of the camera. */
  sbgl_Vec3 camera_pos;

  /** @brief Monotonically increasing index for double-buffer rotation. */
  uint32_t frame_idx;

  /** @brief Total number of frames processed for LRU tracking. */
  uint64_t total_frames;

  /** @brief Timer for periodic telemetry reporting. */
  float telemetry_timer;

  /** @brief Frame counter for FPS calculation. */
  uint32_t fps_frames;
};

sbgl_VoxelSystem* sbgl_Voxel_Create(sbgl_Context* ctx, const sbgl_VoxelConfig* config) {
  /*
   * The voxel system is allocated using malloc to ensure its lifetime 
   * is independent of the calling thread's arena.
   */
  sbgl_VoxelSystem* sys = (sbgl_VoxelSystem*)malloc(sizeof(sbgl_VoxelSystem));

  if (!sys) {
    return NULL;
  }
  memset(sys, 0, sizeof(sbgl_VoxelSystem));

  sys->ctx = ctx;
  sys->chunk_radius = config->chunk_radius;
  sys->enable_telemetry = config->enable_telemetry;
  
  /* 
   * Initialize the voxel pool with the requested capacity.
   * We utilize a dedicated internal arena for the CPU-side pool management metadata.
   */
  sbl_arena_init(&sys->poolArena, 1 * 1024 * 1024);
  sys->pool = VoxelPool_Init(&sys->poolArena, config->max_slots);

  if (!sys->pool) {
    sbl_arena_free(&sys->poolArena);
    free(sys);
    return NULL;
  }

  /*
   * Initialize the camera tracking state to force an initial update.
   */
  sys->last_cam_chunk = (sbgl_ivec3){ -1000000, -1000000, -1000000 };

  /* 
   * Load the compute shaders required for the voxel processing pipeline.
   * These shaders handle generation, shell extraction, and frustum culling.
   */
  sbgl_Shader genShader = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_COMPUTE, "shaders/voxel_gen.comp.spv");
  sbgl_Shader shellShader = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_COMPUTE, "shaders/voxel_shell.comp.spv");
  sbgl_Shader cullShader = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_COMPUTE, "shaders/voxel_cull.comp.spv");

  if (genShader == SBGL_INVALID_HANDLE || shellShader == SBGL_INVALID_HANDLE || cullShader == SBGL_INVALID_HANDLE) {
    if (genShader != SBGL_INVALID_HANDLE) sbgl_DestroyShader(ctx, genShader);
    if (shellShader != SBGL_INVALID_HANDLE) sbgl_DestroyShader(ctx, shellShader);
    if (cullShader != SBGL_INVALID_HANDLE) sbgl_DestroyShader(ctx, cullShader);
    sbl_arena_free(&sys->poolArena);
    free(sys);
    return NULL;
  }

  /*
   * Create the compute pipelines. Once created, the shader modules can be released.
   */
  sys->genPipe = sbgl_CreateComputePipeline(ctx, genShader);
  sys->shellPipe = sbgl_CreateComputePipeline(ctx, shellShader);
  sys->cullPipe = sbgl_CreateComputePipeline(ctx, cullShader);

  sbgl_DestroyShader(ctx, genShader);
  sbgl_DestroyShader(ctx, shellShader);
  sbgl_DestroyShader(ctx, cullShader);

  /* 
   * Allocate the GPU-side buffers. The mask and instance buffers store the voxel
   * data, while the AABB buffer is used for GPU-driven culling.
   */
  sys->maskBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, config->max_slots * 65536, NULL);
  sys->instBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, config->max_slots * 65536 * sizeof(uint32_t), NULL);
  sys->aabbBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, config->max_slots * sizeof(sbgl_AABB), NULL);

  /*
   * Initialize control buffers.
   */
  sys->shellCountsBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, config->max_slots * sizeof(uint32_t), NULL);
  void* mappedShell = sbgl_MapBuffer(ctx, sys->shellCountsBuf);
  if (mappedShell) {
    memset(mappedShell, 0, config->max_slots * sizeof(uint32_t));
    sbgl_UnmapBuffer(ctx, sys->shellCountsBuf);
  }

  for (int i = 0; i < 2; ++i) {
    sys->indirectCmdBuf[i] = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE | SBGL_BUFFER_USAGE_INDIRECT, config->max_slots * sizeof(sbgl_IndirectCommand), NULL);

    void* mappedCmd = sbgl_MapBuffer(ctx, sys->indirectCmdBuf[i]);
    if (mappedCmd) {
      memset(mappedCmd, 0, config->max_slots * sizeof(sbgl_IndirectCommand));
      sbgl_UnmapBuffer(ctx, sys->indirectCmdBuf[i]);
    }
  }

  return sys;
}

void sbgl_Voxel_Update(sbgl_VoxelSystem* sys, sbgl_Vec3 camera_pos) {
  /*
   * The update phase performs camera-relative chunk management.
   * It identifies chunks that have entered the visibility radius and 
   * schedules them for generation in the compute pipelines.
   * 
   * Update logic is triggered only when the camera crosses a chunk boundary
   * to minimize unnecessary CPU/GPU overhead.
   */
  static double lastTime = 0;
  double currentTime = sbgl_GetTime(sys->ctx);
  if (lastTime == 0) lastTime = currentTime;
  float dt = (float)(currentTime - lastTime);
  lastTime = currentTime;

  sys->telemetry_timer += dt;
  sys->fps_frames++;

  sys->camera_pos = camera_pos;
  const float CHUNK_SIZE_F = 256.0f;
  int camCX = (int)floorf(camera_pos.x / CHUNK_SIZE_F);
  int camCY = (int)floorf(camera_pos.y / CHUNK_SIZE_F);
  int camCZ = (int)floorf(camera_pos.z / CHUNK_SIZE_F);

  if (camCX != sys->last_cam_chunk.x || camCY != sys->last_cam_chunk.y || camCZ != sys->last_cam_chunk.z) {
    /*
     * When a boundary is crossed, the system dispatches compute shaders to 
     * generate and process chunks within the specified radius.
     */
    sbgl_BeginCompute(sys->ctx);

    /*
     * Fetch base device addresses once to avoid redundant HAL calls during the 
     * chunk scanning loop.
     */
    uint64_t maskBaseAddr = sbgl_GetBufferDeviceAddress(sys->ctx, sys->maskBuf);
    uint64_t instBaseAddr = sbgl_GetBufferDeviceAddress(sys->ctx, sys->instBuf);
    uint64_t shellCountsBaseAddr = sbgl_GetBufferDeviceAddress(sys->ctx, sys->shellCountsBuf);

    /*
     * Map buffers to allow for localized CPU-side clearing and AABB updates.
     */
    uint32_t* mPtr = (uint32_t*)sbgl_MapBuffer(sys->ctx, sys->maskBuf);
    uint32_t* cPtr = (uint32_t*)sbgl_MapBuffer(sys->ctx, sys->shellCountsBuf);
    sbgl_AABB* aabbPtr = (sbgl_AABB*)sbgl_MapBuffer(sys->ctx, sys->aabbBuf);

    int radius = (int)sys->chunk_radius;
    for (int dz = -radius; dz <= radius; dz++) {
      for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
          sbgl_ivec3 pos = { camCX + dx, camCY + dy, camCZ + dz };

          bool is_new = false;
          int32_t slot = VoxelPool_AcquireSlot(sys->pool, pos, &is_new);

          if (is_new) {
            /*
             * For new or recycled slots, clear existing GPU state and dispatch 
             * generation and shell extraction pipelines.
             */
            uint64_t maskAddr = maskBaseAddr + (slot * 65536);
            uint64_t instAddr = instBaseAddr + (slot * 65536 * 4);
            uint64_t countAddr = shellCountsBaseAddr + (slot * 4);

            memset(mPtr + (slot * 16384), 0, 65536);
            cPtr[slot] = 0;

            sbgl_BindComputePipeline(sys->ctx, sys->genPipe);
            sbgl_GenPushConstants gpc = {
              .maskAddress = maskAddr,
              .offset = sbgl_Vec4Set((float)pos.x * 64.0f, (float)pos.y * 64.0f, (float)pos.z * 64.0f, 0.0f),
              .seed = 42.0f
            };
            sbgl_PushConstants(sys->ctx, sizeof(gpc), &gpc);
            sbgl_DispatchCompute(sys->ctx, 4096, 1, 1);
            
            /*
             * Barrier ensures that generation is complete before shell extraction begins.
             */
            sbgl_MemoryBarrier(sys->ctx, SBGL_BARRIER_COMPUTE_TO_COMPUTE);

            sbgl_BindComputePipeline(sys->ctx, sys->shellPipe);
            sbgl_ShellPushConstants spc = {
              .maskAddress = maskAddr,
              .instanceAddress = instAddr,
              .counterAddress = countAddr
            };
            sbgl_PushConstants(sys->ctx, sizeof(spc), &spc);
            sbgl_DispatchCompute(sys->ctx, 4096, 1, 1);

            /*
             * Update the AABB on the CPU for efficient frustum culling in the render phase.
             */
            sbgl_AABB aabb;
            aabb.min = sbgl_Vec4Set((float)pos.x * CHUNK_SIZE_F, (float)pos.y * CHUNK_SIZE_F, (float)pos.z * CHUNK_SIZE_F, 1.0f);
            aabb.max = sbgl_Vec4Set(aabb.min.x + CHUNK_SIZE_F, aabb.min.y + CHUNK_SIZE_F, aabb.min.z + CHUNK_SIZE_F, 1.0f);
            memcpy(aabbPtr + slot, &aabb, sizeof(sbgl_AABB));
          }
        }
      }
    }

    sbgl_UnmapBuffer(sys->ctx, sys->aabbBuf);
    sbgl_UnmapBuffer(sys->ctx, sys->shellCountsBuf);
    sbgl_UnmapBuffer(sys->ctx, sys->maskBuf);

    sbgl_EndCompute(sys->ctx);

    sys->last_cam_chunk = (sbgl_ivec3){ camCX, camCY, camCZ };
  }

  /*
   * Advance the pool tracking for LRU management.
   */
  VoxelPool_UpdateFrame(sys->pool, sys->total_frames++);
}

void sbgl_Voxel_Cull(sbgl_VoxelSystem* sys, sbgl_Mat4 view_proj) {
  /*
   * Voxel culling follows a GPU-driven pipeline. A compute shader performs 
   * frustum culling on all managed chunks, populating an indirect draw buffer 
   * with commands for only the visible portions of the world.
   * This is done outside the main render pass to avoid validation errors.
   */
  sbgl_BeginCompute(sys->ctx);

  /*
   * Retrieve the double-buffered GPU handles for the current frame.
   */
  sbgl_Buffer currCmds = sys->indirectCmdBuf[sys->frame_idx];
  sbgl_Buffer currCounts = sys->shellCountsBuf;

  /*
   * Bind the culling pipeline and upload the camera and frustum metadata via 
   * push constants.
   */
  sbgl_BindComputePipeline(sys->ctx, sys->cullPipe);

  sbgl_CullPushConstants cpc;
  cpc.viewProj = view_proj;
  cpc.aabbAddress = sbgl_GetBufferDeviceAddress(sys->ctx, sys->aabbBuf);
  cpc.commandAddress = sbgl_GetBufferDeviceAddress(sys->ctx, currCmds);
  cpc.countsAddress = sbgl_GetBufferDeviceAddress(sys->ctx, currCounts);
  cpc.cameraPos = sys->camera_pos;
  cpc.maxDistance = (float)(sys->chunk_radius + 1) * 256.0f;

  sbgl_PushConstants(sys->ctx, sizeof(cpc), &cpc);

  /*
   * Dispatch the culling kernel. Each workgroup processes a subset of the 
   * total chunk slots available in the pool.
   */
  uint32_t groupCount = (sys->pool->capacity + 63) / 64;
  sbgl_DispatchCompute(sys->ctx, groupCount, 1, 1);

  /*
   * Inject memory barriers to ensure that the indirect command buffer and 
   * graphics pipelines can safely consume the results of the compute culling.
   */
  sbgl_MemoryBarrier(sys->ctx, SBGL_BARRIER_COMPUTE_TO_INDIRECT);
  sbgl_MemoryBarrier(sys->ctx, SBGL_BARRIER_COMPUTE_TO_GRAPHICS);

  sbgl_EndCompute(sys->ctx);
}

void sbgl_Voxel_Render(sbgl_VoxelSystem* sys) {
  /*
   * Issue the multi-draw indirect call. The GPU will execute the array of 
   * commands generated by the culling phase in a single submission.
   * This must be called inside the dynamic rendering pass.
   */
  sbgl_Buffer currCmds = sys->indirectCmdBuf[sys->frame_idx];
  sbgl_DrawIndirect(sys->ctx, currCmds, 0, sys->pool->capacity);

  /*
   * Handle telemetry reporting if enabled.
   */
  if (sys->enable_telemetry && sys->telemetry_timer >= 1.0f) {
    uint32_t visibleCount = 0;
    sbgl_IndirectCommand* cmds = (sbgl_IndirectCommand*)sbgl_MapBuffer(sys->ctx, currCmds);
    if (cmds) {
      for (uint32_t i = 0; i < sys->pool->capacity; i++) {
        if (cmds[i].instanceCount > 0) visibleCount++;
      }
      sbgl_UnmapBuffer(sys->ctx, currCmds);
    }

    printf("FPS: %u | GPU: %.2fms | Visible: %u/%u\n", 
           sys->fps_frames, 
           sbgl_GetTelemetry(sys->ctx).gpu_render_time, 
           visibleCount, 
           sys->pool->capacity);
    
    sys->telemetry_timer = 0.0f;
    sys->fps_frames = 0;
  }

  /*
   * Rotate the frame index for the next update/render cycle.
   */
  sys->frame_idx = (sys->frame_idx + 1) % 2;
}

void sbgl_Voxel_Destroy(sbgl_VoxelSystem* sys) {
  /*
   * GPU resources are released back to the driver before the context 
   * handle itself is discarded.
   */
  if (!sys) return;

  /*
   * Ensure the GPU is no longer accessing the resources before destruction.
   */
  sbgl_DeviceWaitIdle(sys->ctx);

  if (sys->genPipe) sbgl_DestroyComputePipeline(sys->ctx, sys->genPipe);
  if (sys->shellPipe) sbgl_DestroyComputePipeline(sys->ctx, sys->shellPipe);
  if (sys->cullPipe) sbgl_DestroyComputePipeline(sys->ctx, sys->cullPipe);

  if (sys->maskBuf) sbgl_DestroyBuffer(sys->ctx, sys->maskBuf);
  if (sys->instBuf) sbgl_DestroyBuffer(sys->ctx, sys->instBuf);
  if (sys->aabbBuf) sbgl_DestroyBuffer(sys->ctx, sys->aabbBuf);

  if (sys->shellCountsBuf) sbgl_DestroyBuffer(sys->ctx, sys->shellCountsBuf);
  for (int i = 0; i < 2; ++i) {
    if (sys->indirectCmdBuf[i]) sbgl_DestroyBuffer(sys->ctx, sys->indirectCmdBuf[i]);
  }

  sbl_arena_free(&sys->poolArena);
  free(sys);
}

uint64_t sbgl_Voxel_GetAABBAddress(sbgl_VoxelSystem* sys) {
  if (!sys) return 0;
  return sbgl_GetBufferDeviceAddress(sys->ctx, sys->aabbBuf);
}

uint64_t sbgl_Voxel_GetInstanceAddress(sbgl_VoxelSystem* sys) {
  if (!sys) return 0;
  return sbgl_GetBufferDeviceAddress(sys->ctx, sys->instBuf);
}
