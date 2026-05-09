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

## Quick Start

### Examples

```bash
git clone https://github.com/SiputBiru/sbgl.git
cmake -B build -DSBGL_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/hello_window
```

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

## Documentation

Comprehensive documentation organized by the development lifecycle is available in the [Documentation Index](docs/INDEX.md) or [Documentation website](https://siputbiru.github.io/sbgl).

### Chapters

1. **Foundations**: Initialization, Windowing, and Input.
2. **Graphics HAL**: Shaders, Buffers, and Pipelines.
3. **Data-Oriented Pipeline**: Render Queues, Sorting, and Batching.
4. **Advanced Techniques**: BDA, MDI, and Arena Management.

## Features

* Bare-metal C99 architecture.
* Explicit Context-based API (no global state).
* Vulkan 1.3 backend.
* Data-Oriented Design (DOD) for cache efficiency.
* Arena-based memory management.
* SIMD-ready math library.
* Native Platform HAL (Wayland, X11, Win32).

## Limitations

As a "bare-metal" framework in active development, SBgl has several known technical constraints:

* **2.5D Voxel Rendering**: The current procedural voxel system is optimized for heightmap-based generation (2.5D). True 3D voxel grids with complex overhangs and caves are not yet natively implemented in the primary examples.
* **Single Command Stream**: While the API supports multiple contexts, command recording is currently serialized into a single primary command buffer per frame. Asynchronous, multi-threaded command recording is on the roadmap but not yet available.
* **Linux-First Maturity**: Although Win32 is supported, the Linux (Wayland/X11) platform layers are the primary development targets and currently offer the highest stability and feature parity.
* **Resource Limits**: Internal pools for buffers, shaders, and pipelines use fixed-capacity arrays (e.g., `SBGL_MAX_BUFFERS`) to ensure O(1) handle lookups and avoid heap fragmentation.

## Performance & Rendering Techniques

The system utilizes several rendering techniques to ensure execution efficiency and reduced CPU-GPU overhead.

| Technique | Implementation Detail | Performance Benefit |
| :--- | :--- | :--- |
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

## License

SBgl is licensed under the [zlib License](https://github.com/SiputBiru/sbgl/blob/main/LICENSE).
