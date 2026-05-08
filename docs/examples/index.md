# SBgl Examples

The `examples/` directory contains several applications demonstrating various features of the library.

---

## Getting Started

### Building the Examples

Examples are integrated into the main CMake build system. From the project root:

```bash
mkdir build
cd build
cmake ..
make
```

### Running an Example

After building, binaries are located in the `build/examples/` directory. For example:

```bash
./build/examples/hello_window
```


---

## Core Examples

* **`window/hello_window.c`**: Demonstrates the basic initialization of the SBgl context, window creation, and the main event loop.
* **`triangle/triangle_main.c`**: Shows how to load shaders, create vertex buffers, and draw a basic static or interactive triangle.
* **`camera/camera_main.c`**: A complete 3D example demonstrating the perspective camera, 3D transformations, depth buffering, and batch ray-casting collision math.
* **`input/input_keyboard.c`**: Showcases the Data-Oriented input system, polling for key presses and held states.
* **`input/input_mouse.c`**: Demonstrates mouse coordinate tracking and color mapping.

---

## Batching & GPU Optimization

* **`batching/batch_main.c`**: Demonstrates high-performance batching by rendering 10,000 instances using Multi-Draw Indirect (MDI) and Buffer Device Address (BDA).
* **`voxels/voxel_main.c`**: Showcases "Pure Procedural" rendering. Generates an infinite 2.5D voxel world entirely on the GPU using `gl_VertexIndex`, achieving zero-bandwidth geometry submission.

---

## Basic Example Pattern

Most SBgl applications follow this standard initialization and main loop pattern:

```c
#include "sbgl.h"
#include <stdio.h>

int main() {
    // Initialize the engine and create a window
    sbgl_InitResult res = sbgl_Init(800, 600, "SBgl Example");
    if (res.error != SBGL_SUCCESS) {
        fprintf(stderr, "Failed to initialize SBgl\n");
        return 1;
    }
    
    sbgl_Context* ctx = res.ctx;

    // Main Execution Loop
    while (!sbgl_WindowShouldClose(ctx)) {
        // Prepare for drawing
        sbgl_BeginDrawing(ctx);
        
        // Clear screen with a color (RGBA)
        sbgl_Clear(ctx, 0.1f, 0.1f, 0.1f, 1.0f);

        // --- Custom Drawing Logic Goes Here ---

        // Present to screen
        sbgl_EndDrawing(ctx);
    }

    // Graceful Shutdown
    sbgl_DeviceWaitIdle(ctx);
    sbgl_Shutdown(ctx);
    return 0;
}
```

---

## Advanced: Interactive Shaders

To pass dynamic data to shaders without the overhead of uniform buffers, SBgl utilizes **Push Constants**.

**C Code:**

```c
float time = get_current_time();
sbgl_PushConstants(ctx, sizeof(float), &time);
```

**GLSL Vertex Shader:**

```glsl
layout(push_constant) uniform Constants {
    float time;
} push;

void main() {
    // Use push.time to animate vertices
}
```
