#include <sbgl.h>
#include <stdio.h>
#include <stdlib.h>

// Include shaders generated via: xxd -i shader.spv > shader.h
#include "shaders/triangle_vert.h"
#include "shaders/triangle_frag.h"

typedef struct {
    float pos[3];
    float color[3];
} Vertex;

int main() {
    printf("Initializing SBgl Hardcoded Shader Example...\n");

    sbgl_InitResult res = sbgl_Init(800, 600, "SBgl Hardcoded Triangle");
    if (res.error != SBGL_SUCCESS) return 1;
    sbgl_Context* ctx = res.ctx;

    // Loading shaders from the embedded byte arrays
    sbgl_Shader vert_shader = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_VERTEX, 
                                            (uint32_t*)triangle_vert_spv, 
                                            triangle_vert_spv_len);
    
    sbgl_Shader frag_shader = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_FRAGMENT, 
                                            (uint32_t*)triangle_frag_spv, 
                                            triangle_frag_spv_len);

    Vertex vertices[] = {
        {{ 0.0f, -0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 1.0f}},
        {{-0.5f,  0.5f, 0.0f}, {1.0f, 0.0f, 1.0f}}
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

    while (!sbgl_WindowShouldClose(ctx)) {
        sbgl_Clear(ctx, 0.0f, 0.0f, 0.0f, 1.0f);
        sbgl_BeginDrawing(ctx);
        
        sbgl_BindPipeline(ctx, pipeline);
        sbgl_BindBuffer(ctx, vbo, SBGL_BUFFER_USAGE_VERTEX);
        sbgl_Draw(ctx, 3, 0);

        sbgl_EndDrawing(ctx);

        if (sbgl_GetInputState(ctx)->keysDown[SBGL_KEY_ESCAPE]) break;
    }

    sbgl_DeviceWaitIdle(ctx);

    sbgl_DestroyPipeline(ctx, pipeline);
    sbgl_DestroyShader(ctx, vert_shader);
    sbgl_DestroyShader(ctx, frag_shader);
    sbgl_DestroyBuffer(ctx, vbo);
    sbgl_Shutdown(ctx);

    return 0;
}
