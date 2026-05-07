#include <sbgl.h>
#include <stdio.h>

/*
 * This example demonstrates the different mouse modes available in SBgl.
 * The system supports three primary modes:
 * - NORMAL: Cursor is visible and behaves normally.
 * - HIDDEN: Cursor is invisible but can still leave the window area.
 * - CAPTURED: Cursor is invisible and locked to the window center,
 *   providing continuous relative motion (ideal for first-person cameras).
 */

int main(void) {
	/* Initialize the library and create a window. */
	sbgl_InitResult res = sbgl_Init(800, 600, "SBgl Mouse Mode Test");
	if (res.error != SBGL_SUCCESS)
		return 1;
	sbgl_Context* ctx = res.ctx;

	printf("--- Mouse Mode Controls ---\n");
	printf("1: NORMAL (Visible, Free)\n");
	printf("2: HIDDEN (Invisible, Free)\n");
	printf("3: CAPTURED (Invisible, Locked)\n");
	printf("ESC: Exit\n");

	/* Main application loop. */
	while (!sbgl_WindowShouldClose(ctx)) {
		/* Clear the background with a dark blue color. */
		sbgl_Clear(ctx, 0.1f, 0.1f, 0.15f, 1.0f);
		sbgl_BeginDrawing(ctx);

		/* Retrieve the current input state to check for key presses. */
		const sbgl_InputState* input = sbgl_GetInputState(ctx);

		/* Toggle between mouse modes based on user input. */
		if (input->keysPressed[SBGL_KEY_1]) {
			printf("Mode: NORMAL\n");
			sbgl_SetMouseMode(ctx, SBGL_MOUSE_MODE_NORMAL);
		}
		if (input->keysPressed[SBGL_KEY_2]) {
			printf("Mode: HIDDEN\n");
			sbgl_SetMouseMode(ctx, SBGL_MOUSE_MODE_HIDDEN);
		}
		if (input->keysPressed[SBGL_KEY_3]) {
			printf("Mode: CAPTURED\n");
			sbgl_SetMouseMode(ctx, SBGL_MOUSE_MODE_CAPTURED);
		}

		/* Exit the application if the Escape key is pressed. */
		if (input->keysDown[SBGL_KEY_ESCAPE])
			break;

		/* Mouse movement deltas are tracked regardless of the current mode. */
		if (input->mouseDeltaX != 0 || input->mouseDeltaY != 0) {
			/* Delta information can be printed here for debugging purposes. */
		}

		/* Finalize the current frame and present it. */
		sbgl_EndDrawing(ctx);
	}

	sbgl_DeviceWaitIdle(ctx);
	/* Clean up resources and shut down the context. */
	sbgl_Shutdown(ctx);
	return 0;
}
