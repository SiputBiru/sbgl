# Installation Guide

This guide covers setting up the development environment and building SBgl for different platforms.

## Prerequisites

* **Compiler**: GCC 9+, Clang 10+, or MSVC 2019+.
* **Build System**: CMake 3.15+.
* **Graphics**: Vulkan SDK (Headers and `glslc` compiler required). Static linking to `vulkan-1` or `libvulkan` is **not** required as SBgl uses the `volk` meta-loader.

---

## Configuration Flags

SBgl provides several CMake options to customize the build:

| Flag | Description | Default |
| :--- | :--- | :--- |
| `SBGL_BUILD_EXAMPLES` | Compiles the example applications in `examples/`. | `OFF` |
| `SBGL_BUILD_TESTS` | Compiles the unit tests in `tests/`. | `OFF` |
| `SBGL_USE_WAYLAND` | (Linux Only) Uses native Wayland. If `OFF`, X11 is used. | `ON` |
| `CMAKE_BUILD_TYPE` | Set to `Debug` for development or `Release` for performance. | `Release` |

---

## Dependency Management

SBgl utilizes a hybrid dependency model to minimize manual setup:

* **Automatic (`FetchContent`)**: High-level dependencies like **volk** are automatically downloaded and configured during the CMake configuration phase.
* **External (System)**: Platform-specific libraries (Wayland, X11) and the Vulkan SDK must be present on the host system.
* **Internal (Embedded)**: Small, critical utilities (like `stb_perlin`) are embedded directly in the source tree.

---

## Installing on Linux (Wayland or X11)

### System Dependencies

Install the required development libraries for the preferred display protocol:

* **Wayland**: `libwayland-dev`, `wayland-protocols`.
* **X11**: `libx11-dev`.
* **Vulkan**: Vulkan SDK headers and `glslc`.

### Build Instructions

```bash
# Default build (Wayland enabled)
cmake -B build -DSBGL_BUILD_EXAMPLES=ON
cmake --build build

# X11 build
cmake -B build-x11 -DSBGL_USE_WAYLAND=OFF -DSBGL_BUILD_EXAMPLES=ON
cmake --build build-x11
```

---

## Installing on Windows (Native)

### Prerequisites

* **Visual Studio 2019/2022** with the "Desktop development with C++" workload.
* **Vulkan SDK**: Ensure `VULKAN_SDK` environment variable is set.

### Build Instructions

Using the Developer Command Prompt or PowerShell:

```powershell
cmake -B build -DSBGL_BUILD_EXAMPLES=ON
cmake --build build --config Release
```

---

## Cross-Compilation (Linux to Windows)

SBgl supports building Windows binaries from a Linux host using the MinGW-w64 toolchain.

### Prerequisites

Install the MinGW-w64 compiler:

* **Debian/Ubuntu**: `sudo apt install mingw-w64`
* **Arch**: `sudo pacman -S mingw-w64-gcc`

### Build Instructions

Specify the Windows system name and the MinGW compiler to CMake:

```bash
cmake -B build-win \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DSBGL_BUILD_EXAMPLES=ON

cmake --build build-win
```

---

## Integration via FetchContent

The simplest way to use SBgl in an external project is via CMake's `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
    sbgl
    GIT_REPOSITORY https://github.com/SiputBiru/sbgl.git
    GIT_TAG main
)
FetchContent_MakeAvailable(sbgl)

target_link_libraries(target_application PRIVATE sbgl)
```
