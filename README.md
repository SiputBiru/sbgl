# SBgl (SiputBiru Graphics Library)

A bare-metal graphics framework written in C99. Provides an explicit, context-based API for 2D/3D applications.

## Quick Start

```bash
git clone https://github.com/SiputBiru/sbgl.git
cmake -B build -DSBGL_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/hello_window
```

## Documentation

### Getting Started
*   [Installation](docs/getting_started/installation.md)
*   [Window Setup](docs/getting_started/window_setup.md)
*   [First Triangle](docs/getting_started/first_triangle.md)

### Manual
*   [Input System](docs/manual/input_system.md)
*   [Math Library](docs/manual/math_library.md)
*   [Memory Management](docs/manual/memory_management.md)
*   [Platform Abstraction](docs/manual/platform_abstraction.md)
*   [Rendering Pipeline](docs/manual/rendering_pipeline.md)
*   [Vulkan Backend](docs/manual/vulkan_backend.md)
*   [Camera System](docs/manual/camera_system.md)

### Resources
*   [Categorized Examples](docs/examples/index.md)
*   [Roadmap](docs/roadmap.md)
*   [Changelog](CHANGELOG.md)

## Features

*   Bare-metal C99 architecture.
*   Explicit Context-based API (no global state).
*   Vulkan 1.3 backend.
*   Data-Oriented Design (DOD) for cache efficiency.
*   Arena-based memory management.
*   SIMD-ready math library.
*   Native Platform HAL (Wayland, X11, Win32).
