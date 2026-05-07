#include "core/sbl_arena.h"
#include <math.h>
#include <sbgl.h>
#include <sbgl_camera.h>
#include <sbgl_math.h>
#include <stdio.h>
#include <time.h>

int main(void) {
	sbgl_InitResult res = sbgl_Init(800, 600, "SBgl Advanced 3D Batching");
	if (res.error != SBGL_SUCCESS)
		return 1;
	sbgl_Context* ctx = res.ctx;

	SblArena arena;
	sbl_arena_init(&arena, 10 * 1024 * 1024); // 10MB for render queues

	sbgl_Shader vert =
		sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_VERTEX, "shaders/batching.vert.spv");
	sbgl_Shader frag =
		sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_FRAGMENT, "shaders/batching.frag.spv");

	sbgl_Vertex vertices[] = { // Triangle (Mesh 0)
							   { sbgl_Vec3Set(0.0f, -0.5f, 0.0f), sbgl_Vec3Set(1, 0, 0) },
							   { sbgl_Vec3Set(0.5f, 0.5f, 0.0f), sbgl_Vec3Set(0, 1, 0) },
							   { sbgl_Vec3Set(-0.5f, 0.5f, 0.0f), sbgl_Vec3Set(0, 0, 1) },

							   // Cube (Mesh 1)
							   { sbgl_Vec3Set(-0.5f, -0.5f, -0.5f), sbgl_Vec3Set(1, 1, 1) },
							   { sbgl_Vec3Set(0.5f, -0.5f, -0.5f), sbgl_Vec3Set(1, 1, 1) },
							   { sbgl_Vec3Set(0.5f, 0.5f, -0.5f), sbgl_Vec3Set(1, 1, 1) },
							   { sbgl_Vec3Set(-0.5f, 0.5f, -0.5f), sbgl_Vec3Set(1, 1, 1) },
							   { sbgl_Vec3Set(-0.5f, -0.5f, 0.5f), sbgl_Vec3Set(1, 1, 1) },
							   { sbgl_Vec3Set(0.5f, -0.5f, 0.5f), sbgl_Vec3Set(1, 1, 1) },
							   { sbgl_Vec3Set(0.5f, 0.5f, 0.5f), sbgl_Vec3Set(1, 1, 1) },
							   { sbgl_Vec3Set(-0.5f, 0.5f, 0.5f), sbgl_Vec3Set(1, 1, 1) },

							   // Pyramid (Mesh 2)
							   { sbgl_Vec3Set(0.0f, 0.5f, 0.0f), sbgl_Vec3Set(1, 1, 0) },	 // Top
							   { sbgl_Vec3Set(-0.5f, -0.5f, -0.5f), sbgl_Vec3Set(1, 0, 1) }, // base
							   { sbgl_Vec3Set(0.5f, -0.5f, -0.5f), sbgl_Vec3Set(1, 0, 1) },
							   { sbgl_Vec3Set(0.5f, -0.5f, 0.5f), sbgl_Vec3Set(1, 0, 1) },
							   { sbgl_Vec3Set(-0.5f, -0.5f, 0.5f), sbgl_Vec3Set(1, 0, 1) }
	};

	uint32_t indices[] = { // Triangle
						   0,
						   2,
						   1,

						   // Cube
						   0,
						   3,
						   2,
						   2,
						   1,
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
						   2,
						   6,
						   6,
						   5,
						   1,
						   2,
						   3,
						   7,
						   7,
						   6,
						   2,
						   0,
						   1,
						   5,
						   5,
						   4,
						   0,

						   // Pyramid
						   0,
						   2,
						   1,
						   0,
						   3,
						   2,
						   0,
						   4,
						   3,
						   0,
						   1,
						   4,
						   1,
						   2,
						   3,
						   3,
						   4,
						   1
	};

	sbgl_Buffer vbo = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_VERTEX, sizeof(vertices), vertices);
	sbgl_Buffer ibo = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_INDEX, sizeof(indices), indices);

	sbgl_VertexAttribute attributes[] = {
		{ 0, offsetof(sbgl_Vertex, position), SBGL_FORMAT_R32G32B32_SFLOAT },
		{ 1, offsetof(sbgl_Vertex, color), SBGL_FORMAT_R32G32B32_SFLOAT }
	};

	sbgl_PipelineConfig config = { .vertexShader = vert,
								   .fragmentShader = frag,
								   .vertexLayout = { sizeof(sbgl_Vertex), 2, attributes } };

	sbgl_Pipeline pipeline = sbgl_CreatePipeline(ctx, &config);
	sbgl_RenderQueue* queue = sbgl_CreateRenderQueue(ctx, &arena);

	int width, height;
	sbgl_GetWindowSize(ctx, &width, &height);
	sbgl_Camera camera =
		sbgl_CameraPerspective(SBGL_PI / 4.0f, (float)width / (float)height, 0.1f, 1000.0f);
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
	float fps_timer = 0.0f;
	int frame_count = 0;

	while (!sbgl_WindowShouldClose(ctx)) {
		sbgl_Clear(ctx, 0.02f, 0.02f, 0.02f, 1.0f);
		sbgl_BeginDrawing(ctx);

		const sbgl_InputState* input = sbgl_GetInputState(ctx);
		if (input->keysDown[SBGL_KEY_ESCAPE]) {
			sbgl_EndDrawing(ctx);
			break;
		}

		float current_time = (float)clock() / CLOCKS_PER_SEC;
		float dt = current_time - last_time;
		last_time = current_time;
		float time = current_time - start_time;

		// Calculate and display FPS
		frame_count++;
		fps_timer += dt;
		if (fps_timer >= 1.0f) {
			printf("FPS: %d | Frame Time: %.2f ms\n", frame_count, (fps_timer / (float)frame_count) * 1000.0f);
			fps_timer = 0.0f;
			frame_count = 0;
		}

		if (input->keysPressed[SBGL_KEY_TAB]) {
			mouse_locked = !mouse_locked;
			sbgl_SetMouseMode(ctx, mouse_locked ? SBGL_MOUSE_MODE_CAPTURED : SBGL_MOUSE_MODE_NORMAL);
		}

		if (mouse_locked) {
			yaw += (float)input->mouseDeltaX * mouse_sensitivity;
			pitch += (float)input->mouseDeltaY * mouse_sensitivity;

			if (pitch > 1.5f)
				pitch = 1.5f;
			if (pitch < -1.5f)
				pitch = -1.5f;
		}

		sbgl_Vec3 front;
		front.x = cosf(yaw) * cosf(pitch);
		front.y = sinf(pitch);
		front.z = sinf(yaw) * cosf(pitch);
		front = sbgl_Vec3Normalize(front);

		sbgl_Vec3 right = sbgl_Vec3Normalize(sbgl_Vec3Cross(front, sbgl_Vec3Set(0.0f, 1.0f, 0.0f)));
		sbgl_Vec3 up = sbgl_Vec3Normalize(sbgl_Vec3Cross(right, front));

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
			camera.position = sbgl_Vec3Add(
				camera.position,
				sbgl_Vec3Mul(sbgl_Vec3Set(0.0f, 1.0f, 0.0f), velocity)
			);
		if (input->keysDown[SBGL_KEY_E])
			camera.position = sbgl_Vec3Sub(
				camera.position,
				sbgl_Vec3Mul(sbgl_Vec3Set(0.0f, 1.0f, 0.0f), velocity)
			);

		camera.target = sbgl_Vec3Add(camera.position, front);
		camera.up = up;

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

		sbgl_BindPipeline(ctx, pipeline);
		sbgl_BindBuffer(ctx, vbo, SBGL_BUFFER_USAGE_VERTEX);
		sbgl_BindBuffer(ctx, ibo, SBGL_BUFFER_USAGE_INDEX);

		sbgl_RenderQueues(ctx, &queue, 1, &vp);

		sbgl_EndDrawing(ctx);
	}

	sbgl_DeviceWaitIdle(ctx);

	sbgl_DestroyPipeline(ctx, pipeline);
	sbgl_DestroyShader(ctx, vert);
	sbgl_DestroyShader(ctx, frag);
	sbgl_DestroyBuffer(ctx, vbo);
	sbgl_DestroyBuffer(ctx, ibo);

	sbl_arena_free(&arena);
	sbgl_Shutdown(ctx);

	return 0;
}
