# SBgl Roadmap

SBgl is a bare-metal graphics framework in C99. This roadmap outlines the development milestones and technical goals.

## HAL & Context Foundation

- [x] Implement explicit Context API (`sbgl_Context`)
- [x] Standardize error handling with result structures (`sbgl_InitResult`)
- [x] Define Internal Platform HAL for OS-agnostic windowing (`sbgl_platform.h`)
- [x] Define Internal Input HAL for decoupled physical input (`sbgl_input.h`)
- [x] Integrate arena allocator for zero-isolated-malloc allocation (`sbl_arena.h`)
- [x] Centralize shared opaque types to prevent C99 redefinition conflicts (`sbgl_types.h`)

## Native Wayland & Vulkan Clear

- [x] Implement native Linux Wayland platform layer using XDG-Shell protocols
- [x] Implement Linux X11 platform layer for legacy display server support
- [x] Implement Win32 platform layer with full virtual key mapping
- [x] Implement Dynamic Vulkan Loading (no build-time link dependency)
- [x] Initialize Vulkan 1.3 with Dynamic Rendering enabled (no RenderPass boilerplate)
- [x] Implement automatic Swapchain recreation and window resizing support
- [x] Stabilization of Arena allocator for self-contained context management
- [x] Implement Data-Oriented Design (DOD) Input API for batch processing

## Core Matrix & Math (simd-ready)

- [x] Implement SIMD-ready Vector math (Vec2, Vec3, Vec4) using SoA-friendly layouts
- [x] Implement Matrix math (Mat4) for 3D transformations
- [x] Implement Quaternion math for rotations
- [x] Implement technical architecture guide (`docs/manual/math_library.md`)

## Camera System & Batch Collision

- [x] Implement Camera system (Orthographic and Perspective)
- [x] Implement Ray-casting and basic collision math optimized for batch testing

## Vertex Buffers & Triangle Rendering

- [x] Implement Vertex Buffer and Index Buffer management with DOD alignment
- [x] Implement Shader loading and SPIR-V integration
- [x] Implement Batch-oriented Geometry rendering (Transformation pipelines)

## Context-Based Backend Refactor (Multi-Context Support)

- [x] Transition graphics backend to use context-local state pointers
- [x] Enable multi-window and multi-threaded execution environments

## API Standardization & Error Handling

- [x] Implement `sbgl_Result` status tracking in the context
- [x] Standardize naming conventions across the public API surface
- [x] Implement configurable resource limits via `sbgl_InitWithConfig()`
- [x] Implement split error architecture (core `sbgl_Result` + backend VkResult detail)
- [x] Implement `sbgl_GetResult()`, `sbgl_GetErrorDetail()` for error inspection
- [x] Implement categorized logging system with file output and rotation
- [x] Implement platform layer error propagation via `sbgl_platform_Result`
- [x] Implement validation layer integration with debug callback routing

## GPU Optimization & Batching (MDI/Bit-packing)

- [x] Implement Bit-Packing for vertex data (32-bit compressed formats)
- [x] Implement 16-byte Command Packet Header (Mesh/Material/Flag packing)
- [x] Implement Procedural Vertex Generation using `gl_VertexIndex`
- [x] Implement Shader Storage Buffer Objects (SSBOs) for per-instance data
- [x] Implement Multi-Draw Indirect (MDI) for single-call batching
- [x] Implement Persistent Transient GPU Buffers (eliminated per-frame allocations)
- [x] Optimize Radix Sort for 8-bit passes and hot-path zero-allocation
- [ ] Implement Greedy Meshing algorithms (Replaced by GPU Procedural Synthesis)
- [x] Implement Infinite Voxel World (2.5D Chunked Instancing)

- [ ] Granular Profiling (per-queue timestamps)

## Multi-Threaded Architecture

### Thread-Local Memory

- [ ] Implement thread-local `SblArena` per worker thread to eliminate allocator contention
- [ ] Add per-frame ring buffers for transient allocations with thread-safe recycling
- [ ] Implement thread-safe memory allocation interface for shared GPU resources

### Parallel Command Recording

- [ ] Implement `VkCommandBuffer` Secondary Buffers for parallel recording across threads
- [ ] Add lock-free render queue submission with thread-safe packet merging
- [ ] Implement per-thread command recording contexts with automatic synchronization
- [ ] Add synchronization barriers between recording and submission phases

### Job System Foundation

- [ ] Implement data-parallel job dispatch system with work-stealing queues
- [ ] Add dependency graph-based job scheduling (lock-free, no mutex in hot path)
- [ ] Implement frame-to-frame job handoff without global synchronization points
- [ ] Add job profiling and bottleneck analysis tooling

### Multi-Context & Multi-Window

- [ ] Enable true multi-window with independent rendering threads per window
- [ ] Add thread-safe context sharing for cross-window resource management
- [ ] Implement thread-local input state processing with consolidated event dispatch
- [ ] Add GPU fence-based synchronization for cross-thread resource lifetime

### Synchronization Strategy

- [ ] Implement fine-grained locking for shared data structures
- [ ] Add lock-free data structures for high-frequency subsystems (render queues, input)
- [ ] Implement multi-fence GPU synchronization for parallel frame processing
- [ ] Add pipeline stage profiling to identify synchronization bottlenecks

## High-Level rendering (Text/Sprites)

- [ ] Implement 2D Sprite batching system (utilizing MDI)
- [ ] Implement Text rendering using signed-distance fields (SDF)
- [ ] Implement Primitive batching (Circles, Lines, Gradients)
- [ ] Implement Render-to-texture support
- [ ] Implement Post-processing pipeline

## Future Considerations

- Full 3D Voxel Grid rendering (Caves, Overhangs)
- macOS Platform Layer (Cocoa) and MoltenVK integration
- Android/NDK Platform support
- WebAssembly (Wasm) backend via WebGPU
- Audio HAL and Backend
