#include <sbgl.h>
#include <stdio.h>
#include <time.h>

typedef struct {
	float mousePos[2];
	float time;
	float padding;
} PushData;

int main(void) {
	sbgl_InitResult res = sbgl_Init(800, 600, "SBgl Unified Triangle Example");
	if (res.error != SBGL_SUCCESS)
		return 1;
	sbgl_Context* ctx = res.ctx;

	sbgl_Shader vert =
		sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_VERTEX, "shaders/triangle.vert.spv");
	sbgl_Shader frag =
		sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_FRAGMENT, "shaders/interactive.frag.spv");

	sbgl_Vertex vertices[] = {
		{ { 0, 16383, 0, 32767 }, 0xFF0000FF, 0 },      // 0: Top
		{ { -16383, -16383, 0, 32767 }, 0xFFFF0000, 0 }, // 1: Bottom-Left
		{ { 16383, -16383, 0, 32767 }, 0xFF00FF00, 0 }   // 2: Bottom-Right
	};

	sbgl_Buffer vbo = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_VERTEX, sizeof(vertices), vertices);

	sbgl_VertexAttribute attributes[] = {
		{ 0, offsetof(sbgl_Vertex, position), SBGL_FORMAT_R16G16B16A16_SNORM },
		{ 1, offsetof(sbgl_Vertex, color), SBGL_FORMAT_R8G8B8A8_UNORM }
	};

	sbgl_PipelineConfig config = { .vertexShader = vert,
								   .fragmentShader = frag,
								   .vertexLayout = { sizeof(sbgl_Vertex), 2, attributes } };

	sbgl_Pipeline pipeline = sbgl_CreatePipeline(ctx, &config);
    double start_time = sbgl_GetTime(ctx);

	printf("--- Triangle Controls ---\n");
	printf("ESC: Exit\n");
	printf("-------------------------\n");

    while (!sbgl_WindowShouldClose(ctx)) {
        const sbgl_InputState* input = sbgl_GetInputState(ctx);
        if (input->keysDown[SBGL_KEY_ESCAPE]) break;

        sbgl_Clear(ctx, 0.05f, 0.05f, 0.05f, 1.0f);
        sbgl_BeginDrawing(ctx);
        
        int width, height;
        sbgl_GetWindowSize(ctx, &width, &height);

        PushData push = {
            .mousePos = { (float)input->mouseX / (float)width, (float)input->mouseY / (float)height },
            .time = (float)(sbgl_GetTime(ctx) - start_time)
        };

        sbgl_BindPipeline(ctx, pipeline);
        sbgl_PushConstants(ctx, sizeof(push), &push);
        sbgl_BindBuffer(ctx, vbo, SBGL_BUFFER_USAGE_VERTEX);
        sbgl_Draw(ctx, 3, 0, 1);

        sbgl_EndDrawing(ctx);
    }

    sbgl_DeviceWaitIdle(ctx);

    sbgl_DestroyPipeline(ctx, pipeline);
    sbgl_DestroyShader(ctx, vert);
    sbgl_DestroyShader(ctx, frag);
    sbgl_DestroyBuffer(ctx, vbo);
    sbgl_Shutdown(ctx);

    return 0;
}
