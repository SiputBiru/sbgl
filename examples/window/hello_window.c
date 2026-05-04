#include <sbgl.h>
#include <stdio.h>

int main(void) {
	printf("Initializing SBgl Hello Window (Result Struct API)...\n");

	sbgl_InitResult res = sbgl_Init(800, 600, "SBgl Hello Window");
	if (res.error != SBGL_SUCCESS)
		return 1;

	sbgl_Context* ctx = res.ctx;

	while (!sbgl_WindowShouldClose(ctx)) {
		const sbgl_InputState* input = sbgl_GetInputState(ctx);

		sbgl_Clear(ctx, 0.1f, 0.2f, 0.3f, 1.0f);

		sbgl_BeginDrawing(ctx);
		sbgl_EndDrawing(ctx);

		if (input->keysDown[SBGL_KEY_ESCAPE]) {
			break;
		}
	}

	sbgl_DeviceWaitIdle(ctx);
	sbgl_Shutdown(ctx);
	return 0;
}
