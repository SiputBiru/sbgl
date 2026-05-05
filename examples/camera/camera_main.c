#include "../example_util.h"
#include <sbgl.h>
#include <sbgl_camera.h>
#include <sbgl_math.h>
#include <stdio.h>
#include <time.h>

typedef struct {
	float pos[3];
	float color[3];
} Vertex;

typedef struct {
	sbgl_Mat4 viewProj;
} PushData;

int main(void) {
	ExampleApp app;
	if (!example_app_init(&app, 800, 600, "SBgl Unified Camera Example"))
		return 1;

	sbgl_Shader vert =
		example_load_shader(app.ctx, SBGL_SHADER_STAGE_VERTEX, "shaders/pyramid.vert.spv");
	sbgl_Shader frag =
		example_load_shader(app.ctx, SBGL_SHADER_STAGE_FRAGMENT, "shaders/pyramid.frag.spv");

	Vertex vertices[] = {
		{ { 0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ { 0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
		{ { -0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
		{ { 0.0f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
		{ { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
		{ { 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
		{ { 0.0f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
		{ { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
		{ { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
		{ { 0.0f, -0.5f, 0.0f }, { 1.0f, 1.0f, 0.0f } },
		{ { -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f } },
		{ { -0.5f, 0.5f, 0.5f }, { 1.0f, 1.0f, 0.0f } },
		{ { -0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f } },
		{ { 0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f } },
		{ { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 1.0f } },
		{ { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f } },
		{ { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 1.0f } },
	};

	sbgl_Buffer vbo =
		sbgl_CreateBuffer(app.ctx, SBGL_BUFFER_USAGE_VERTEX, sizeof(vertices), vertices);

	sbgl_VertexAttribute attributes[] = {
		{ 0, offsetof(Vertex, pos), SBGL_FORMAT_R32G32B32_SFLOAT },
		{ 1, offsetof(Vertex, color), SBGL_FORMAT_R32G32B32_SFLOAT }
	};

	sbgl_PipelineConfig config = { .vertexShader = vert,
								   .fragmentShader = frag,
								   .vertexLayout = { sizeof(Vertex), 2, attributes } };

	sbgl_Pipeline pipeline = sbgl_CreatePipeline(app.ctx, &config);

	sbgl_Camera camera = sbgl_CameraPerspective(SBGL_PI / 4.0f, 800.0f / 600.0f, 0.1f, 100.0f);
	camera.position = sbgl_Vec3Set(0.0f, -2.0f, -3.0f);
	camera.target = sbgl_Vec3Set(0.0f, 0.0f, 0.0f);
	camera.up = sbgl_Vec3Set(0.0f, -1.0f, 0.0f);

	float start_time = (float)clock() / CLOCKS_PER_SEC;

	while (!sbgl_WindowShouldClose(app.ctx)) {
		if (sbgl_GetInputState(app.ctx)->keysDown[SBGL_KEY_ESCAPE])
			break;

		sbgl_Clear(app.ctx, 0.0f, 0.0f, 0.0f, 0.0f);
		sbgl_BeginDrawing(app.ctx);

		sbgl_Mat4 model = sbgl_Mat4Rotate(
			((float)clock() / CLOCKS_PER_SEC - start_time) * 10.5f,
			sbgl_Vec3Set(0.0f, 1.0f, 0.0f)
		);
		PushData push = { .viewProj = sbgl_Mat4Mul(
							  sbgl_CameraGetProjection(&camera),
							  sbgl_Mat4Mul(sbgl_CameraGetView(&camera), model)
						  ) };

		sbgl_BindPipeline(app.ctx, pipeline);
		sbgl_PushConstants(app.ctx, sizeof(push), &push);
		sbgl_BindBuffer(app.ctx, vbo, SBGL_BUFFER_USAGE_VERTEX);
		sbgl_Draw(app.ctx, 18, 0);

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
