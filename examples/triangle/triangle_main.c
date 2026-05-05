#include <sbgl.h>
#include "../example_util.h"
#include <stdio.h>
#include <time.h>

typedef struct {
    float pos[3];
    float color[3];
} Vertex;

typedef struct {
    float mousePos[2];
    float time;
    float padding;
} PushData;

int main(void) {
    ExampleApp app;
    if (!example_app_init(&app, 800, 600, "SBgl Unified Triangle Example")) return 1;

    sbgl_Shader vert = example_load_shader(app.ctx, SBGL_SHADER_STAGE_VERTEX, "shaders/triangle.vert.spv");
    sbgl_Shader frag = example_load_shader(app.ctx, SBGL_SHADER_STAGE_FRAGMENT, "shaders/interactive.frag.spv");

    Vertex vertices[] = {
        {{ 0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}
    };

    sbgl_Buffer vbo = sbgl_CreateBuffer(app.ctx, SBGL_BUFFER_USAGE_VERTEX, sizeof(vertices), vertices);

    sbgl_VertexAttribute attributes[] = {
        {0, offsetof(Vertex, pos), SBGL_FORMAT_R32G32B32_SFLOAT},
        {1, offsetof(Vertex, color), SBGL_FORMAT_R32G32B32_SFLOAT}
    };

    sbgl_PipelineConfig config = {
        .vertexShader = vert,
        .fragmentShader = frag,
        .vertexLayout = { sizeof(Vertex), 2, attributes }
    };

    sbgl_Pipeline pipeline = sbgl_CreatePipeline(app.ctx, &config);
    float start_time = (float)clock() / CLOCKS_PER_SEC;

    while (!sbgl_WindowShouldClose(app.ctx)) {
        const sbgl_InputState* input = sbgl_GetInputState(app.ctx);
        if (input->keysDown[SBGL_KEY_ESCAPE]) break;

        sbgl_Clear(app.ctx, 0.05f, 0.05f, 0.05f, 1.0f);
        sbgl_BeginDrawing(app.ctx);
        
        PushData push = {
            .mousePos = { (float)input->mouseX / (float)app.width, (float)input->mouseY / (float)app.height },
            .time = ((float)clock() / CLOCKS_PER_SEC) - start_time
        };

        sbgl_BindPipeline(app.ctx, pipeline);
        sbgl_PushConstants(app.ctx, sizeof(push), &push);
        sbgl_BindBuffer(app.ctx, vbo, SBGL_BUFFER_USAGE_VERTEX);
        sbgl_Draw(app.ctx, 3, 0);

        sbgl_EndDrawing(app.ctx);
    }

    sbgl_DeviceWaitIdle(app.ctx);

    sbgl_DestroyPipeline(app.ctx, pipeline);
    sbgl_DestroyShader(app.ctx, vert);
    sbgl_DestroyShader(app.ctx, frag);
    sbgl_DestroyBuffer(app.ctx, vbo);
    example_app_shutdown(&app);

    return 0;
}
