# SBgl Examples

The `examples/` directory contains sample applications demonstrating various aspects of the SBgl framework.

## Structure

### Window & Base Setup
*   **`window/hello_window.c`**: Demonstrates the basic initialization of the SBgl context, window creation, and the main event loop.

### Input Handling
*   **`input/input_keyboard.c`**: Shows how to query keyboard state and respond to key presses.
*   **`input/input_mouse.c`**: Demonstrates reading mouse positions, buttons, and calculating mouse deltas for relative movement.

### Camera & Math
*   **`camera/cameraOrtho_example.c`**: Illustrates using the math library to create an orthographic projection and transform coordinates.
*   **`camera/cameraPersp_example.c`**: Demonstrates setting up a perspective projection matrix.
*   **`camera/camera3d_pyramid.c`**: A comprehensive 3D example drawing a rotating colored pyramid, showcasing perspective projection, view matrices, push constants, and backface culling.

### Triangle Rendering
*   **`triangle/draw_hardcoded_triangle.c`**: A minimal example drawing a single triangle by hardcoding vertex data directly into the shaders.
*   **`triangle/draw_triangle.c`**: Draws a single triangle using vertex buffers (`sbgl_Buffer`) to supply the vertex data to the pipeline.
*   **`triangle/draw_interactive_triangle.c`**: An advanced triangle example demonstrating dynamic input, time-based updates, and Push Constants to communicate frame data to the shaders.

### Testing
*   **`test/math_test.c`**: A test suite verifying the correctness of SIMD-ready vector, matrix, and quaternion operations within `sbgl_math.h`.
