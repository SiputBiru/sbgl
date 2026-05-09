# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased] - 2026-05-09

### Added
- **Transient GPU Buffer System**: Implemented a persistent, per-frame transient buffer pool in the Vulkan backend (16MB per frame in flight). This eliminates per-frame Vulkan buffer allocations, memory mappings, and deallocations for dynamic data like instances and indirect commands.
- **GPU Transient Allocator**: Added `sbgl_gfx_AllocateTransient` to the internal HAL to enable low-overhead, linear sub-allocation from the persistent transient buffers.

### Changed
- **Optimized Radix Sort**: Refactored `sbgl_radix_sort` to use 8-bit passes (256 buckets) instead of 16-bit passes (65,536 buckets). This significantly reduces CPU cache misses and stack zeroing overhead.
- **Allocation-Free Hot Path**: Updated `sbgl_radix_sort` to accept external workspace buffers, eliminating per-frame `malloc`/`free` calls in the core batching system.
- **Render Queue Refactor**: Updated `sbgl_RenderQueuesEx` to use the transient GPU allocator and optimized sorter, resolving the CPU bottleneck in the voxel example.
- **Updated HAL Signatures**: Modified `sbgl_gfx_DrawIndirect` to accept a byte offset, supporting multi-draw indirect commands within shared transient buffers.

### Fixed
- **Voxel Example Performance**: Resolved the primary CPU bottleneck in `voxel_main`, reducing CPU frame time from ~11ms to <2ms for standard render distances.

## [Unreleased] - 2026-05-08

### Added
- **Bit-Field Optimization**: Reduced `sbgl_DrawPacket` from 24 bytes to 16 bytes by packing MeshID, MaterialID, and rendering flags into a 32-bit header. This increases CPU cache density by 50% during sorting and batching.
- **Packed Vertex Format**: Transitioned `sbgl_Vertex` to a 16-byte cache-aligned structure using `int16_t[4]` for positions (SNORM) and `uint32_t` for packed RGBA8 colors, reducing GPU vertex bandwidth by over 50%.
- **Transient Arena Management**: Introduced a frame-local `transientArena` in the engine context to handle merge-and-sort operations, eliminating heap allocations in the hot path.
- **Extended Submission API**: Updated `sbgl_SubmitDraw` to expose bit-packed rendering flags (`blendMode`, `sidedness`, `tags`) directly to the public API.
- **Real-time Telemetry**: Implemented a high-precision profiling system using Vulkan Timestamp Queries and platform timers to measure independent CPU and GPU execution times.
- **New Verification Test**: Added `packet_packing_test.c` to ensure structural alignment and bit-packing logic integrity.

### Fixed
- **Submission API Mismatches**: Corrected all demonstration examples and documentation to align with the updated `sbgl_SubmitDraw` signature.
- **Standalone Build Integrity**: Fixed `tests/CMakeLists.txt` to correctly handle engine test compilation when `SBGL_BUILD_STANDALONE` is enabled.
- **Internal State Verification**: Refactored `core_flags_test.c` to comply with strict compiler warnings and verified internal state machine transitions.

## [Unreleased] - 2026-05-04

### Added
- **Vulkan Backend**:
    - Implemented dynamic swapchain format selection via `vkGetPhysicalDeviceSurfaceFormatsKHR` to resolve hardcoded format validation errors.
    - Added support for `VK_FORMAT_B8G8R8A8_SRGB` and `VK_FORMAT_R8G8B8A8_SRGB` as preferred swapchain formats.
- **Rendering Pipeline**:
    - Implemented a rendering pipeline using Vulkan 1.3 **Dynamic Rendering**, eliminating RenderPass and Framebuffer management.
    - **Resource Management**: Introduced a handle-based system (`sbgl_Buffer`, `sbgl_Shader`, `sbgl_Pipeline`) for GPU resources using internal SoA pools for DOD-compliant performance.
    - **Shader System**: Added SPIR-V shader loading with support for both dynamic file-based loading and hardcoded byte arrays (via `xxd`).
    - **Explicit PSO**: Implemented Pipeline State Object (PSO) creation with configurable vertex layouts and shader stages.
    - **Interactive Rendering**: Added **Push Constants** support to the public API and Vulkan backend, enabling per-frame data updates (e.g., mouse position).
    - **Depth Buffering**: Implemented a dedicated depth attachment and enabled depth testing in the graphics pipeline to correct 3D geometry sorting.
    - **Synchronization Refactor**: Transitioned to a "Frames in Flight" model (2 overlapping frames) to resolve semaphore reuse validation errors and improve GPU utilization.
    - **Device Teardown**: Introduced `sbgl_DeviceWaitIdle()` to the public API to ensure the GPU is idle before destroying resources.
    - **New Examples**:
        - `examples/triangle/triangle_main.c`: Triangle rendering using vertex buffers and optional push-constant interaction.
        - `examples/camera/camera_main.c`: 3D example with a rotating pyramid, depth testing, and perspective projection.
    - **Documentation**: Created `docs/manual/rendering_pipeline.md` covering the architecture and usage workflows.
    - **Examples Restructure**: Reorganized `examples/` directory into topic-based subdirectories (`window`, `input`, `camera`, `triangle`, `batching`) and added `docs/examples/index.md` to catalog them.

### Fixed
- **Vulkan Backend**:
    - Resolved `vkDestroyPipeline` and `vkDestroyBuffer` validation errors (`VUID-vkDestroyPipeline-pipeline-00765`, `VUID-vkDestroyBuffer-buffer-00922`) in examples by adding explicit `sbgl_DeviceWaitIdle()` calls before resource destruction.
    - Enhanced API documentation in `sbgl.h` and `docs/manual/vulkan_backend.md` with critical warnings regarding GPU synchronization and teardown sequences.
    - Resolved `vkCreateSwapchainKHR` crashes (`floating point exception`) caused by zero-extent windows (minimized or unmapped) and unsupported image formats.
    - Improved swapchain image count selection logic to respect physical device limits.
    - Fixed validation errors related to `imageFormat` and `imageColorSpace` mismatches.
- **Build System**:
    - **Test Relocation**: Decoupled test applications from the `examples/` directory. Tests are now located in a top-level `tests/` directory.
    - **New Build Flag**: Introduced `SBGL_BUILD_TESTS` CMake option to toggle the compilation of internal tests independently of examples.
    - **Examples**: Refined `SBGL_BUILD_EXAMPLES` to strictly target demonstration applications.
    - Implemented a default `Debug` build configuration to ensure development environments have Vulkan validation layers and strict warnings enabled by default.
    - Added strict compiler flags (`-Werror`, `-Wmissing-prototypes`, `-Wstrict-prototypes` on GCC/Clang; `/WX` on MSVC) specifically for `Debug` builds to enforce coding standards.
    - Updated all example and test entry points to use `int main(void)` and ensured all internal functions use explicit `(void)` parameter lists to satisfy strict prototype requirements.
    - Fixed an issue where `.spv` shader files were deleted by the CMake build system after header conversion, preventing examples from loading them at runtime.
    - Improved shader conversion using `copy_if_different`.
    - Resolved unused variable warnings in test files to maintain a clean `-Werror` build.

- **Documentation System Refinement**:
    - Decoupled documentation generation from the source tree by relocating Doxygen output from `docs/out` to `build/docs`.
    - Updated `CMakeLists.txt` to dynamically configure Doxygen output paths, ensuring a cleaner root directory.
    - Image asset synchronization within the build-local documentation site.
- **Structural Cleanup**:
    - Moved the `shaders/` directory into `examples/shaders/` to reduce root directory clutter and better reflect its purpose as example-only assets.
    - Optimized the shader build pipeline to ensure generated assets stay within the build environment.

- **SIMD-Ready Math Library**:
    - Implemented a single-header math library (`sbgl_math.h`) providing Vector (Vec2, Vec3, Vec4), Matrix (Mat4), and Quaternion types.
    - Optimized memory layouts with 16-byte alignment and padding to facilitate efficient SIMD instruction generation and cache line utilization.
    - Implemented an Inverse Square Root (`sbgl_InvSqrt`) based on the Quake III Arena algorithm using C99-compliant union punning.
    - Provided a set of affine transformations: Translation, Scaling, Rotation (Axis-Angle), Perspective, Orthographic, and LookAt.
    - Added constructor functions (e.g., `sbgl_vec3()`) for natural initialization syntax.
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
    - Integrated synchronization to handle window minimization (0x0 dimensions).
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
    - Updated input helpers (`sbgl_IsKeyDown`, etc.) to perform validation checks and read directly from the context's internal state.

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
- **Public API**: Defined `sbgl.h` with an explicit Context management and physical scancode definitions.
- **Memory Management**: Integrated `sbl_arena.h` for zero-isolated-malloc window state allocation.
- **Opaque Types**: Created `sbgl_types.h` to centralize forward declarations and resolve C99 typedef redefinition warnings.
- **Input Refinement**: 
    - Decoupled the input system into a dedicated HAL (`sbgl_input.h`) and platform-specific implementations.
    - Replaced raw `memcpy` for keyboard states with **Struct Assignment**.
    - Added support for one-shot key triggers (`sbgl_IsKeyPressed`).
    - Implemented native pointer support (position, delta, buttons) for Wayland, X11, and Win32.
- **Git Integration**: Initialized git repository and added a `.gitignore`.
- **Documentation**: 
    - Integrated **Doxygen** for API documentation generation.
    - Added a `docs` target to CMake (`cmake --build build --target docs`).
    - Added `docs/manual/vulkan_backend.md` with a detailed explanation of the engine's graphics architecture.
- **Examples**: Added `hello_window.c` and `input_test.c` (interactive color switching) to verify the entire stack.

### Fixed
- **Compiler Warnings**:
    - Silenced ISO C99 pedantic warnings regarding anonymous structs in `sbgl_math.h` using the `__extension__` keyword.
    - Eliminated unused parameter warnings in Wayland platform callbacks by implementing explicit `(void)` casts, ensuring a clean build with `-Wall -Wextra -Wpedantic`.
- **Memory**:
 Resolved a segmentation fault in `sbl_arena_free` caused by a Use-After-Free when the arena structure was self-contained within its own memory blocks.
- **GPU Synchronization**: Fixed a Vulkan driver crash during rapid window resizing by implementing internal frame lifecycle tracking (`isDrawing` flag), preventing out-of-order command submissions.
- **Wayland Protocol Crashes**: Resolved "listener function is NULL" errors by providing callback implementations for `wl_keyboard` and `wl_pointer`.
- **Wayland Window Visibility**: Resolved the issue where windows remained invisible until a valid Vulkan buffer was attached and submitted.
- **Wayland ANR**: Fixed "Application Not Responding" dialogs by restoring the XDG-Shell ping/pong heartbeat.
- **GPU Synchronization**: Fixed a potential hang in the frame lifecycle by adopting an explicit `BeginDrawing`/`EndDrawing` pattern.
- **C99 Compatibility**: Fixed header redefinition errors by refactoring forward declarations into a dedicated types header.
