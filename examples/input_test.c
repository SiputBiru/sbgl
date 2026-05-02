#include <sbgl.h>
#include <stdio.h>

int main() {
    printf("Initializing Input Test (Context API)...\n");
    printf("Controls: R=Red, G=Green, B=Blue, ESC=Exit\n");
    
    sbgl_Context* ctx = sbgl_Init(800, 600, "SBgl Input Test");
    if (ctx->result != SBGL_SUCCESS) {
        printf("Failed to init: %d\n", ctx->result);
        return 1;
    }

    float r = 0.1f, g = 0.2f, b = 0.3f;

    while (!sbgl_WindowShouldClose(ctx)) {
        if (sbgl_IsKeyDown(ctx, SBGL_KEY_R)) {
            r = 1.0f; g = 0.0f; b = 0.0f;
        }
        if (sbgl_IsKeyDown(ctx, SBGL_KEY_G)) {
            r = 0.0f; g = 1.0f; b = 0.0f;
        }
        if (sbgl_IsKeyDown(ctx, SBGL_KEY_B)) {
            r = 0.0f; g = 0.0f; b = 1.0f;
        }

        sbgl_Clear(ctx, r, g, b, 1.0f);
        
        sbgl_BeginDrawing(ctx);
        // Drawing logic here
        sbgl_EndDrawing(ctx);

        if (sbgl_IsKeyDown(ctx, SBGL_KEY_ESCAPE)) {
            break;
        }
    }

    sbgl_Shutdown(ctx);
    return 0;
}
