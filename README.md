# SBgl (SiputBiru Graphics Library)

[![C99](https://img.shields.io/badge/C-99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![Vulkan](https://img.shields.io/badge/Vulkan-1.3-red.svg)](https://www.vulkan.org/)
![Platform](https://img.shields.io/badge/Platform-Wayland_|_X11_|_Win32-green.svg)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/SiputBiru/sbgl)

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
