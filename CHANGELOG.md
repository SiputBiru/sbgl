# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased] - 2026-05-02

### Added
- **Core Architecture**: Implemented a modular HAL (Hardware Abstraction Layer) separating Core, Platform, and Graphics Backend.
- **Build System**: Created a robust `CMakeLists.txt` with automatic Wayland detection, protocol generation via `wayland-scanner`, and Vulkan linkage.
- **Platform HAL**: Defined `sbgl_platform.h` for OS-agnostic windowing, input, and timing.
- **Native Wayland**: Implemented `sbgl_platform_linux.c` using native Wayland protocols (`xdg-shell`). Handles the async event loop and input listeners.
- **Vulkan Backend**:
    - Vulkan 1.3 Instance and Wayland Surface creation.
    - Automatic Physical Device selection (preferring discrete GPUs).
    - Logical Device creation with **Dynamic Rendering** enabled.
    - Swapchain management with image view generation.
    - **Clear Screen**: Implemented `BeginFrame` and `EndFrame` to clear the screen to a deep blue color using dynamic rendering.
- **Public API**: Defined `sbgl.h` with a clean, Raylib-style interface and physical scancode definitions.
- **Memory Management**: Integrated `sbl_arena.h` for zero-isolated-malloc window state allocation.
- **Opaque Types**: Created `sbgl_types.h` to centralize forward declarations and resolve C99 typedef redefinition warnings.
- **Input Refinement**: 
    - Decoupled the input system into a dedicated HAL (`sbgl_input.h`) and platform-specific implementations.
    - Added support for one-shot key triggers (`sbgl_IsKeyPressed`).
    - Implemented full Wayland pointer support (position, delta, buttons).
- **Git Integration**: Initialized git repository and added a comprehensive `.gitignore`.
- **Documentation**: 
    - Integrated **Doxygen** for automated API documentation generation.
    - Added a `docs` target to CMake (`cmake --build build --target docs`).
    - Added `VULKAN_BACKEND.md` with a detailed explanation of the engine's graphics architecture.
- **LSP Support**: Enabled `CMAKE_EXPORT_COMPILE_COMMANDS` to provide full context for `clangd` and other language servers.
- **Examples**: Added `hello_window.c` and `input_test.c` (interactive color switching) to verify the entire stack.

### Fixed
- **Wayland Protocol Crashes**: Resolved "listener function is NULL" errors by providing complete callback implementations for `wl_keyboard` and `wl_pointer`.
- **Wayland Window Visibility**: Resolved the issue where windows remained invisible until a valid Vulkan buffer was attached and submitted.
- **GPU Synchronization**: Fixed a potential hang in the frame lifecycle by adopting an explicit `BeginDrawing`/`EndDrawing` pattern.
- **C99 Compatibility**: Fixed header redefinition errors by refactoring forward declarations into a dedicated types header.
