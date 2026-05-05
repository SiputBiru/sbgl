#include <sbgl.h>
#include "../example_util.h"

int main(void) {
    ExampleApp app;
    if (!example_app_init(&app, 800, 600, "SBgl Hello Window")) return 1;

    while (!sbgl_WindowShouldClose(app.ctx)) {
        if (sbgl_GetInputState(app.ctx)->keysDown[SBGL_KEY_ESCAPE]) break;

        sbgl_Clear(app.ctx, 0.1f, 0.2f, 0.3f, 1.0f);
        sbgl_BeginDrawing(app.ctx);
        sbgl_EndDrawing(app.ctx);
    }

    example_app_shutdown(&app);
    return 0;
}
