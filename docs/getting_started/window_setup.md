# Window Setup

Initializing the SBgl context and creating a native window.

## Initializing the Context

SBgl uses an explicit context for all operations.

```c
#include <sbgl.h>

int main() {
    sbgl_InitResult res = sbgl_Init(800, 600, "My App");
    if (res.error != SBGL_SUCCESS) return 1;
    sbgl_Context* ctx = res.ctx;
    
    // ...
    
    sbgl_Shutdown(ctx);
    return 0;
}
```

## The Main Loop

```c
while (!sbgl_WindowShouldClose(ctx)) {
    sbgl_os_PollEvents(ctx); // OS event processing
    
    sbgl_BeginDrawing(ctx);
    // Rendering operations
    sbgl_EndDrawing(ctx);
}
```

