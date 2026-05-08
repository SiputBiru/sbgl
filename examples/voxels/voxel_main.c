#include <sbgl.h>
#include <sbgl_camera.h>
#include <sbgl_math.h>
#include <core/sbl_arena.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct {
    sbgl_Mat4 viewProj;
    uint64_t instanceAddress;
} VoxelPushConstants;

int main(void) {
    /* Initialize the graphics context and window. */
    sbgl_InitResult res = sbgl_Init(1280, 720, "SBgl Procedural Voxels");
    if (res.error != SBGL_SUCCESS) return 1;
    sbgl_Context* ctx = res.ctx;

    /* Initialize the memory arena. */
    SblArena arena;
    sbl_arena_init(&arena, 32 * 1024 * 1024);

    /* Load the procedural voxel shaders. */
    sbgl_Shader vert = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_VERTEX, "shaders/voxel.vert.spv");
    sbgl_Shader frag = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_FRAGMENT, "shaders/voxel.frag.spv");

    /* Create a dummy VBO. Pure procedural mode doesn't use it, but we bind it for safety. */
    sbgl_Vertex dummy_vert = {sbgl_Vec3Set(0,0,0), sbgl_Vec3Set(0,0,0)};
    sbgl_Buffer vbo = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_VERTEX, sizeof(dummy_vert), &dummy_vert);

    /* Create a linear index buffer (0, 1, 2...) for procedural generation. */
    const uint32_t PRO_INDEX_COUNT = 36864; // 32 * 32 * 36
    uint32_t* pro_indices = malloc(sizeof(uint32_t) * PRO_INDEX_COUNT);
    for (uint32_t i = 0; i < PRO_INDEX_COUNT; i++) pro_indices[i] = i;
    sbgl_Buffer pro_ibo = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_INDEX, sizeof(uint32_t) * PRO_INDEX_COUNT, pro_indices);
    free(pro_indices);

    /* Empty vertex layout for procedural rendering. */
    sbgl_PipelineConfig pipe_cfg = {
        .vertexShader = vert, .fragmentShader = frag,
        .vertexLayout = {sizeof(sbgl_Vertex), 0, NULL}
    };
    sbgl_Pipeline pipeline = sbgl_CreatePipeline(ctx, &pipe_cfg);
    sbgl_RenderQueue* queue = sbgl_CreateRenderQueue(ctx, &arena);

    sbgl_Camera camera = sbgl_CameraPerspective(SBGL_PI/4.0f, 1280.0f/720.0f, 0.1f, 2000.0f);
    camera.position = sbgl_Vec3Set(0.0f, 40.0f, 100.0f);

    float pitch = -0.4f, yaw = -SBGL_PI / 2.0f;
    bool mouse_locked = false;
    float move_speed = 100.0f, mouse_sensitivity = 0.005f;
    float start_time = (float)clock() / CLOCKS_PER_SEC, last_time = start_time;
    float fps_timer = 0.0f;
    int frame_count = 0;

    while (!sbgl_WindowShouldClose(ctx)) {
        sbgl_Clear(ctx, 0.5f, 0.7f, 1.0f, 1.0f);
        sbgl_BeginDrawing(ctx);

        const sbgl_InputState* input = sbgl_GetInputState(ctx);
        if (input->keysDown[SBGL_KEY_ESCAPE]) break;

        float current_time = (float)clock() / CLOCKS_PER_SEC;
        float dt = current_time - last_time;
        last_time = current_time;

        frame_count++; fps_timer += dt;
        if (fps_timer >= 1.0f) {
            printf("FPS: %d | Frame Time: %.2f ms\n", frame_count, (fps_timer / (float)frame_count) * 1000.0f);
            fps_timer = 0.0f; frame_count = 0;
        }

        if (input->keysPressed[SBGL_KEY_TAB]) {
            mouse_locked = !mouse_locked;
            sbgl_SetMouseMode(ctx, mouse_locked ? SBGL_MOUSE_MODE_CAPTURED : SBGL_MOUSE_MODE_NORMAL);
        }

        if (mouse_locked) {
            yaw += (float)input->mouseDeltaX * mouse_sensitivity;
            pitch -= (float)input->mouseDeltaY * mouse_sensitivity;
            if (pitch > 1.5f) pitch = 1.5f; 
            if (pitch < -1.5f) pitch = -1.5f;
        }

        sbgl_Vec3 front = sbgl_Vec3Normalize(sbgl_Vec3Set(cosf(yaw) * cosf(pitch), sinf(pitch), sinf(yaw) * cosf(pitch)));
        sbgl_Vec3 right = sbgl_Vec3Normalize(sbgl_Vec3Cross(front, sbgl_Vec3Set(0, 1, 0)));
        float velocity = move_speed * dt;

        if (input->keysDown[SBGL_KEY_W]) camera.position = sbgl_Vec3Add(camera.position, sbgl_Vec3Mul(front, velocity));
        if (input->keysDown[SBGL_KEY_S]) camera.position = sbgl_Vec3Sub(camera.position, sbgl_Vec3Mul(front, velocity));
        if (input->keysDown[SBGL_KEY_A]) camera.position = sbgl_Vec3Sub(camera.position, sbgl_Vec3Mul(right, velocity));
        if (input->keysDown[SBGL_KEY_D]) camera.position = sbgl_Vec3Add(camera.position, sbgl_Vec3Mul(right, velocity));
        if (input->keysDown[SBGL_KEY_Q]) camera.position = sbgl_Vec3Sub(camera.position, sbgl_Vec3Mul(sbgl_Vec3Set(0, 1, 0), velocity));
        if (input->keysDown[SBGL_KEY_E]) camera.position = sbgl_Vec3Add(camera.position, sbgl_Vec3Mul(sbgl_Vec3Set(0, 1, 0), velocity));

        camera.target = sbgl_Vec3Add(camera.position, front);
        camera.up = sbgl_Vec3Normalize(sbgl_Vec3Cross(right, front));

        /* Submit 121 chunk instances.
           The first instance contains camera chunk offsets in its transform matrix. */
        int camChunkX = (int)floorf(camera.position.x / 32.0f);
        int camChunkZ = (int)floorf(camera.position.z / 32.0f);

        for (int i = 0; i < 121; i++) {
            sbgl_InstanceData data = {0};
            if (i == 0) {
                // Pass metadata in the first instance
                data.transform.m[0][0] = (float)camChunkX;
                data.transform.m[0][1] = (float)camChunkZ;
            }
            sbgl_SubmitDraw(queue, 3, 0, 0, &data);
        }

        VoxelPushConstants pc = {
            .viewProj = sbgl_Mat4Mul(sbgl_CameraGetProjection(&camera), sbgl_CameraGetView(&camera)),
        };

        sbgl_BindPipeline(ctx, pipeline);
        sbgl_BindBuffer(ctx, vbo, SBGL_BUFFER_USAGE_VERTEX);
        sbgl_BindBuffer(ctx, pro_ibo, SBGL_BUFFER_USAGE_INDEX);
        sbgl_RenderQueues(ctx, &queue, 1, &pc.viewProj);

        sbgl_EndDrawing(ctx);
    }

    sbgl_DeviceWaitIdle(ctx);
    sbgl_Shutdown(ctx);
    return 0;
}
