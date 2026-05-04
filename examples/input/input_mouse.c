#include <sbgl.h>
#include <stdio.h>

int main() {
	printf("Initializing SBgl Mouse Input Test...\n");
	printf("Controls: Move Mouse to change color, ESC=Exit\n");

	int width = 800;
	int height = 600;
	sbgl_InitResult res = sbgl_Init(width, height, "SBgl Mouse Color Map");
	if (res.error != SBGL_SUCCESS) {
		printf("Failed to init: %d\n", res.error);
		return 1;
	}

	sbgl_Context* ctx = res.ctx;

	while (!sbgl_WindowShouldClose(ctx)) {
		// Fetch current input state once per frame (DOD Batch Access)
		const sbgl_InputState* input = sbgl_GetInputState(ctx);

		// Update window size in case it was resized
		sbgl_GetWindowSize(ctx, &width, &height);

		// Calculate color based on mouse position
		// Map X to Red (0.0 to 1.0)
		// Map Y to Green (0.0 to 1.0)
		float r = (float)input->mouseX / (float)width;
		float g = (float)input->mouseY / (float)height;
		float b = 0.5f; // Constant blue

		// Clamp values just in case
		if (r < 0.0f) {
			r = 0.0f;
		}
		if (r > 1.0f) {
			r = 1.0f;
		}
		if (g < 0.0f) {
			g = 0.0f;
		}
		if (g > 1.0f) {
			g = 1.0f;
		}

		// Apply dynamic clear color
		sbgl_Clear(ctx, r, g, b, 1.0f);

		sbgl_BeginDrawing(ctx);
		// Add additional rendering here if needed
		sbgl_EndDrawing(ctx);

		if (input->keysDown[SBGL_KEY_ESCAPE]) {
			break;
		}
	}

	sbgl_DeviceWaitIdle(ctx);
	sbgl_Shutdown(ctx);
	return 0;
}
