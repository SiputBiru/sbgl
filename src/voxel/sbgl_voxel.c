/**
 * @file sbgl_voxel.c
 * @brief Internal implementation of the voxel rendering system.
 */

#include "sbgl_voxel.h"
#include "sbgl_pool.h"
#include "core/sbl_arena.h"
#include "core/sbgl_context_internal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "voxel_gen.h"
#include "voxel_mesh.h"
#include "voxel_cull.h"

/** @brief World-space size of a single voxel chunk in meters. */
#define SBGL_VOXEL_CHUNK_SIZE 256.0f

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
  uint64_t _pad;
  sbgl_Vec4 offset;
  float seed;
  uint32_t _pad2[3];
} sbgl_GenPushConstants;

/**
 * @brief Push constants for the shell extraction compute shader.
 */
typedef struct {
  uint64_t maskAddress;
  uint64_t instanceAddress;
  uint64_t counterAddress;
  uint64_t _pad;
  sbgl_Vec4 offset;
} sbgl_ShellPushConstants;

/**
 * @brief Push constants for the frustum culling and indirect draw compute shader.
 */
typedef struct {
  sbgl_Mat4 viewProj;
  uint64_t aabbAddress;
  uint64_t commandAddress;
  uint64_t countsAddress;
  float _pad[2];
  sbgl_Vec4 cameraPos;
  float maxDistance;
  uint32_t _pad2[3];
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

  /** @brief GPU buffer storing the material palette attributes. */
  sbgl_Buffer materialPaletteBuf;

  /** @brief GPU buffer storing Axis-Aligned Bounding Boxes for culling. Double-buffered. */
  sbgl_Buffer aabbBuf[2];

  /** @brief CPU-side mirror of AABB data to avoid mapping the previous frame's GPU buffer. */
  sbgl_AABB* cpuAABB;

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

  /** @brief Time of the previous update for delta calculation. */
  double last_update_time;
};

sbgl_VoxelSystem* sbgl_Voxel_Create(sbgl_Context* ctx, const sbgl_VoxelConfig* config) {
  /*
   * The voxel system is allocated from the context's persistent arena
   * to remain consistent with the library's memory model.
   */
  SblArena* ctxArena = sbgl_GetContextArena(ctx);
  if (!ctxArena)
    return NULL;

  sbgl_VoxelSystem* sys = SBL_ARENA_PUSH_STRUCT_ZERO(ctxArena, sbgl_VoxelSystem);
  if (!sys)
    return NULL;

  sys->ctx = ctx;
  sys->chunk_radius = config->chunk_radius;
  sys->enable_telemetry = config->enable_telemetry;
  sys->last_update_time = sbgl_GetTime(ctx);

  /* 
   * Initialize the voxel pool with the requested capacity.
   * We utilize a dedicated internal arena for the CPU-side pool management metadata.
   */
  sbl_arena_init(&sys->poolArena, 1 * 1024 * 1024);
  sys->pool = VoxelPool_Init(&sys->poolArena, config->max_slots);

  if (!sys->pool) {
    sbl_arena_free(&sys->poolArena);
    return NULL;
  }

  /*
   * Allocate a CPU-side mirror of the AABB data. This avoids the read-write
   * hazard that occurs when mapping the previous frame's GPU buffer while
   * the GPU may still be consuming it.
   */
  sys->cpuAABB = SBL_ARENA_PUSH_ARRAY_ZERO(&sys->poolArena, sbgl_AABB, config->max_slots);
  if (!sys->cpuAABB) {
    sbl_arena_free(&sys->poolArena);
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
  sbgl_Shader genShader = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_COMPUTE, (const uint32_t*)voxel_gen_comp_spv, voxel_gen_comp_spv_len);
  sbgl_Shader shellShader = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_COMPUTE, (const uint32_t*)voxel_mesh_comp_spv, voxel_mesh_comp_spv_len);
  sbgl_Shader cullShader = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_COMPUTE, (const uint32_t*)voxel_cull_comp_spv, voxel_cull_comp_spv_len);

  if (genShader == SBGL_INVALID_HANDLE || shellShader == SBGL_INVALID_HANDLE || cullShader == SBGL_INVALID_HANDLE) {
    if (genShader != SBGL_INVALID_HANDLE) sbgl_DestroyShader(ctx, genShader);
    if (shellShader != SBGL_INVALID_HANDLE) sbgl_DestroyShader(ctx, shellShader);
    if (cullShader != SBGL_INVALID_HANDLE) sbgl_DestroyShader(ctx, cullShader);
    sbl_arena_free(&sys->poolArena);
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
  sys->maskBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE | SBGL_BUFFER_USAGE_TRANSFER_DST, config->max_slots * 65536, NULL);
  sys->instBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, config->max_slots * 65536 * sizeof(uint32_t) * 2, NULL);
  for (int i = 0; i < 2; ++i) {
    sys->aabbBuf[i] = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, config->max_slots * sizeof(sbgl_AABB), NULL);
    void* mappedAABB = sbgl_MapBuffer(ctx, sys->aabbBuf[i]);
    if (mappedAABB) {
      memset(mappedAABB, 0, config->max_slots * sizeof(sbgl_AABB));
      sbgl_UnmapBuffer(ctx, sys->aabbBuf[i]);
    }
  }

  /*
   * Initialize the material palette with default terrain attributes.
   * This provides a consistent base for procedural generation and shading.
   */
  sys->materialPaletteBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE, 64 * sizeof(sbgl_Material), NULL);
  sbgl_Material* palette = (sbgl_Material*)sbgl_MapBuffer(ctx, sys->materialPaletteBuf);
  if (palette) {
    memset(palette, 0, 64 * sizeof(sbgl_Material));
    
    /* Index 0: Grass (Greenish) */
    palette[0].color = sbgl_Vec4Set(0.3f, 0.6f, 0.2f, 1.0f);
    palette[0].roughness = 0.8f;
    palette[0].metalness = 0.0f;

    /* Index 1: Dirt (Brownish) */
    palette[1].color = sbgl_Vec4Set(0.4f, 0.3f, 0.2f, 1.0f);
    palette[1].roughness = 0.9f;
    palette[1].metalness = 0.0f;

    /* Index 2: Stone (Grey) */
    palette[2].color = sbgl_Vec4Set(0.5f, 0.5f, 0.5f, 1.0f);
    palette[2].roughness = 0.6f;
    palette[2].metalness = 0.0f;

    /* Index 3: Sand (Yellowish) */
    palette[3].color = sbgl_Vec4Set(0.8f, 0.7f, 0.4f, 1.0f);
    palette[3].roughness = 0.7f;
    palette[3].metalness = 0.0f;

    sbgl_UnmapBuffer(ctx, sys->materialPaletteBuf);
  }

  /*
   * Initialize control buffers.
   */
  sys->shellCountsBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_STORAGE | SBGL_BUFFER_USAGE_TRANSFER_DST, config->max_slots * sizeof(uint32_t), NULL);
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
   * The frame index is synchronized with the engine backend to ensure
   * correct double-buffering.
   */
  sys->frame_idx = sbgl_GetFrameIndex(sys->ctx);

  double currentTime = sbgl_GetTime(sys->ctx);
  float dt = (float)(currentTime - sys->last_update_time);
  sys->last_update_time = currentTime;

  sys->telemetry_timer += dt;
  sys->fps_frames++;

  sys->camera_pos = camera_pos;
  int camCX = (int)floorf(camera_pos.x / SBGL_VOXEL_CHUNK_SIZE);
  int camCY = (int)floorf(camera_pos.y / SBGL_VOXEL_CHUNK_SIZE);
  int camCZ = (int)floorf(camera_pos.z / SBGL_VOXEL_CHUNK_SIZE);

  int radius = (int)sys->chunk_radius;

  /* Always refresh timestamps for all slots that are within the visibility radius.
     This prevents the LRU pool from prematurely recycling chunks while the camera 
     is moving within a single chunk or stationary. */
  for (uint32_t i = 0; i < sys->pool->capacity; i++) {
    if (sys->pool->active[i]) {
      sbgl_ivec3 p = sys->pool->positions[i];
      if (abs(p.x - camCX) <= radius && abs(p.z - camCZ) <= radius && p.y >= (camCY - 1) && p.y <= (camCY + 2)) {
        sys->pool->last_used_frames[i] = sys->pool->current_frame;
      }
    }
  }

  /* Update logic is triggered only when the camera crosses a chunk boundary
     to minimize unnecessary CPU/GPU overhead. */
  if (camCX != sys->last_cam_chunk.x || camCY != sys->last_cam_chunk.y || camCZ != sys->last_cam_chunk.z) {
    sbgl_BeginCompute(sys->ctx);

    uint64_t maskBaseAddr = sbgl_GetBufferDeviceAddress(sys->ctx, sys->maskBuf);
    uint64_t instBaseAddr = sbgl_GetBufferDeviceAddress(sys->ctx, sys->instBuf);
    uint64_t shellCountsBaseAddr = sbgl_GetBufferDeviceAddress(sys->ctx, sys->shellCountsBuf);

    sbgl_AABB* aabbPtr = (sbgl_AABB*)sbgl_MapBuffer(sys->ctx, sys->aabbBuf[sys->frame_idx]);

    /* Copy current persistent state from the CPU-side mirror to the new frame's GPU buffer. */
    if (aabbPtr) {
      memcpy(aabbPtr, sys->cpuAABB, sys->pool->capacity * sizeof(sbgl_AABB));
    }

    /* First pass: Acquire and generate new chunks in the radius. */
    for (int dz = -radius; dz <= radius; dz++) {
      for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -1; dy <= 2; dy++) {
          sbgl_ivec3 pos = { camCX + dx, camCY + dy, camCZ + dz };

          bool is_new = false;
          int32_t slot = VoxelPool_AcquireSlot(sys->pool, pos, &is_new);

          if (is_new) {
            uint64_t maskAddr = maskBaseAddr + (slot * 65536);
            uint64_t instAddr = instBaseAddr + (slot * 65536 * 8);
            uint64_t countAddr = shellCountsBaseAddr + (slot * 4);

            /* Clear the presence mask and instance counts using GPU-side commands
               to avoid expensive host-to-device synchronization. */
            sbgl_FillBuffer(sys->ctx, sys->maskBuf, slot * 65536, 65536, 0);
            sbgl_FillBuffer(sys->ctx, sys->shellCountsBuf, slot * 4, 4, 0);

            /* Ensure the GPU-side clears are visible before the compute shaders start.
               SBGL_BARRIER_COMPUTE_TO_COMPUTE now includes TRANSFER synchronization. */
            sbgl_MemoryBarrier(sys->ctx, SBGL_BARRIER_COMPUTE_TO_COMPUTE);

            sbgl_BindComputePipeline(sys->ctx, sys->genPipe);
            sbgl_GenPushConstants gpc = {
              .maskAddress = maskAddr,
              .offset = sbgl_Vec4Set((float)pos.x * SBGL_VOXEL_CHUNK_SIZE, (float)pos.y * SBGL_VOXEL_CHUNK_SIZE, (float)pos.z * SBGL_VOXEL_CHUNK_SIZE, 0.0f),
              .seed = 42.0f
            };
            sbgl_PushConstants(sys->ctx, sizeof(gpc), &gpc);
            sbgl_DispatchCompute(sys->ctx, 4096, 1, 1);
            
            sbgl_MemoryBarrier(sys->ctx, SBGL_BARRIER_COMPUTE_TO_COMPUTE);

            sbgl_BindComputePipeline(sys->ctx, sys->shellPipe);
            sbgl_ShellPushConstants spc = {
              .maskAddress = maskAddr,
              .instanceAddress = instAddr,
              .counterAddress = countAddr,
              .offset = gpc.offset
            };
            sbgl_PushConstants(sys->ctx, sizeof(spc), &spc);
            sbgl_DispatchCompute(sys->ctx, 1, 64, 1);

            sbgl_AABB aabb;
            aabb.min = sbgl_Vec4Set((float)pos.x * SBGL_VOXEL_CHUNK_SIZE, (float)pos.y * SBGL_VOXEL_CHUNK_SIZE, (float)pos.z * SBGL_VOXEL_CHUNK_SIZE, 1.0f);
            aabb.max = sbgl_Vec4Set(aabb.min.x + SBGL_VOXEL_CHUNK_SIZE, aabb.min.y + SBGL_VOXEL_CHUNK_SIZE, aabb.min.z + SBGL_VOXEL_CHUNK_SIZE, 1.0f);
            if (aabbPtr) memcpy(aabbPtr + slot, &aabb, sizeof(sbgl_AABB));
            sys->cpuAABB[slot] = aabb;
          }
        }
      }
    }

    /* Second pass: Deactivate slots that have left the visibility radius. 
       This prevents orphaned chunks from flickering or appearing in the wrong location. */
    for (uint32_t i = 0; i < sys->pool->capacity; i++) {
        if (sys->pool->active[i]) {
            sbgl_ivec3 p = sys->pool->positions[i];
            if (abs(p.x - camCX) > radius || abs(p.z - camCZ) > radius || p.y < (camCY - 1) || p.y > (camCY + 2)) {
                sys->pool->active[i] = 0;
                /* Reset the instance count on the GPU to immediately silence the chunk. */
                sbgl_FillBuffer(sys->ctx, sys->shellCountsBuf, i * 4, 4, 0);
            }
        }
    }

    if (aabbPtr) sbgl_UnmapBuffer(sys->ctx, sys->aabbBuf[sys->frame_idx]);

    /* Ensure that the host-side updates to the AABB buffer are visible to the GPU
       before the culling and graphics phases begin. */
    sbgl_MemoryBarrier(sys->ctx, SBGL_BARRIER_HOST_TO_COMPUTE);
    sbgl_MemoryBarrier(sys->ctx, SBGL_BARRIER_HOST_TO_GRAPHICS);

    /* Ensure that the GPU-side clears from the deactivation pass are visible 
       before the culling phase starts. */
    sbgl_MemoryBarrier(sys->ctx, SBGL_BARRIER_COMPUTE_TO_COMPUTE);

    sbgl_EndCompute(sys->ctx);

    sys->last_cam_chunk = (sbgl_ivec3){ camCX, camCY, camCZ };
  } else {
    /* Even if the camera hasn't crossed a chunk boundary, we must still update the
       AABB buffer for the current frame by copying from the CPU-side mirror.
       This ensures that the culling shader always has a valid set of AABBs. */
    sbgl_AABB* aabbPtr = (sbgl_AABB*)sbgl_MapBuffer(sys->ctx, sys->aabbBuf[sys->frame_idx]);
    if (aabbPtr) {
      memcpy(aabbPtr, sys->cpuAABB, sys->pool->capacity * sizeof(sbgl_AABB));
      sbgl_UnmapBuffer(sys->ctx, sys->aabbBuf[sys->frame_idx]);
    }
    sbgl_MemoryBarrier(sys->ctx, SBGL_BARRIER_HOST_TO_COMPUTE);
    sbgl_MemoryBarrier(sys->ctx, SBGL_BARRIER_HOST_TO_GRAPHICS);
  }

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

  /* Ensure any prior host writes to mapped buffers are visible. */
  sbgl_MemoryBarrier(sys->ctx, SBGL_BARRIER_HOST_TO_COMPUTE);

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
  cpc.aabbAddress = sbgl_GetBufferDeviceAddress(sys->ctx, sys->aabbBuf[sys->frame_idx]);
  cpc.commandAddress = sbgl_GetBufferDeviceAddress(sys->ctx, currCmds);
  cpc.countsAddress = sbgl_GetBufferDeviceAddress(sys->ctx, currCounts);
  cpc.cameraPos = sbgl_Vec4Set(sys->camera_pos.x, sys->camera_pos.y, sys->camera_pos.z, 1.0f);
  cpc.maxDistance = (float)(sys->chunk_radius + 1) * SBGL_VOXEL_CHUNK_SIZE * 1.5f;

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
    uint32_t totalInstances = 0;
    sbgl_IndirectCommand* cmds = (sbgl_IndirectCommand*)sbgl_MapBuffer(sys->ctx, currCmds);
    if (cmds) {
      for (uint32_t i = 0; i < sys->pool->capacity; i++) {
        if (cmds[i].instanceCount > 0) {
          visibleCount++;
          totalInstances += cmds[i].instanceCount;
        }
      }
      sbgl_UnmapBuffer(sys->ctx, currCmds);
    }

    printf("FPS: %u | GPU: %.2fms | Visible: %u/%u | Instances: %u\n", 
           sys->fps_frames, 
           sbgl_GetTelemetry(sys->ctx).gpu_render_time, 
           visibleCount, 
           sys->pool->capacity,
           totalInstances);
    
    sys->telemetry_timer = 0.0f;
    sys->fps_frames = 0;
  }
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
  if (sys->materialPaletteBuf) sbgl_DestroyBuffer(sys->ctx, sys->materialPaletteBuf);
  for (int i = 0; i < 2; ++i) {
    if (sys->aabbBuf[i]) sbgl_DestroyBuffer(sys->ctx, sys->aabbBuf[i]);
  }

  if (sys->shellCountsBuf) sbgl_DestroyBuffer(sys->ctx, sys->shellCountsBuf);
  for (int i = 0; i < 2; ++i) {
    if (sys->indirectCmdBuf[i]) sbgl_DestroyBuffer(sys->ctx, sys->indirectCmdBuf[i]);
  }

  sbl_arena_free(&sys->poolArena);
  /* The sys struct itself is allocated from the context arena and is freed
     when the context is shut down. */
}

uint64_t sbgl_Voxel_GetAABBAddress(sbgl_VoxelSystem* sys) {
  if (!sys) return 0;
  return sbgl_GetBufferDeviceAddress(sys->ctx, sys->aabbBuf[sys->frame_idx]);
}

uint64_t sbgl_Voxel_GetInstanceAddress(sbgl_VoxelSystem* sys) {
  if (!sys) return 0;
  return sbgl_GetBufferDeviceAddress(sys->ctx, sys->instBuf);
}

uint64_t sbgl_Voxel_GetPaletteAddress(sbgl_VoxelSystem* sys) {
  if (!sys) return 0;
  return sbgl_GetBufferDeviceAddress(sys->ctx, sys->materialPaletteBuf);
}

