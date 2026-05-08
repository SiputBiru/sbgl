# SBgl (SiputBiru Graphics Library)

[![C99](https://img.shields.io/badge/C-99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![Vulkan](https://img.shields.io/badge/Vulkan-1.3-red.svg)](https://www.vulkan.org/)
![Platform](https://img.shields.io/badge/Platform-Wayland_|_X11_|_Win32-green.svg)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/SiputBiru/sbgl)

> [!WARNING]
> **API Instability Notice**: SBgl is currently in an experimental phase of development. The API is considered unstable and is subject to significant changes or complete removal at any given moment without prior notice. Use in production environments is not recommended.

A "low-level" graphics framework engineered for modern hardware. Built on C99 and Vulkan 1.3, prioritize in Data-Oriented Design to maximize cache efficiency. By employing an explicit, context-based API and handle-based resource management, SBgl eliminates hidden state and provides total control over the rendering pipeline, memory allocation, and multi-threaded command recording.

## Quick Start

### Try the Examples

```bash
git clone https://github.com/SiputBiru/sbgl.git
cmake -B build -DSBGL_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/hello_window
```

### Use SBgl in Your Project

The simplest way to integrate SBgl into your own C/C++ project is using CMake's `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
    sbgl
    GIT_REPOSITORY https://github.com/SiputBiru/sbgl.git
    GIT_TAG main
)
FetchContent_MakeAvailable(sbgl)

# Link your target against SBgl
target_link_libraries(your_application PRIVATE sbgl)
```

## Documentation

### Getting Started

* [Installation](docs/getting_started/installation.md)
* [Window Setup](docs/getting_started/window_setup.md)
* [First Triangle](docs/getting_started/first_triangle.md)

### Manual

* [Input System](docs/manual/input_system.md)
* [Math Library](docs/manual/math_library.md)
* [Memory Management](docs/manual/memory_management.md)
* [Platform Abstraction](docs/manual/platform_abstraction.md)
* [Rendering Pipeline](docs/manual/rendering_pipeline.md)
* [Vulkan Backend](docs/manual/vulkan_backend.md)
* [Voxel Rendering](docs/manual/voxel_rendering.md)
* [Camera System](docs/manual/camera_system.md)

### Resources

* [Categorized Examples](docs/examples/index.md)
* [Roadmap](docs/roadmap.md)
* [Changelog](CHANGELOG.md)

## Features

* Bare-metal C99 architecture.
* Explicit Context-based API (no global state).
* Vulkan 1.3 backend.
* Data-Oriented Design (DOD) for cache efficiency.
* Arena-based memory management.
* SIMD-ready math library.
* Native Platform HAL (Wayland, X11, Win32).

## Performance & Rendering Techniques

The system utilizes several modern rendering techniques to ensure high-performance execution and minimal CPU-GPU overhead.

| Technique | Implementation Detail | Performance Benefit |
| :--- | :--- | :--- |
| **Multi-Draw Indirect (MDI)** | Draw commands are baked into a GPU-visible buffer and submitted via `vkCmdDrawIndexedIndirect`. | Reduces CPU submission overhead and eliminates per-draw call driver validation. |
| **Buffer Device Address (BDA)** | Uses `GL_EXT_buffer_reference` to access instance data via 64-bit pointers in push constants. | Eliminates descriptor set updates and provides fast, direct memory access within shaders. |
| **Data-Oriented Design (DOD)** | Contiguous memory layouts for render queues and instance data. | Maximizes L1/L2 cache utilization during packet submission and sorting phases. |
| **Stable Radix Sort** | High-performance CPU sorting of draw packets by mesh and material identifiers. | Minimizes pipeline state changes and maximizes the effectiveness of indirect command merging. |
| **Memory Arenas** | Allocation of render-loop resources through `SblArena`. | Eliminates per-frame heap fragmentation and allocation latency in the hot path. |

* Transition from global includes/definitions to target-scoped properties (target_include_directories, target_compile_definitions) for better encapsulation.
* Move tool discovery (glslc, xxd) to top-level CMake to avoid redundant lookups.
* Modernize Wayland protocol discovery using pkg_get_variable.
* Enable missing engine tests (batcher, sort, heightmap, voxel_logic) in tests/CMakeLists.txt with a dedicated add_engine_test macro.
* Update Doxyfile to include all manual/getting_started documentation and correctly map the image asset path.
* Standardize on PROJECT_BINARY_DIR for generated assets to ensure compatibility when integrated as a subproject.
