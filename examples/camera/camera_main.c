#include <sbgl.h>
#include <sbgl_camera.h>
#include <sbgl_math.h>
#include <stdio.h>
#include <time.h>

typedef struct {
	sbgl_Mat4 viewProj;
} PushData;

int main(void) {
	sbgl_InitResult res = sbgl_Init(800, 600, "SBgl Unified Camera Example");
	if (res.error != SBGL_SUCCESS) return 1;
	sbgl_Context* ctx = res.ctx;

	sbgl_Shader vert =
		sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_VERTEX, "shaders/pyramid.vert.spv");
	sbgl_Shader frag =
		sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_FRAGMENT, "shaders/pyramid.frag.spv");

	sbgl_Vertex vertices[] = {
		{ sbgl_Vec3Set(0.0f, -0.5f, 0.0f), sbgl_Vec3Set(1.0f, 0.0f, 0.0f) },
		{ sbgl_Vec3Set(0.5f, 0.5f, -0.5f), sbgl_Vec3Set(1.0f, 0.0f, 0.0f) },
		{ sbgl_Vec3Set(-0.5f, 0.5f, -0.5f), sbgl_Vec3Set(1.0f, 0.0f, 0.0f) },
		{ sbgl_Vec3Set(0.0f, -0.5f, 0.0f), sbgl_Vec3Set(0.0f, 1.0f, 0.0f) },
		{ sbgl_Vec3Set(0.5f, 0.5f, 0.5f), sbgl_Vec3Set(0.0f, 1.0f, 0.0f) },
		{ sbgl_Vec3Set(0.5f, 0.5f, -0.5f), sbgl_Vec3Set(0.0f, 1.0f, 0.0f) },
		{ sbgl_Vec3Set(0.0f, -0.5f, 0.0f), sbgl_Vec3Set(0.0f, 0.0f, 1.0f) },
		{ sbgl_Vec3Set(-0.5f, 0.5f, 0.5f), sbgl_Vec3Set(0.0f, 0.0f, 1.0f) },
		{ sbgl_Vec3Set(0.5f, 0.5f, 0.5f), sbgl_Vec3Set(0.0f, 0.0f, 1.0f) },
		{ sbgl_Vec3Set(0.0f, -0.5f, 0.0f), sbgl_Vec3Set(1.0f, 1.0f, 0.0f) },
		{ sbgl_Vec3Set(-0.5f, 0.5f, -0.5f), sbgl_Vec3Set(1.0f, 1.0f, 0.0f) },
		{ sbgl_Vec3Set(-0.5f, 0.5f, 0.5f), sbgl_Vec3Set(1.0f, 1.0f, 0.0f) },
		{ sbgl_Vec3Set(-0.5f, 0.5f, -0.5f), sbgl_Vec3Set(1.0f, 0.0f, 1.0f) },
		{ sbgl_Vec3Set(0.5f, 0.5f, -0.5f), sbgl_Vec3Set(1.0f, 0.0f, 1.0f) },
		{ sbgl_Vec3Set(0.5f, 0.5f, 0.5f), sbgl_Vec3Set(1.0f, 0.0f, 1.0f) },
		{ sbgl_Vec3Set(-0.5f, 0.5f, -0.5f), sbgl_Vec3Set(0.0f, 1.0f, 1.0f) },
		{ sbgl_Vec3Set(0.5f, 0.5f, 0.5f), sbgl_Vec3Set(0.0f, 1.0f, 1.0f) },
		{ sbgl_Vec3Set(-0.5f, 0.5f, 0.5f), sbgl_Vec3Set(0.0f, 1.0f, 1.0f) },
	};

	sbgl_Buffer vbo =
		sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_VERTEX, sizeof(vertices), vertices);

	sbgl_VertexAttribute attributes[] = {
		{ 0, offsetof(sbgl_Vertex, position), SBGL_FORMAT_R32G32B32_SFLOAT },
		{ 1, offsetof(sbgl_Vertex, color), SBGL_FORMAT_R32G32B32_SFLOAT }
	};

	sbgl_PipelineConfig config = { .vertexShader = vert,
								   .fragmentShader = frag,
								   .vertexLayout = { sizeof(sbgl_Vertex), 2, attributes } };

	sbgl_Pipeline pipeline = sbgl_CreatePipeline(ctx, &config);

	int width, height;
	sbgl_GetWindowSize(ctx, &width, &height);
	sbgl_Camera camera = sbgl_CameraPerspective(SBGL_PI / 4.0f, (float)width / (float)height, 0.1f, 100.0f);
	camera.position = sbgl_Vec3Set(0.0f, -2.0f, -3.0f);
	camera.target = sbgl_Vec3Set(0.0f, 0.0f, 0.0f);
	camera.up = sbgl_Vec3Set(0.0f, -1.0f, 0.0f);

	float start_time = (float)clock() / CLOCKS_PER_SEC;

	printf("--- Camera Controls ---\n");
	printf("ESC: Exit\n");
	printf("-----------------------\n");

	// Define a batch of AABBs for collision testing
	sbgl_AABB boxes[] = {
		{ sbgl_Vec3Set(-1.0f, -1.0f, -1.0f), sbgl_Vec3Set(1.0f, 1.0f, 1.0f) },
		{ sbgl_Vec3Set(2.0f, -0.5f, -0.5f), sbgl_Vec3Set(3.0f, 0.5f, 0.5f) },
		{ sbgl_Vec3Set(-3.0f, -0.5f, -0.5f), sbgl_Vec3Set(-2.0f, 0.5f, 0.5f) }
	};

	sbgl_HitResult results[3];

	while (!sbgl_WindowShouldClose(ctx)) {
		if (sbgl_GetInputState(ctx)->keysDown[SBGL_KEY_ESCAPE])
			break;

		sbgl_Clear(ctx, 0.0f, 0.0f, 0.0f, 0.0f);
		sbgl_BeginDrawing(ctx);

		float time = (float)clock() / CLOCKS_PER_SEC - start_time;
		sbgl_Mat4 model = sbgl_Mat4Rotate(time * 10.5f, sbgl_Vec3Set(0.0f, 1.0f, 0.0f));

		// Perform ray-casting test from the camera position forward
		sbgl_Ray ray;
		ray.origin = camera.position;
		ray.direction = sbgl_Vec3Normalize(sbgl_Vec3Sub(camera.target, camera.position));
		sbgl_RayAABBIntersectBatch(ray, boxes, results, 3);

		for (int i = 0; i < 3; i++) {
			if (results[i].hit) {
				// Simple logging of hits (in a real app, this could trigger visual feedback)
				// printf("Hit Box %d at distance %.2f\n", i, results[i].distance);
			}
		}

		PushData push = { .viewProj = sbgl_Mat4Mul(
							  sbgl_CameraGetProjection(&camera),
							  sbgl_Mat4Mul(sbgl_CameraGetView(&camera), model)
						  ) };

		sbgl_BindPipeline(ctx, pipeline);
		sbgl_PushConstants(ctx, sizeof(push), &push);
		sbgl_BindBuffer(ctx, vbo, SBGL_BUFFER_USAGE_VERTEX);
		sbgl_Draw(ctx, 18, 0);

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
