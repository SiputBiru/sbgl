# SBgl Examples

The `examples/` directory contains several applications demonstrating various features of the library.

---

## Getting Started

### 1. Build the Examples
Examples are integrated into the main CMake build system. From the project root:

```bash
mkdir build
cd build
cmake ..
make
```

### 2. Run an Example
After building, binaries are located in the `build/examples/` directory. For example:

```bash
./examples/window/hello_window
```

---

## Core Examples

*   **`window/hello_window.c`**: Demonstrates the basic initialization of the SBgl context, window creation, and the main event loop.
*   **`triangle/draw_triangle.c`**: Shows how to load shaders, create vertex buffers, and draw a basic static triangle.
*   **`triangle/draw_interactive_triangle.c`**: Illustrates how to use Push Constants to pass dynamic data (like time) to the GPU for interactive effects.
*   **`camera/camera3d_pyramid.c`**: A complete 3D example demonstrating the perspective camera, 3D transformations, and depth buffering.
*   **`input/input_keyboard.c`**: Showcases the Data-Oriented input system, polling for key presses and held states.
*   **`input/input_mouse.c`**: Demonstrates mouse coordinate tracking and color mapping.

---

## Basic Example Pattern

Most SBgl applications follow this standard initialization and main loop pattern:

```c
#include "sbgl.h"
#include <stdio.h>

int main() {
    // 1. Initialize the engine and create a window
    sbgl_InitResult res = sbgl_Init(800, 600, "SBgl Example");
    if (res.error != SBGL_SUCCESS) {
        fprintf(stderr, "Failed to initialize SBgl\n");
        return 1;
    }
    
    sbgl_Context* ctx = res.ctx;

    // 2. Main Execution Loop
    while (!sbgl_WindowShouldClose(ctx)) {
        // Prepare for drawing
        sbgl_BeginDrawing(ctx);
        
        // Clear screen with a color (RGBA)
        sbgl_Clear(ctx, 0.1f, 0.1f, 0.1f, 1.0f);

        // --- Custom Drawing Logic Goes Here ---

        // Present to screen
        sbgl_EndDrawing(ctx);
    }

    // 3. Graceful Shutdown
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
