# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased] - 2026-05-03

### Added
- **Phase 4: Rendering Pipeline**:
    - Implemented a complete rendering pipeline using Vulkan 1.3 **Dynamic Rendering**, eliminating RenderPass and Framebuffer management.
    - **Resource Management**: Introduced a handle-based system (`sbgl_Buffer`, `sbgl_Shader`, `sbgl_Pipeline`) for GPU resources using internal SoA pools for DOD-compliant performance.
    - **Buffer Allocator**: Implemented a block-based GPU memory allocator for Vertex and Index buffers with staging support for CPU-to-GPU transfers.
    - **Shader System**: Added SPIR-V shader loading with support for both dynamic file-based loading and hardcoded byte arrays (via `xxd`).
    - **Explicit PSO**: Implemented Pipeline State Object (PSO) creation with configurable vertex layouts and shader stages.
    - **Interactive Rendering**: Added **Push Constants** support to the public API and Vulkan backend, enabling low-latency per-frame data updates (e.g., mouse position).
    - **New Examples**:
        - `draw_triangle.c`: Static triangle rendering using VBOs.
        - `draw_interactive_triangle.c`: Rainbow triangle responding to mouse cursor position.
        - `draw_hardcoded_triangle.c`: Demonstrates self-contained applications with embedded shaders.
    - **Documentation**: Created `docs/RENDERING_PIPELINE.md` covering the new architecture and usage workflows.

- **Documentation System Refinement**:
    - Decoupled documentation generation from the source tree by relocating Doxygen output from `docs/out` to `build/docs`.
    - Updated `CMakeLists.txt` to dynamically configure Doxygen output paths, ensuring a cleaner root directory.
    - Automated image asset synchronization within the build-local documentation site.
- **Structural Cleanup**:
    - Moved the `shaders/` directory into `examples/shaders/` to reduce root directory clutter and better reflect its purpose as example-only assets.
    - Optimized the shader build pipeline to ensure generated assets stay within the build environment.

- **SIMD-Ready Math Library**:
    - Implemented a single-header math library (`sbgl_math.h`) providing Vector (Vec2, Vec3, Vec4), Matrix (Mat4), and Quaternion types.
    - Optimized memory layouts with 16-byte alignment and padding to facilitate efficient SIMD instruction generation and cache line utilization.
    - Implemented a precision-tuned Fast Inverse Square Root (`sbgl_InvSqrt`) based on the Quake III Arena algorithm using C99-compliant union punning.
    - Provided a comprehensive set of affine transformations: Translation, Scaling, Rotation (Axis-Angle), Perspective, Orthographic, and LookAt.
    - Added type-safe constructor functions (e.g., `sbgl_vec3()`) for natural initialization syntax.
    - Integrated the math library into the Doxygen documentation system with automatic symbol linking.
    - Created a technical architecture guide (`docs/MATH_LIB.md`) with usage examples and implementation rationales.

### Changed
- **Data-Oriented Input API**:
    - Transitioned the input system to a Data-Oriented Design (DOD) model to improve cache utilization and enable batch processing.
    - Removed single-instance query functions: `sbgl_IsKeyDown`, `sbgl_IsKeyPressed`, `sbgl_IsMouseButtonDown`, `sbgl_GetMousePos`, and `sbgl_GetMouseDelta`.
    - Introduced `sbgl_GetInputState`, providing a read-only pointer to the contiguous `sbgl_InputState` structure for direct array access.
    - Implemented a static, zero-initialized dummy state to handle null-context edge cases without internal branching.

## [Unreleased] - 2026-05-02

### Added
- **Window Resizing**:
    - Implemented automatic swapchain recreation on window resize events across Wayland, X11, and Win32.
    - Added `sbgl_GetWindowSize` to the public API for context-based dimension queries.
    - Integrated synchronization to handle window minimization (0x0 dimensions) gracefully.
- **Input System Refactor**:
    - Encapsulated all physical input state into a `sbgl_InputState` structure tied directly to the `sbgl_Context`.
    - Eliminated global input state in favor of context-local state, enabling zero-overhead real-time input access via "key map" arrays.
    - Added `keysPressed` tracking for frame-accurate trigger detection.
- **Arena-Based Memory Management**:
    - Fully eliminated `malloc` and `free` from the platform and Vulkan layers.
    - Implemented a **Mark/Rewind** pattern for the Vulkan backend to recycle arena memory during swapchain recreation without leaks.

### Changed
- **Core Architecture**:
    - Updated internal HAL signatures to pass the engine context or specialized state pointers instead of relying on globals.
    - Updated `sbgl_gfx_Init` to accept the context's `SblArena` for graphics-layer allocations.
- **Public API**:
    - Updated input helpers (`sbgl_IsKeyDown`, etc.) to perform safety checks and read directly from the context's internal state.

### Added
- **Core Architecture**: Implemented a modular HAL (Hardware Abstraction Layer) separating Core, Platform, and Graphics Backend.
- **Build System**: 
    - Created a `CMakeLists.txt` with automatic backend detection and protocol generation.
    - Added `SBGL_BUILD_EXAMPLES` option (defaults to **OFF** for a minimal core build).
    - Added `SBGL_BUILD_STANDALONE` option for compiling library sources directly into executables (Unity-style build).
    - Enabled `CMAKE_EXPORT_COMPILE_COMMANDS` for LSP context support.
    - Added `docs-clean` target to manage documentation artifacts.
- **Platform HAL**: Defined `sbgl_platform.h` for OS-agnostic windowing, timing, and native integration.
- **Multi-Platform Support**: 
    - Fully implemented the **Win32 Platform Layer** (`window.c`, `input.c`) with virtual key mapping.
    - Fully implemented the **X11 Platform Layer** (`window_x11.c`, `input_x11.c`).
    - Implemented a native **Wayland Platform Layer** using XDG-Shell protocols (`window_wayland.c`, `input_wayland.c`).
    - Reorganized Linux implementations into a modular, decoupled structure using a shared `linux_internal.h`.
    - Added build-time switching between backends via the `SBGL_USE_WAYLAND` CMake option.
- **Vulkan Backend**:
    - Implemented **Dynamic Vulkan Loading**: The engine now loads `libvulkan.so` or `vulkan-1.dll` at runtime, eliminating build-time link dependencies.
    - Vulkan 1.3 Instance and Surface creation for Wayland, X11, and Win32.
    - Automatic Physical Device selection (preferring discrete GPUs).
    - Logical Device creation with **Dynamic Rendering** enabled.
    - Swapchain management with image view generation.
    - **Clear Screen**: Implemented `BeginFrame` and `EndFrame` to clear the screen to a color using dynamic rendering.
- **Public API**: Defined `sbgl.h` with a Raylib-style interface, physical scancode definitions, and explicit Context management.
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
- **Compiler Warnings**:
    - Silenced ISO C99 pedantic warnings regarding anonymous structs in `sbgl_math.h` using the `__extension__` keyword.
    - Eliminated unused parameter warnings in Wayland platform callbacks by implementing explicit `(void)` casts, ensuring a clean build with `-Wall -Wextra -Wpedantic`.
- **Memory Safety**:
 Resolved a segmentation fault in `sbl_arena_free` caused by a Use-After-Free when the arena structure was self-contained within its own memory blocks.
- **GPU Synchronization**: Fixed a Vulkan driver crash during rapid window resizing by implementing internal frame lifecycle tracking (`isDrawing` flag), preventing out-of-order command submissions.
- **Wayland Protocol Crashes**: Resolved "listener function is NULL" errors by providing complete callback implementations for `wl_keyboard` and `wl_pointer`.
- **Wayland Window Visibility**: Resolved the issue where windows remained invisible until a valid Vulkan buffer was attached and submitted.
- **Wayland ANR**: Fixed "Application Not Responding" dialogs by restoring the XDG-Shell ping/pong heartbeat.
- **GPU Synchronization**: Fixed a potential hang in the frame lifecycle by adopting an explicit `BeginDrawing`/`EndDrawing` pattern.
- **C99 Compatibility**: Fixed header redefinition errors by refactoring forward declarations into a dedicated types header.
