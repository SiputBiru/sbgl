#include <sbgl.h>
#include "../example_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    float pos[3];
    float color[3];
} Vertex;

typedef struct {
    float mousePos[2];
    float time;
    float padding; // Alignment
} PushData;

int main() {
    sbgl_InitResult res = sbgl_Init(800, 600, "SBgl Interactive Rainbow Triangle");
    if (res.error != SBGL_SUCCESS) return 1;
    sbgl_Context* ctx = res.ctx;

    size_t vert_size, frag_size;
    uint32_t* vert_code = read_file("shaders/triangle.vert.spv", &vert_size);
    uint32_t* frag_code = read_file("shaders/interactive.frag.spv", &frag_size);

    sbgl_Shader vert_shader = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_VERTEX, vert_code, vert_size);
    sbgl_Shader frag_shader = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_FRAGMENT, frag_code, frag_size);

    free(vert_code);
    free(frag_code);

    Vertex vertices[] = {
        {{ 0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}
    };

    sbgl_Buffer vbo = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_VERTEX, sizeof(vertices), vertices);

    sbgl_VertexAttribute attributes[] = {
        {0, offsetof(Vertex, pos)},
        {1, offsetof(Vertex, color)}
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

    while (!sbgl_WindowShouldClose(ctx)) {
        const sbgl_InputState* input = sbgl_GetInputState(ctx);
        if (input->keysDown[SBGL_KEY_ESCAPE]) break;

        sbgl_Clear(ctx, 0.05f, 0.05f, 0.05f, 1.0f);
        sbgl_BeginDrawing(ctx);
        
        float current_time = ((float)clock() / CLOCKS_PER_SEC) - start_time;
        
        PushData push = {
            .mousePos = { (float)input->mouseX / 800.0f, (float)input->mouseY / 600.0f },
            .time = current_time
        };

        sbgl_BindPipeline(ctx, pipeline);
        sbgl_PushConstants(ctx, sizeof(push), &push);
        sbgl_BindBuffer(ctx, vbo, SBGL_BUFFER_USAGE_VERTEX);
        sbgl_Draw(ctx, 3, 0);

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
