# SBgl (SiputBiru Graphics Library)

<p align="left">
  <a href="https://en.wikipedia.org/wiki/C99"><img src="https://img.shields.io/badge/C-99-blue.svg" alt="C99"></a>
  <a href="https://www.vulkan.org/"><img src="https://img.shields.io/badge/Vulkan-1.3-red.svg" alt="Vulkan 1.3"></a>
  <img src="https://img.shields.io/badge/Platform-Wayland%20%7C%20X11%20%7C%20Win32-green.svg" alt="Platform">
  <a href="https://github.com/SiputBiru/sbgl/blob/main/LICENSE"><img src="https://img.shields.io/badge/License-zlib-lightgrey.svg" alt="License"></a>
  <a href="https://github.com/SiputBiru/sbgl"><img src="https://img.shields.io/badge/build-passing-brightgreen.svg" alt="Build Status"></a>
</p>

A graphics framework engineered for target hardware. Built on C99 and Vulkan 1.3, prioritize in Data-Oriented Design to maximize cache efficiency. By employing an explicit, context-based API and handle-based resource management, SBgl eliminates hidden state and enables control over the rendering pipeline, memory allocation, and multi-threaded command recording.

> [!WARNING]
> **API Instability Notice**: SBgl is currently in an experimental phase of development. The API is considered unstable and is subject to significant changes or complete removal at any given moment without prior notice. Use in production environments is not recommended.

## Dependencies

| Dependency | Version | Purpose | Integration |
| :--- | :--- | :--- | :--- |
| **[volk](https://github.com/zeux/volk)** | 1.4.350 | Vulkan Meta-loader for device-specific function dispatching. | CMake FetchContent (Automatic) |
| **Wayland** | Native | Primary windowing system for Linux development. | System Package (External) |
| **X11** | Native | Legacy windowing support for Linux platforms. | System Package (External) |
| **stb_perlin** | 0.5 | Single-file noise generator for procedural voxel terrain. | Source-embedded (Internal) |
| **glslc** | SDK | SPIR-V shader compiler (Khronos/Google). | Vulkan SDK (External) |
| **xxd** | OS | Utility for embedding binary assets into C source code. | System Package (External) |

## Quick Start

### Integrating SBgl

The simplest way to integrate SBgl into a C/C++ project is using CMake's `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
    sbgl
    GIT_REPOSITORY https://github.com/SiputBiru/sbgl.git
    GIT_TAG main
)
FetchContent_MakeAvailable(sbgl)

# Link target against SBgl
target_link_libraries(your_application PRIVATE sbgl)
```

### Basic example

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
        
        // Set clear color for the next frame (RGBA)
        sbgl_SetClearColor(ctx, 0.1f, 0.1f, 0.1f, 1.0f);

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

### Custom Resource Limits

For applications requiring more resources, use `sbgl_InitWithConfig()`:

```c
sbgl_InitConfig config = {
    .windowWidth = 1920,
    .windowHeight = 1080,
    .windowTitle = "High-Resource Application",
    .limits = {
        .maxBuffers = 4096,      // Default: 1024
        .maxShaders = 512,       // Default: 256
        .maxPipelines = 1024     // Default: 256
    },
    .enableValidation = true
};

sbgl_InitResult res = sbgl_InitWithConfig(&config);
```

Alternatively, start from defaults and override specific fields:

```c
sbgl_InitConfig config = sbgl_DefaultInitConfig;
config.windowWidth = 1920;
config.windowHeight = 1080;
config.limits.maxBuffers = 4096;

sbgl_InitResult res = sbgl_InitWithConfig(&config);
```

### Building from scratch

```bash
git clone https://github.com/SiputBiru/sbgl.git

# Release build (default)
cmake -B build -DSBGL_BUILD_EXAMPLES=ON
cmake --build build

# Debug build (with symbols, no optimization)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DSBGL_BUILD_EXAMPLES=ON
cmake --build build

./build/examples/hello_window
```

Full installation details are available in the [Getting Started: Installation](https://siputbiru.github.io/sbgl/md_docs_2getting__started_2installation.html) guide.

## Documentation

Comprehensive documentation organized by the development lifecycle is available in the [Documentation Index](docs/INDEX.md) or [Documentation website](https://siputbiru.github.io/sbgl).

### Chapters

* **Foundations**: Initialization, Windowing, and Input.
* **Graphics HAL**: Shaders, Buffers, and Pipelines.
* **Data-Oriented Pipeline**: Render Queues, Sorting, and Batching.
* **Advanced Techniques**: BDA, MDI, and Arena Management.

## Features

* Bare-metal C99 architecture.
* Explicit Context-based API (no global state).
* Vulkan 1.3 backend.
* General-purpose Compute Shaders & GPU-Driven Culling.
* Data-Oriented Design (DOD) for cache efficiency.
* Arena-based memory management.
* SIMD-ready math library.
* Native Platform HAL (Wayland, X11, Win32).

## Limitations

As a "bare-metal" framework in active development, SBgl has several known technical constraints:

* **Single Command Stream**: While the API supports multiple contexts, command recording is currently serialized into a single primary command buffer per frame. Asynchronous, multi-threaded command recording is on the roadmap but not yet available.
* **Linux-First Maturity**: Although Win32 is supported, the Linux (Wayland/X11) platform layers are the primary development targets and currently offer the highest stability and feature parity.
* **Resource Limits**: While resource limits (buffers, shaders, pipelines) are configurable at initialization via `sbgl_InitWithConfig()`, they remain fixed for the lifetime of the context to ensure O(1) handle lookups and avoid heap fragmentation.

## Performance & Rendering Techniques

The system utilizes several rendering techniques to ensure execution efficiency and reduced CPU-GPU overhead.

| Technique | Implementation Detail | Performance Benefit |
| :--- | :--- | :--- |
| **GPU-Driven Culling** | Visibility testing and indirect command generation performed via Compute Shaders. | Eliminates CPU-side culling bottlenecks and minimizes PCIe bandwidth by keeping draw data on the GPU. |
| **Multi-Draw Indirect (MDI)** | Draw commands are baked into a GPU-visible buffer and submitted via `vkCmdDrawIndexedIndirect`. | Reduces CPU submission overhead and eliminates per-draw call driver validation. |
| **Buffer Device Address (BDA)** | Uses `GL_EXT_buffer_reference` to access instance data via 64-bit pointers in push constants. | Eliminates descriptor set updates and provides direct memory access within shaders. |
| **Data-Oriented Design (DOD)** | Contiguous memory layouts for render queues and instance data. | Maximizes L1/L2 cache utilization during packet submission and sorting phases. |
| **Stable Radix Sort** | Sorting of draw packets by mesh and material identifiers. | Minimizes pipeline state changes and increases the effectiveness of indirect command merging. |
| **Memory Arenas** | Allocation of render-loop resources through `SblArena`. | Eliminates per-frame heap fragmentation and allocation latency in the hot path. |

## Internal Development Notes

* Transition from global includes/definitions to target-scoped properties (`target_include_directories`, `target_compile_definitions`) for better encapsulation.
* Move tool discovery (`glslc`, `xxd`) to top-level CMake to avoid redundant lookups.
* Modernize Wayland protocol discovery using `pkg_get_variable`.
* Enable missing engine tests (batcher, sort, heightmap, voxel_logic) in `tests/CMakeLists.txt` with a dedicated `add_engine_test` macro.
* Update `Doxyfile` to include all manual/getting_started documentation and correctly map the image asset path.
* Standardize on `PROJECT_BINARY_DIR` for generated assets to ensure compatibility when integrated as a subproject.
* Implemented a default Release build configuration. Use -DCMAKE_BUILD_TYPE=Debug to enable symbols and strict warnings for development.
* Enforced 16-byte alignment in `SblArena` to support SIMD-optimized types in Release builds.
* Standardized vertex position `w` component to `1.0` in examples to ensure correct perspective projection in shaders.
* Increased default `transientArena` size to 16MB to support high-frequency batching of up to 100,000 instances.
* Transitioned the voxel engine to a 3D chunked SSBO structure with GPU-driven frustum culling.
* Implemented the General Purpose Compute API for pipeline dispatching and memory synchronization.

## License

SBgl is licensed under the [zlib License](https://github.com/SiputBiru/sbgl/blob/main/LICENSE).
