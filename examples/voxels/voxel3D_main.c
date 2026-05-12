/**
 * @file voxel3D_main.c
 * @brief Zero-Sync Infinite Voxel Engine Flagship Example (Refactored).
 */

#include "sbgl.h"
#include "sbgl_camera.h"
#include "sbgl_math.h"
#include "sbgl_voxel.h"
#include <stdio.h>

/**
 * @brief Push constants for the graphics pipeline.
 */
typedef struct {
	sbgl_Mat4 viewProj;
	uint64_t aabbAddress;
	uint64_t voxelDataAddress;
	uint64_t paletteAddress;
} RenderPushConstants;

int main(void) {
	/*
	 * Initialize the SBgl framework with a standard window configuration.
	 */
	sbgl_InitResult res = sbgl_Init(1280, 720, "SBgl Zero-Sync Infinite Voxels");
	if (res.error != SBGL_SUCCESS) {
		return 1;
	}
	sbgl_Context* ctx = res.ctx;

	/*
	 * Create the voxel system. This encapsulates all GPU resource management,
	 * compute pipelines, and chunk management logic.
	 */
	sbgl_VoxelConfig vConfig = { .max_slots = 256, .chunk_radius = 3, .enable_telemetry = true };
	sbgl_VoxelSystem* voxelSys = sbgl_Voxel_Create(ctx, &vConfig);

	if (!voxelSys) {
		fprintf(stderr, "Failed to initialize VoxelSystem\n");
		sbgl_Shutdown(ctx);
		return 1;
	}

	/*
	 * Load the shaders for the final rendering pass.
	 * Note: The compute shaders are managed internally by the voxel system.
	 */
	sbgl_Shader vertShader =
		sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_VERTEX, "shaders/voxel3D.vert.spv");
	sbgl_Shader fragShader =
		sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_FRAGMENT, "shaders/voxel3D.frag.spv");

	if (vertShader == SBGL_INVALID_HANDLE || fragShader == SBGL_INVALID_HANDLE) {
		fprintf(stderr, "Failed to load graphics shaders\n");
		sbgl_Voxel_Destroy(voxelSys);
		sbgl_Shutdown(ctx);
		return 1;
	}

	/*
	 * Configure the graphics pipeline for voxel rendering.
	 */
	sbgl_Pipeline renderPipe = sbgl_CreatePipeline(
		ctx,
		&(sbgl_PipelineConfig){ .vertexShader = vertShader, .fragmentShader = fragShader }
	);

	if (renderPipe == SBGL_INVALID_HANDLE) {
		fprintf(stderr, "Failed to create graphics pipeline\n");
		sbgl_Voxel_Destroy(voxelSys);
		sbgl_Shutdown(ctx);
		return 1;
	}

	/*
	 * Create a simple index buffer for the unit cube.
	 * This is used as the geometry template for all voxel instances.
	 */
	uint32_t idxs[36];
	for (uint32_t i = 0; i < 36; i++)
		idxs[i] = i;
	sbgl_Buffer iBuf = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_INDEX, sizeof(idxs), idxs);

	/*
	 * Setup the interactive camera system.
	 */
	sbgl_Camera camera = sbgl_CameraPerspective(0.8f, 1280.0f / 720.0f, 0.1f, 10000.0f);
	camera.position = sbgl_Vec3Set(0.0f, 400.0f, -800.0f);
	camera.target = sbgl_Vec3Set(0.0f, 128.0f, 0.0f);

	float pitch = -0.4f, yaw = SBGL_PI / 2.0f;
	bool mouseLocked = false;
	float moveSpeed = 400.0f, sensitivity = 0.005f;
	double lastTime = sbgl_GetTime(ctx);

	printf("--- Voxel Controls ---\n");
	printf("W/A/S/D: Move\n");
	printf("Q/E: Vertical Move\n");
	printf("TAB: Lock/Unlock Mouse\n");
	printf("ESC: Exit\n");
	printf("----------------------\n");

	while (!sbgl_WindowShouldClose(ctx)) {
		double currentTime = sbgl_GetTime(ctx);
		float dt = (float)(currentTime - lastTime);
		lastTime = currentTime;

		/* Ensure event polling and GPU command buffer start happen before input query */
		sbgl_BeginCompute(ctx);

		const sbgl_InputState* input = sbgl_GetInputState(ctx);
		if (input->keysDown[SBGL_KEY_ESCAPE])
			break;

		/* Handle Mouse Locking */
		if (input->keysPressed[SBGL_KEY_TAB]) {
			mouseLocked = !mouseLocked;
			sbgl_SetMouseMode(ctx, mouseLocked ? SBGL_MOUSE_MODE_CAPTURED : SBGL_MOUSE_MODE_NORMAL);
		}

		/* Update Camera Orientation */
		if (mouseLocked) {
			yaw += (float)input->mouseDeltaX * sensitivity;
			pitch -= (float)input->mouseDeltaY * sensitivity;
			if (pitch > 1.5f)
				pitch = 1.5f;
			if (pitch < -1.5f)
				pitch = -1.5f;
		}

		sbgl_Vec3 front = sbgl_Vec3Normalize(
			sbgl_Vec3Set(cosf(yaw) * cosf(pitch), sinf(pitch), sinf(yaw) * cosf(pitch))
		);
		sbgl_Vec3 right = sbgl_Vec3Normalize(sbgl_Vec3Cross(front, sbgl_Vec3Set(0, 1, 0)));
		float velocity = moveSpeed * dt;

		/* Handle Movement */
		if (input->keysDown[SBGL_KEY_W])
			camera.position = sbgl_Vec3Add(camera.position, sbgl_Vec3Mul(front, velocity));
		if (input->keysDown[SBGL_KEY_S])
			camera.position = sbgl_Vec3Sub(camera.position, sbgl_Vec3Mul(front, velocity));
		if (input->keysDown[SBGL_KEY_A])
			camera.position = sbgl_Vec3Sub(camera.position, sbgl_Vec3Mul(right, velocity));
		if (input->keysDown[SBGL_KEY_D])
			camera.position = sbgl_Vec3Add(camera.position, sbgl_Vec3Mul(right, velocity));
		if (input->keysDown[SBGL_KEY_Q])
			camera.position.y -= velocity;
		if (input->keysDown[SBGL_KEY_E])
			camera.position.y += velocity;

		camera.target = sbgl_Vec3Add(camera.position, front);
		camera.up = sbgl_Vec3Normalize(sbgl_Vec3Cross(right, front));

		int width, height;
		sbgl_GetWindowSize(ctx, &width, &height);
		camera.aspect = (float)width / (float)height;

		sbgl_Mat4 viewProj =
			sbgl_Mat4Mul(sbgl_CameraGetProjection(&camera), sbgl_CameraGetView(&camera));

		/*
		 * Perform the voxel system update.
		 * This dispatches compute shaders only if the camera crosses a chunk boundary.
		 */
		sbgl_Voxel_Update(voxelSys, camera.position);

		/*
		 * Perform GPU-driven culling before starting the graphics pass.
		 */
		sbgl_Voxel_Cull(voxelSys, viewProj);

		/*
		 * Clear the framebuffer and begin the graphics pass.
		 */
		sbgl_Clear(ctx, 0.5f, 0.7f, 1.0f, 1.0f);
		sbgl_BeginDrawing(ctx);

		/* Bind the rendering pipeline and geometry */
		sbgl_BindPipeline(ctx, renderPipe);
		sbgl_BindBuffer(ctx, iBuf, SBGL_BUFFER_USAGE_INDEX);

		/* Setup graphics push constants using addresses retrieved from the voxel system */
		RenderPushConstants rpc = { .viewProj = viewProj,
									.aabbAddress = sbgl_Voxel_GetAABBAddress(voxelSys),
									.voxelDataAddress = sbgl_Voxel_GetInstanceAddress(voxelSys),
									.paletteAddress = sbgl_Voxel_GetPaletteAddress(voxelSys) };
		sbgl_PushConstants(ctx, sizeof(rpc), &rpc);

		/* Dispatch the voxel rendering commands (Graphics pass only) */
		sbgl_Voxel_Render(voxelSys);

		sbgl_EndDrawing(ctx);
	}

	/*
	 * Gracefully release all resources before shutting down.
	 */
	sbgl_DeviceWaitIdle(ctx);
	sbgl_Voxel_Destroy(voxelSys);
	sbgl_DestroyPipeline(ctx, renderPipe);
	sbgl_DestroyShader(ctx, vertShader);
	sbgl_DestroyShader(ctx, fragShader);
	sbgl_DestroyBuffer(ctx, iBuf);
	sbgl_Shutdown(ctx);

	return 0;
}
