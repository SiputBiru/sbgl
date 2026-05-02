# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased] - 2026-05-02

### Added
- **Core Architecture**: Implemented a modular HAL (Hardware Abstraction Layer) separating Core, Platform, and Graphics Backend.
- **Build System**: 
    - Created a robust `CMakeLists.txt` with automatic backend detection and protocol generation.
    - Added `SBGL_BUILD_EXAMPLES` option (defaults to **OFF** for a minimal core build).
    - Added `SBGL_BUILD_STANDALONE` option for compiling library sources directly into executables (Unity-style build).
    - Enabled `CMAKE_EXPORT_COMPILE_COMMANDS` for full LSP context support.
    - Added `docs-clean` target to manage documentation artifacts.
- **Platform HAL**: Defined `sbgl_platform.h` for OS-agnostic windowing, timing, and native integration.
- **Multi-Platform Support**: 
    - Fully implemented the **Win32 Platform Layer** (`window.c`, `input.c`) with full virtual key mapping.
    - Fully implemented the **X11 Platform Layer** (`window_x11.c`, `input_x11.c`).
    - Implemented a native **Wayland Platform Layer** using XDG-Shell protocols (`window_wayland.c`, `input_wayland.c`).
    - Reorganized Linux implementations into a modular, decoupled structure using a shared `linux_internal.h`.
    - Added build-time switching between backends via the `SBGL_USE_WAYLAND` CMake option.
- **Vulkan Backend**:
    - Implemented **Dynamic Vulkan Loading**: The engine now loads `libvulkan.so` or `vulkan-1.dll` at runtime, eliminating build-time link dependencies and improving portability.
    - Vulkan 1.3 Instance and Surface creation for Wayland, X11, and Win32.
    - Automatic Physical Device selection (preferring discrete GPUs).
    - Logical Device creation with **Dynamic Rendering** enabled.
    - Swapchain management with image view generation.
    - **Clear Screen**: Implemented `BeginFrame` and `EndFrame` to clear the screen to a customizable color using dynamic rendering.
- **Public API**: Defined `sbgl.h` with a clean, Raylib-style interface, physical scancode definitions, and muCOSA-inspired explicit Context management.
- **Memory Management**: Integrated `sbl_arena.h` for zero-isolated-malloc window state allocation.
- **Opaque Types**: Created `sbgl_types.h` to centralize forward declarations and resolve C99 typedef redefinition warnings.
- **Input Refinement**: 
    - Decoupled the input system into a dedicated HAL (`sbgl_input.h`) and platform-specific implementations.
    - Replaced raw `memcpy` for keyboard states with type-safe **Struct Assignment**.
    - Added support for one-shot key triggers (`sbgl_IsKeyPressed`).
    - Implemented full native pointer support (position, delta, buttons) for Wayland, X11, and Win32.
- **Git Integration**: Initialized git repository and added a comprehensive `.gitignore`.
- **Documentation**: 
    - Integrated **Doxygen** for automated API documentation generation.
    - Added a `docs` target to CMake (`cmake --build build --target docs`).
    - Added `VULKAN_BACKEND.md` with a detailed explanation of the engine's graphics architecture.
- **Examples**: Added `hello_window.c` and `input_test.c` (interactive color switching) to verify the entire stack.

### Fixed
- **Wayland Protocol Crashes**: Resolved "listener function is NULL" errors by providing complete callback implementations for `wl_keyboard` and `wl_pointer`.
- **Wayland Window Visibility**: Resolved the issue where windows remained invisible until a valid Vulkan buffer was attached and submitted.
- **Wayland ANR**: Fixed "Application Not Responding" dialogs by restoring the XDG-Shell ping/pong heartbeat.
- **GPU Synchronization**: Fixed a potential hang in the frame lifecycle by adopting an explicit `BeginDrawing`/`EndDrawing` pattern.
- **C99 Compatibility**: Fixed header redefinition errors by refactoring forward declarations into a dedicated types header.
