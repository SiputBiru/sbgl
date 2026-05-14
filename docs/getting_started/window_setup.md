# Window Setup

Initializing the SBgl context and creating a native window.

## Initializing the Context

SBgl uses an explicit context for all operations.

```c
#include <sbgl.h>

int main() {
    sbgl_InitResult res = sbgl_Init(800, 600, "Application");
    if (res.error != SBGL_SUCCESS) return 1;
    sbgl_Context* ctx = res.ctx;

    // ...

    sbgl_Shutdown(ctx);
    return 0;
}
```

## Initialization with Configuration

For applications requiring custom resource limits or validation settings, use `sbgl_InitWithConfig()`:

```c
sbgl_InitConfig config = {
    .windowWidth = 1920,
    .windowHeight = 1080,
    .windowTitle = "Application",
    .limits = {
        .maxBuffers = 4096,      // Maximum GPU buffers
        .maxShaders = 512,       // Maximum shader modules
        .maxPipelines = 1024     // Maximum graphics/compute pipelines
    },
    .enableValidation = true     // Enable Vulkan validation layers
};

sbgl_InitResult res = sbgl_InitWithConfig(&config);
if (res.error != SBGL_SUCCESS) return 1;
sbgl_Context* ctx = res.ctx;
```

### Using Default Configuration

The `sbgl_DefaultInitConfig` macro provides sensible defaults that can be selectively overridden:

```c
// Start with defaults, override only what is needed
sbgl_InitConfig config = sbgl_DefaultInitConfig;
config.windowWidth = 1920;
config.windowHeight = 1080;
config.limits.maxBuffers = 4096;

sbgl_InitResult res = sbgl_InitWithConfig(&config);
```

### Default Resource Limits

When using `sbgl_Init()` or `sbgl_DefaultInitConfig`, the following defaults apply:

| Resource | Default | Minimum |
|----------|---------|---------|
| Buffers | 1024 | 64 |
| Shaders | 256 | 16 |
| Pipelines | 256 | 16 |

Limits below the minimum are automatically clamped to ensure stability.

## The Main Loop

```c
while (!sbgl_WindowShouldClose(ctx)) {
    sbgl_BeginDrawing(ctx);
    // Rendering operations
    sbgl_EndDrawing(ctx);
}
```

