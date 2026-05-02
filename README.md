# SBgl (SiputBiru Graphics Library)

A professional, bare-metal graphics framework written in C99. Designed for high-performance 2D/3D applications with zero legacy boilerplate and an explicit, context-based API.

[![C99](https://img.shields.io/badge/C-99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![Vulkan](https://img.shields.io/badge/Vulkan-1.3-red.svg)](https://www.vulkan.org/)
[![Platform](https://img.shields.io/badge/Platform-Wayland%20%7C%20Win32-green.svg)](#)

## Core Philosophy

SBgl is built for developers who want total control over the hardware without the overhead of heavy engines.
- **Explicit Context**: No global state. Every operation is tied to an explicit sbgl_Context.
- **Modern Vulkan**: Strictly requires Vulkan 1.3 to utilize Dynamic Rendering and modern synchronization.
- **Native Platform HAL**: Direct integration with Wayland (XDG-Shell) and Win32, preventing header leakage to user code.
- **Arena-Backed**: Memory is managed via arenas for predictable, high-speed allocation.

## Prerequisites

- **Compiler**: GCC 9+, Clang 10+, or MSVC 2019+.
- **Build System**: CMake 3.15+.
- **Graphics**: Vulkan SDK 1.3+.
- **Linux**: Wayland and/or X11 development libraries.

## Building from Source

```bash
# Configure the project
cmake -B build -S .

# Build everything
cmake --build build
```

## Basic Example

```c
#include <sbgl.h>

int main() {
    sbgl_Context* ctx = sbgl_Init(800, 600, "SBgl Window");
    if (ctx->result != SBGL_SUCCESS) return 1;

    while (!sbgl_WindowShouldClose(ctx)) {
        // Prepare frame
        sbgl_Clear(ctx, 0.1f, 0.2f, 0.3f, 1.0f);
        
        sbgl_BeginDrawing(ctx);
        // Your drawing logic here...
        sbgl_EndDrawing(ctx);
    }

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

- [VULKAN_BACKEND.md](./VULKAN_BACKEND.md): Detailed architectural deep-dive into the graphics layer.
- [CHANGELOG.md](./CHANGELOG.md): Project history and milestones.

---
*Created by SiputBiru.*
