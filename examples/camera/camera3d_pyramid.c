#include <sbgl.h>
#include <sbgl_math.h>
#include <sbgl_camera.h>
#include "../example_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    float pos[3];
    float color[3];
} Vertex;

typedef struct {
    sbgl_Mat4 viewProj;
} PushData;

int main(void) {
    sbgl_InitResult res = sbgl_Init(800, 600, "SBgl 3D Pyramid (Backface Culling)");
    if (res.error != SBGL_SUCCESS) return 1;
    sbgl_Context* ctx = res.ctx;

    size_t vert_size, frag_size;
    uint32_t* vert_code = read_file("shaders/pyramid.vert.spv", &vert_size);
    uint32_t* frag_code = read_file("shaders/pyramid.frag.spv", &frag_size);

    if (!vert_code || !frag_code) {
        if (vert_code) free(vert_code);
        if (frag_code) free(frag_code);
        sbgl_Shutdown(ctx);
        return 1;
    }

    sbgl_Shader vert_shader = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_VERTEX, vert_code, vert_size);
    sbgl_Shader frag_shader = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_FRAGMENT, frag_code, frag_size);

    free(vert_code);
    free(frag_code);

    // Tip: T(0, -0.5, 0)
    // Base: A(-0.5, 0.5, -0.5), B(0.5, 0.5, -0.5), C(0.5, 0.5, 0.5), D(-0.5, 0.5, 0.5)
    // Front face is Clockwise
    Vertex vertices[] = {
        // Front Face (Red)
        {{ 0.0f, -0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}}, // T
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // B
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // A

        // Right Face (Green)
        {{ 0.0f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}}, // T
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}}, // C
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // B

        // Back Face (Blue)
        {{ 0.0f, -0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}}, // T
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // D
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // C

        // Left Face (Yellow)
        {{ 0.0f, -0.5f,  0.0f}, {1.0f, 1.0f, 0.0f}}, // T
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}}, // A
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}}, // D

        // Base 1 (Magenta)
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, // A
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, // B
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}}, // C

        // Base 2 (Cyan)
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}}, // A
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}}, // C
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}}, // D
    };

    sbgl_Buffer vbo = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_VERTEX, sizeof(vertices), vertices);

    sbgl_VertexAttribute attributes[] = {
        {0, offsetof(Vertex, pos), SBGL_FORMAT_R32G32B32_SFLOAT},
        {1, offsetof(Vertex, color), SBGL_FORMAT_R32G32B32_SFLOAT}
    };

    sbgl_PipelineConfig config = {
        .vertexShader = vert_shader,
        .fragmentShader = frag_shader,
        .vertexLayout = {
            .stride = sizeof(Vertex),
            .attributeCount = 2,
            .attributes = attributes
        }
    };

    sbgl_Pipeline pipeline = sbgl_CreatePipeline(ctx, &config);

    float start_time = (float)clock() / CLOCKS_PER_SEC;

    // Stateful Camera Setup
    float aspect = 800.0f / 600.0f;
    sbgl_Camera camera = sbgl_CameraPerspective(SBGL_PI / 4.0f, aspect, 0.1f, 100.0f);
    camera.position = sbgl_Vec3Set(0.0f, -2.0f, -3.0f);
    camera.target = sbgl_Vec3Set(0.0f, 0.0f, 0.0f);
    camera.up = sbgl_Vec3Set(0.0f, -1.0f, 0.0f);

    while (!sbgl_WindowShouldClose(ctx)) {
        const sbgl_InputState* input = sbgl_GetInputState(ctx);
        if (input->keysDown[SBGL_KEY_ESCAPE]) break;

        sbgl_Clear(ctx, 0.1f, 0.1f, 0.1f, 1.0f);
        sbgl_BeginDrawing(ctx);
        
        float current_time = ((float)clock() / CLOCKS_PER_SEC) - start_time;
        
        // Rotate around Y axis
        sbgl_Mat4 model = sbgl_Mat4Rotate(current_time * 1.5f, sbgl_Vec3Set(0.0f, 1.0f, 0.0f));
        
        // Combine Camera matrices
        sbgl_Mat4 view = sbgl_CameraGetView(&camera);
        sbgl_Mat4 proj = sbgl_CameraGetProjection(&camera);
        sbgl_Mat4 viewProjModel = sbgl_Mat4Mul(proj, sbgl_Mat4Mul(view, model));

        PushData push = {
            .viewProj = viewProjModel
        };

        sbgl_BindPipeline(ctx, pipeline);
        sbgl_PushConstants(ctx, sizeof(push), &push);
        sbgl_BindBuffer(ctx, vbo, SBGL_BUFFER_USAGE_VERTEX);
        sbgl_Draw(ctx, 18, 0);

        sbgl_EndDrawing(ctx);
    }

    sbgl_DeviceWaitIdle(ctx);

    sbgl_DestroyPipeline(ctx, pipeline);
    sbgl_DestroyShader(ctx, vert_shader);
    sbgl_DestroyShader(ctx, frag_shader);
    sbgl_DestroyBuffer(ctx, vbo);
    sbgl_Shutdown(ctx);

    return 0;
}
