#include <core/sbl_arena.h>
#include <math.h>
#include <sbgl.h>
#include <sbgl_camera.h>
#include <sbgl_math.h>
#include <sbgl_types.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define STB_PERLIN_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#include "stb_perlin.h"
#pragma GCC diagnostic pop

typedef struct {
    sbgl_Mat4 viewProj;
    uint64_t instanceAddress; // for Instances
    uint64_t heightAddress;      // for SSBO
} VoxelPushConstants;

// Verified logic functions for Dynamic Radius Control
static const int VOXEL_CHUNK_DIM = 32;
static const float VOXEL_CHUNK_SIZE = 32.0f;
static const int VOXEL_WORLD_SIZE = 2048;
static const float VOXEL_FAR_PLANE_MARGIN = 1.5f;

static int calculate_instance_count(int radius) {
    int width = (radius * 2 + 1);
    return width * width;
}

static int update_radius(int current, int delta) {
    int next = current + delta;
    if (next < 1)
        return 1;
    if (next > 50)
        return 50;
    return next;
}

static float calculate_far_plane(int radius) {
    return (float)(radius + 1) * VOXEL_CHUNK_SIZE * VOXEL_FAR_PLANE_MARGIN;
}

static void apply_radius_change(sbgl_Camera* camera, int* radius, int* instance_count, int delta) {
    *radius = update_radius(*radius, delta);
    *instance_count = calculate_instance_count(*radius);
    camera->far_plane = calculate_far_plane(*radius);
    printf(
        "Render Radius: %d (%d chunks), Far Plane: %.1f\n",
        *radius,
        *instance_count,
        camera->far_plane
    );
}

int main(void) {
    /* Initialize the graphics context and window. */
    sbgl_InitResult res = sbgl_Init(1280, 720, "SBgl Procedural Voxels");
    if (res.error != SBGL_SUCCESS)
        return 1;
    sbgl_Context* ctx = res.ctx;

    /* Generating world things */
    float* height_data = malloc(sizeof(float) * VOXEL_WORLD_SIZE * VOXEL_WORLD_SIZE);

    for (int z = 0; z < VOXEL_WORLD_SIZE; z++) {
        for (int x = 0; x < VOXEL_WORLD_SIZE; x++) {
            float noise = 0;
            float amp = 1.0f;
            for (int oct = 0; oct < 6; oct++) {
                // To make the noise periodic at VOXEL_WORLD_SIZE=2048, 
                // the period in the noise coordinate space must be a power of two.
                // A base period of 16 units is selected for the first octave.
                // 16.0 / 2048.0 = 0.0078125
                float base_freq = 16.0f / (float)VOXEL_WORLD_SIZE;
                float oct_freq = base_freq * (float)(1 << oct);                
                // The wrap parameter must match the period of the current octave.
                // For oct=0, period=16. For oct=5, period=16*32 = 512.
                // However, stb_perlin_noise3 wraps internally at 256.
                // The wrap is capped at 256 to remain within STB's valid range.
                int wrap = 16 << oct;
                if (wrap > 256) wrap = 256;

                noise += stb_perlin_noise3(
                             (float)x * oct_freq,
                             (float)z * oct_freq,
                             0,
                             wrap,
                             wrap,
                             wrap
                         ) *
                         amp;
                amp *= 0.5f;
            }
            height_data[z * VOXEL_WORLD_SIZE + x] = floorf((noise + 1.0f) * 0.5f * 64.0f);
        }
    }

    sbgl_Buffer height_ssbo = sbgl_CreateBuffer(
        ctx,
        SBGL_BUFFER_USAGE_STORAGE,
        sizeof(float) * VOXEL_WORLD_SIZE * VOXEL_WORLD_SIZE,
        height_data
    );
    free(height_data);

    /* Initialize the memory arena. */
    SblArena arena;
    sbl_arena_init(&arena, 32 * 1024 * 1024);

    /* Load the procedural voxel shaders. */
    sbgl_Shader vert =
        sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_VERTEX, "shaders/voxel.vert.spv");
    sbgl_Shader frag =
        sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_FRAGMENT, "shaders/voxel.frag.spv");

    /* Create a dummy VBO. Pure procedural mode does not require one, but binding ensures safety. */
    sbgl_Vertex dummy_vert = { sbgl_Vec3Set(0, 0, 0), sbgl_Vec3Set(0, 0, 0) };
    sbgl_Buffer vbo =
        sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_VERTEX, sizeof(dummy_vert), &dummy_vert);

    /* Create a linear index buffer (0, 1, 2...) for procedural generation. */
    const uint32_t PRO_INDEX_COUNT = VOXEL_CHUNK_DIM * VOXEL_CHUNK_DIM * 36;
    uint32_t* pro_indices = malloc(sizeof(uint32_t) * PRO_INDEX_COUNT);
    for (uint32_t i = 0; i < PRO_INDEX_COUNT; i++)
        pro_indices[i] = i;
    sbgl_Buffer pro_ibo = sbgl_CreateBuffer(
        ctx,
        SBGL_BUFFER_USAGE_INDEX,
        sizeof(uint32_t) * PRO_INDEX_COUNT,
        pro_indices
    );
    free(pro_indices);

    /* Empty vertex layout for procedural rendering. */
    sbgl_PipelineConfig pipe_cfg = { .vertexShader = vert,
                                     .fragmentShader = frag,
                                     .vertexLayout = { sizeof(sbgl_Vertex), 0, NULL } };
    sbgl_Pipeline pipeline = sbgl_CreatePipeline(ctx, &pipe_cfg);
    sbgl_RenderQueue* queue = sbgl_CreateRenderQueue(ctx, &arena);

    /* Render Distance Configuration. */
    int radius = 5;
    int instance_count = calculate_instance_count(radius);
    float initial_far = calculate_far_plane(radius);

    sbgl_Camera camera =
        sbgl_CameraPerspective(SBGL_PI / 4.0f, 1280.0f / 720.0f, 0.1f, initial_far);
    camera.position = sbgl_Vec3Set(0.0f, 40.0f, 100.0f);

    float pitch = -0.4f, yaw = -SBGL_PI / 2.0f;
    bool mouse_locked = false;
    float move_speed = 100.0f, mouse_sensitivity = 0.005f;
    float start_time = (float)clock() / CLOCKS_PER_SEC, last_time = start_time;
    float fps_timer = 0.0f;
    int frame_count = 0;

    printf("--- Voxel Controls ---\n");
    printf("W/A/S/D: Move\n");
    printf("Q/E: Vertical Move\n");
    printf("TAB: Lock/Unlock Mouse\n");
    printf("+/-: Change Render Distance\n");
    printf("ESC: Exit\n");
    printf("----------------------\n");

    while (!sbgl_WindowShouldClose(ctx)) {
        sbgl_Clear(ctx, 0.5f, 0.7f, 1.0f, 1.0f);
        sbgl_BeginDrawing(ctx);

        const sbgl_InputState* input = sbgl_GetInputState(ctx);
        if (input->keysDown[SBGL_KEY_ESCAPE])
            break;

        float current_time = (float)clock() / CLOCKS_PER_SEC;
        float dt = current_time - last_time;
        last_time = current_time;

        frame_count++;
        fps_timer += dt;
        if (fps_timer >= 1.0f) {
            printf(
                "FPS: %d | Frame Time: %.2f ms\n",
                frame_count,
                (fps_timer / (float)frame_count) * 1000.0f
            );
            fps_timer = 0.0f;
            frame_count = 0;
        }

        if (input->keysPressed[SBGL_KEY_TAB]) {
            mouse_locked = !mouse_locked;
            sbgl_SetMouseMode(
                ctx,
                mouse_locked ? SBGL_MOUSE_MODE_CAPTURED : SBGL_MOUSE_MODE_NORMAL
            );
        }

        // Radius control using +/- keys
        if (input->keysPressed[SBGL_KEY_EQUAL]) {
            apply_radius_change(&camera, &radius, &instance_count, 1);
        }
        if (input->keysPressed[SBGL_KEY_MINUS]) {
            apply_radius_change(&camera, &radius, &instance_count, -1);
        }

        if (mouse_locked) {
            yaw += (float)input->mouseDeltaX * mouse_sensitivity;
            pitch -= (float)input->mouseDeltaY * mouse_sensitivity;
            if (pitch > 1.5f)
                pitch = 1.5f;
            if (pitch < -1.5f)
                pitch = -1.5f;
        }

        sbgl_Vec3 front = sbgl_Vec3Normalize(
            sbgl_Vec3Set(cosf(yaw) * cosf(pitch), sinf(pitch), sinf(yaw) * cosf(pitch))
        );
        sbgl_Vec3 right = sbgl_Vec3Normalize(sbgl_Vec3Cross(front, sbgl_Vec3Set(0, 1, 0)));
        float velocity = move_speed * dt;

        if (input->keysDown[SBGL_KEY_W])
            camera.position = sbgl_Vec3Add(camera.position, sbgl_Vec3Mul(front, velocity));
        if (input->keysDown[SBGL_KEY_S])
            camera.position = sbgl_Vec3Sub(camera.position, sbgl_Vec3Mul(front, velocity));
        if (input->keysDown[SBGL_KEY_A])
            camera.position = sbgl_Vec3Sub(camera.position, sbgl_Vec3Mul(right, velocity));
        if (input->keysDown[SBGL_KEY_D])
            camera.position = sbgl_Vec3Add(camera.position, sbgl_Vec3Mul(right, velocity));
        if (input->keysDown[SBGL_KEY_Q])
            camera.position =
                sbgl_Vec3Sub(camera.position, sbgl_Vec3Mul(sbgl_Vec3Set(0, 1, 0), velocity));
        if (input->keysDown[SBGL_KEY_E])
            camera.position =
                sbgl_Vec3Add(camera.position, sbgl_Vec3Mul(sbgl_Vec3Set(0, 1, 0), velocity));

        camera.target = sbgl_Vec3Add(camera.position, front);
        camera.up = sbgl_Vec3Normalize(sbgl_Vec3Cross(right, front));

        /* Submit chunk instances.
           Every instance contains camera chunk metadata and radius in its transform matrix. */
        int camChunkX = (int)floorf(camera.position.x / VOXEL_CHUNK_SIZE);
        int camChunkZ = (int)floorf(camera.position.z / VOXEL_CHUNK_SIZE);

        for (int i = 0; i < instance_count; i++) {
            sbgl_InstanceData data = { 0 };
            // Pass camera chunk metadata and radius in every instance.
            // This ensures the shader can retrieve it using gl_BaseInstanceARB.
            data.transform.m[0][0] = (float)camChunkX;
            data.transform.m[0][1] = (float)camChunkZ;
            data.transform.m[0][2] = (float)radius;
            
            sbgl_SubmitDraw(queue, 3, 0, 0, &data);
        }

        VoxelPushConstants pc = {
            .viewProj =
                sbgl_Mat4Mul(sbgl_CameraGetProjection(&camera), sbgl_CameraGetView(&camera)),
            .heightAddress = sbgl_GetBufferDeviceAddress(ctx, height_ssbo)
        };

        sbgl_BindPipeline(ctx, pipeline);
        sbgl_BindBuffer(ctx, vbo, SBGL_BUFFER_USAGE_VERTEX);
        sbgl_BindBuffer(ctx, pro_ibo, SBGL_BUFFER_USAGE_INDEX);
        sbgl_RenderQueuesEx(ctx, &queue, 1, &pc.viewProj, pc.heightAddress);

        sbgl_EndDrawing(ctx);
    }

    sbgl_DeviceWaitIdle(ctx);
    sbgl_DestroyBuffer(ctx, height_ssbo);
    sbgl_Shutdown(ctx);
    return 0;
}
