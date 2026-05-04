#include <sbgl.h>
#include <stdio.h>

int main() {
	printf("Initializing Input Test (Result Struct API)...\n");
	printf("Controls: R=Red, G=Green, B=Blue, ESC=Exit\n");

	sbgl_InitResult res = sbgl_Init(800, 600, "SBgl Input Test");
	if (res.error != SBGL_SUCCESS) {
		printf("Failed to init: %d\n", res.error);
		return 1;
	}

	sbgl_Context* ctx = res.ctx;
	float r = 0.1f, g = 0.2f, b = 0.3f;

	while (!sbgl_WindowShouldClose(ctx)) {
		const sbgl_InputState* input = sbgl_GetInputState(ctx);

		if (input->keysDown[SBGL_KEY_R]) {
			r = 1.0f;
			g = 0.0f;
			b = 0.0f;
		}
		if (input->keysDown[SBGL_KEY_G]) {
			r = 0.0f;
			g = 1.0f;
			b = 0.0f;
		}
		if (input->keysDown[SBGL_KEY_B]) {
			r = 0.0f;
			g = 0.0f;
			b = 1.0f;
		}

		sbgl_Clear(ctx, r, g, b, 1.0f);

		sbgl_BeginDrawing(ctx);
		// Drawing logic here
		sbgl_EndDrawing(ctx);

		if (input->keysDown[SBGL_KEY_ESCAPE]) {
			break;
		}
	}

	sbgl_DeviceWaitIdle(ctx);
	sbgl_Shutdown(ctx);
	return 0;
}
