# Installation Guide

The following steps outline the setup of the development environment and the integration of SBgl into an application.

## Prerequisites

*   **Compiler**: GCC 9+, Clang 10+, or MSVC 2019+.
*   **Build System**: CMake 3.15+.
*   **Graphics**: Vulkan SDK 1.3+ (Include headers for compilation).
*   **Linux**: Wayland and/or X11 development libraries.

## Integration via FetchContent

Add this to your `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(
    sbgl
    GIT_REPOSITORY https://github.com/SiputBiru/sbgl.git
    GIT_TAG main
)
FetchContent_MakeAvailable(sbgl)
target_link_libraries(your_project PRIVATE sbgl)
```

## Building from Source

```bash
cmake -B build -DSBGL_BUILD_EXAMPLES=ON
cmake --build build
```
