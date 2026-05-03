# SBgl (SiputBiru Graphics Library) {#mainpage}

A bare-metal graphics framework written in C99. Designed for 2D/3D applications with an explicit, context-based API.

[![C99](https://img.shields.io/badge/C-99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![Vulkan](https://img.shields.io/badge/Vulkan-1.3-red.svg)](https://www.vulkan.org/)
![Platform](https://img.shields.io/badge/Platform-Wayland_|_X11_|_Win32-green.svg)

## Core Philosophy

SBgl is built for developers who want control over the hardware without the overhead of heavy engines.

- **Explicit Context**: No global state. Every operation is tied to an explicit sbgl_Context.
- **Data-Oriented Design**: APIs and data layouts are designed for cache efficiency and batch processing.
- **Concurrency**: Implements a Job System for lock-free, data-parallel transformations across worker threads.
- **Vulkan 1.3**: Utilizes Dynamic Rendering and synchronization.
- **Native Platform HAL**: Direct integration with Wayland (XDG-Shell) and Win32, preventing header leakage to user code.
- **Arena-Backed**: Memory is managed via arenas for predictable allocation.

## Prerequisites

- **Compiler**: GCC 9+, Clang 10+, or MSVC 2019+.
- **Build System**: CMake 3.15+.
- **Graphics**: Vulkan SDK 1.3+.
- **Linux**: Wayland and/or X11 development libraries.

## Building from Source

This project uses CMake. Follow these steps to configure, build, and run the framework.

### Configure

Create the build directory and generate the compilation database for the LSP (clangd).

**For Wayland (Default):**

```bash
cmake -B build -S . -DSBGL_USE_WAYLAND=ON
```

**For X11:**

```bash
cmake -B build -S . -DSBGL_USE_WAYLAND=OFF
```

**For Windows (Cross-compile from Linux):**
Requires `mingw-w64` toolchain.

```bash
cmake -B build-win -S . -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DSBGL_BUILD_EXAMPLES=ON -DCMAKE_EXE_LINKER_FLAGS="-static"
```

**Enable Examples:**
To build the test applications alongside the library:

```bash
cmake -B build -S . -DSBGL_BUILD_EXAMPLES=ON
```

**Standalone Build (No Library File):**
Compiles the library source code directly into the example executables (no `libsbgl.a` is created).

```bash
cmake -B build -S . -DSBGL_BUILD_EXAMPLES=ON -DSBGL_BUILD_STANDALONE=ON
```

### Build

Compile the library and all example applications.

```bash
cmake --build build
```

### Clean

SBgl supports standard and deep cleaning operations.

**Standard Clean:**
Removes compiled object files and executables but keeps the configuration.

```bash
cmake --build build --target clean
```

**Documentation Clean:**
Removes the generated `docs/` folder.

```bash
cmake --build build --target docs-clean
```

**Deep Clean (Reset):**
To completely reset the project state, simply delete the build directory.

```bash
rm -rf build
```

### Run Examples

After building, examples located in the build directory can be executed.

```bash
# Run basic window test (Linux natively)
./build/examples/hello_window

# Run interactive input test (Linux natively)
./build/examples/input_test
```

**Testing Windows Builds (via Wine):**
If you cross-compiled for Windows, you can test the executables directly on Linux using Wine.

```bash
wine ./build-win/examples/hello_window.exe
```

## Generating Documentation

SBgl uses **Doxygen** for automated API documentation. A searchable HTML site detailing the internal architecture and public API can be generated.

### Generate

Ensure Doxygen is installed and run the specific documentation target.

```bash
cmake --build build --target docs
```

### View

The output is located in `docs/html/index.html` and can be opened in any web browser.

## Basic Example

```c
#include <sbgl.h>
#include <stdio.h>

int main() {
    // Initialize SBgl with a Result Struct pattern
    sbgl_InitResult res = sbgl_Init(800, 600, "SBgl Window");
    if (res.error != SBGL_SUCCESS) return 1;

    sbgl_Context* ctx = res.ctx;

    while (!sbgl_WindowShouldClose(ctx)) {
        const sbgl_InputState* input = sbgl_GetInputState(ctx);

        // Set clear color (RGBA)
        sbgl_Clear(ctx, 0.1f, 0.2f, 0.3f, 1.0f);

        sbgl_BeginDrawing(ctx);
        // Rendering logic here...
        sbgl_EndDrawing(ctx);

        if (input->keysDown[SBGL_KEY_ESCAPE]) {
            break;
        }
    }

    // Explicit Context cleanup
    sbgl_Shutdown(ctx);
    return 0;
}
```

## Project Structure

- include/: Public API headers.
- src/core/: Engine logic, HAL definitions, and memory management.
- src/platform/: OS-specific implementations (Wayland, Win32).
- src/backend/: Graphics API implementations (Vulkan 1.3).
- examples/: Ready-to-run test applications.

## Documentation

- [VULKAN_BACKEND.md](docs/VULKAN_BACKEND.md): Detailed explanation of the graphics layer.
- [ROADMAP.md](docs/ROADMAP.md): Development milestones and future goals.
- [CHANGELOG.md](CHANGELOG.md): Project history and technical milestones.

---
*Created by SiputBiru.*
