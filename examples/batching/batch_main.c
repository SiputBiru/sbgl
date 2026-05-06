#include "../example_util.h"
#include <sbgl.h>
#include <sbgl_camera.h>
#include <sbgl_math.h>
// #include <stdio.h>
// #include <stdlib.h>
#include "core/sbl_arena.h"
#include <time.h>

typedef struct {
	float pos[3];
	float color[3];
} Vertex;

int main(void) {
	ExampleApp app;
	if (!example_app_init(&app, 800, 600, "SBgl Advanced 3D Batching"))
		return 1;

	SblArena arena;
	sbl_arena_init(&arena, 10 * 1024 * 1024); // 10MB for render queues

	sbgl_Shader vert =
		example_load_shader(app.ctx, SBGL_SHADER_STAGE_VERTEX, "shaders/batching.vert.spv");
	sbgl_Shader frag =
		example_load_shader(app.ctx, SBGL_SHADER_STAGE_FRAGMENT, "shaders/batching.frag.spv");

	Vertex vertices[] = { // Triangle (Mesh 0)
						  { { 0.0f, -0.5f, 0.0f }, { 1, 0, 0 } },
						  { { 0.5f, 0.5f, 0.0f }, { 0, 1, 0 } },
						  { { -0.5f, 0.5f, 0.0f }, { 0, 0, 1 } },

						  // Cube (Mesh 1)
						  { { -0.5f, -0.5f, -0.5f }, { 1, 1, 1 } },
						  { { 0.5f, -0.5f, -0.5f }, { 1, 1, 1 } },
						  { { 0.5f, 0.5f, -0.5f }, { 1, 1, 1 } },
						  { { -0.5f, 0.5f, -0.5f }, { 1, 1, 1 } },
						  { { -0.5f, -0.5f, 0.5f }, { 1, 1, 1 } },
						  { { 0.5f, -0.5f, 0.5f }, { 1, 1, 1 } },
						  { { 0.5f, 0.5f, 0.5f }, { 1, 1, 1 } },
						  { { -0.5f, 0.5f, 0.5f }, { 1, 1, 1 } },

						  // Pyramid (Mesh 2)
						  { { 0.0f, 0.5f, 0.0f }, { 1, 1, 0 } },	// Top
						  { { -0.5f, -0.5f, -0.5f }, { 1, 0, 1 } }, // base
						  { { 0.5f, -0.5f, -0.5f }, { 1, 0, 1 } },
						  { { 0.5f, -0.5f, 0.5f }, { 1, 0, 1 } },
						  { { -0.5f, -0.5f, 0.5f }, { 1, 0, 1 } }
	};

	uint32_t indices[] = { // Triangle
						   0,
						   1,
						   2,

						   // Cube
						   0,
						   1,
						   2,
						   2,
						   3,
						   0,
						   4,
						   5,
						   6,
						   6,
						   7,
						   4,
						   0,
						   4,
						   7,
						   7,
						   3,
						   0,
						   1,
						   5,
						   6,
						   6,
						   2,
						   1,
						   0,
						   1,
						   5,
						   5,
						   4,
						   0,
						   3,
						   2,
						   6,
						   6,
						   7,
						   3,

						   // Pyramid
						   0,
						   1,
						   2,
						   0,
						   2,
						   3,
						   0,
						   3,
						   4,
						   0,
						   4,
						   1,
						   1,
						   2,
						   3,
						   3,
						   4,
						   1
	};

	sbgl_Buffer vbo =
		sbgl_CreateBuffer(app.ctx, SBGL_BUFFER_USAGE_VERTEX, sizeof(vertices), vertices);
	sbgl_Buffer ibo = sbgl_CreateBuffer(app.ctx, SBGL_BUFFER_USAGE_INDEX, sizeof(indices), indices);

	sbgl_VertexAttribute attributes[] = {
		{ 0, offsetof(Vertex, pos), SBGL_FORMAT_R32G32B32_SFLOAT },
		{ 1, offsetof(Vertex, color), SBGL_FORMAT_R32G32B32_SFLOAT }
	};

	sbgl_PipelineConfig config = { .vertexShader = vert,
								   .fragmentShader = frag,
								   .vertexLayout = { sizeof(Vertex), 2, attributes } };

	sbgl_Pipeline pipeline = sbgl_CreatePipeline(app.ctx, &config);
	sbgl_RenderQueue* queue = sbgl_CreateRenderQueue(app.ctx, &arena);

	sbgl_Camera camera =
		sbgl_CameraPerspective(SBGL_PI / 4.0f, (float)app.width / (float)app.height, 0.1f, 1000.0f);
	camera.position = sbgl_Vec3Set(0.0f, 50.0f, 200.0f);
	camera.target = sbgl_Vec3Set(0.0f, 0.0f, 0.0f);
	camera.up = sbgl_Vec3Set(0.0f, 1.0f, 0.0f);

	float pitch = -0.2f;
	float yaw = -SBGL_PI / 2.0f;
	bool mouse_locked = false;
	float move_speed = 100.0f;
	float mouse_sensitivity = 0.005f;

	float start_time = (float)clock() / CLOCKS_PER_SEC;
	float last_time = start_time;

	while (!sbgl_WindowShouldClose(app.ctx)) {
		if (sbgl_GetInputState(app.ctx)->keysDown[SBGL_KEY_ESCAPE])
			break;

		float current_time = (float)clock() / CLOCKS_PER_SEC;
		float dt = current_time - last_time;
		last_time = current_time;
		float time = current_time - start_time;

		const sbgl_InputState* input = sbgl_GetInputState(app.ctx);

		if (input->keysPressed[SBGL_KEY_TAB]) {
			mouse_locked = !mouse_locked;
		}

		if (mouse_locked) {
			yaw += (float)input->mouseDeltaX * mouse_sensitivity;
			pitch -= (float)input->mouseDeltaY * mouse_sensitivity;

			if (pitch > 1.5f) pitch = 1.5f;
			if (pitch < -1.5f) pitch = -1.5f;
		}

		sbgl_Vec3 front;
		front.x = cosf(yaw) * cosf(pitch);
		front.y = sinf(pitch);
		front.z = sinf(yaw) * cosf(pitch);
		front = sbgl_Vec3Normalize(front);

		sbgl_Vec3 right = sbgl_Vec3Normalize(sbgl_Vec3Cross(front, sbgl_Vec3Set(0.0f, 1.0f, 0.0f)));
		sbgl_Vec3 up = sbgl_Vec3Normalize(sbgl_Vec3Cross(right, front));

		float velocity = move_speed * dt;

		if (input->keysDown[SBGL_KEY_W]) camera.position = sbgl_Vec3Add(camera.position, sbgl_Vec3Mul(front, velocity));
		if (input->keysDown[SBGL_KEY_S]) camera.position = sbgl_Vec3Sub(camera.position, sbgl_Vec3Mul(front, velocity));
		if (input->keysDown[SBGL_KEY_A]) camera.position = sbgl_Vec3Sub(camera.position, sbgl_Vec3Mul(right, velocity));
		if (input->keysDown[SBGL_KEY_D]) camera.position = sbgl_Vec3Add(camera.position, sbgl_Vec3Mul(right, velocity));

		camera.target = sbgl_Vec3Add(camera.position, front);
		camera.up = up;

		sbgl_Clear(app.ctx, 0.02f, 0.02f, 0.02f, 1.0f);
		sbgl_BeginDrawing(app.ctx);

		sbgl_Mat4 vp = sbgl_Mat4Mul(sbgl_CameraGetProjection(&camera), sbgl_CameraGetView(&camera));

		// Submit 10,000 instances
		for (int i = 0; i < 10000; i++) {
			uint32_t meshId = i % 3;

			float angle = time + (float)i;
			sbgl_Vec3 pos = sbgl_Vec3Set(
				((float)(i % 100) - 50.0f) * 10.0f,
				((float)(i / 100 % 100) - 50.0f) * 10.0f,
				((float)(i / 10000) - 0.5f) * 10.0f
			);

			sbgl_InstanceData data;
			data.transform = sbgl_Mat4Mul(
				sbgl_Mat4Translate(pos),
				sbgl_Mat4Rotate(angle, sbgl_Vec3Set(0.4f, 1.0f, 0.2f))
			);

			float r = (float)(i % 255) / 255.0f;
			float g = (float)((i * 7) % 255) / 255.0f;
			float b = (float)((i * 13) % 255) / 255.0f;
			data.color = sbgl_Vec4Set(r, g, b, 1.0f);

			// Use meshId as the sort key for maximum batching
			sbgl_SubmitDraw(queue, meshId, 0, (sbgl_SortKey)meshId, &data);
		}

		sbgl_BindPipeline(app.ctx, pipeline);
		sbgl_BindBuffer(app.ctx, vbo, SBGL_BUFFER_USAGE_VERTEX);
		sbgl_BindBuffer(app.ctx, ibo, SBGL_BUFFER_USAGE_INDEX);

		sbgl_RenderQueues(app.ctx, &queue, 1, &vp);

		sbgl_EndDrawing(app.ctx);
	}

	sbgl_DeviceWaitIdle(app.ctx);

	sbgl_DestroyPipeline(app.ctx, pipeline);
	sbgl_DestroyShader(app.ctx, vert);
	sbgl_DestroyShader(app.ctx, frag);
	sbgl_DestroyBuffer(app.ctx, vbo);
	sbgl_DestroyBuffer(app.ctx, ibo);

	sbl_arena_free(&arena);
	example_app_shutdown(&app);

	return 0;
}
