# SBgl Roadmap

SBgl (SiputBiru Graphics Library) is a bare-metal graphics framework in C99. This roadmap outlines the development milestones and future technical goals.

## Status: Phase 4 Complete

### Phase 1: Core Architecture and HAL
- [x] Implement explicit Context API (sbgl_Context)
- [x] Standardize error handling with result structures (sbgl_InitResult)
- [x] Define Internal Platform HAL for OS-agnostic windowing (sbgl_platform.h)
- [x] Define Internal Input HAL for decoupled physical input (sbgl_input.h)
- [x] Integrate arena allocator for zero-isolated-malloc allocation (sbl_arena.h)
- [x] Centralize shared opaque types to prevent C99 redefinition conflicts (sbgl_types.h)

### Phase 2: Multi-Platform Support and Graphics Foundation
- [x] Implement native Linux Wayland platform layer using XDG-Shell protocols
- [x] Implement Linux X11 platform layer for legacy display server support
- [x] Implement Win32 platform layer with full virtual key mapping
- [x] Implement Dynamic Vulkan Loading (no build-time link dependency)
- [x] Initialize Vulkan 1.3 with Dynamic Rendering enabled (no RenderPass boilerplate)
- [x] Implement automatic Swapchain recreation and window resizing support
- [x] Stabilization of Arena allocator for self-contained context management
- [x] Implement Doxygen documentation system
- [x] Implement CMake build system with Unity-style build support
- [x] Implement Data-Oriented Design (DOD) Input API for batch processing

### Phase 3: Core Matrix and Math
- [x] Implement SIMD-ready Vector math (Vec2, Vec3, Vec4) using SoA-friendly layouts
- [x] Implement Matrix math (Mat4) for 3D transformations
- [x] Implement Quaternion math for rotations
- [x] Implement Camera system (Orthographic and Perspective)
- [x] Implement Ray-casting and basic collision math optimized for batch testing

### Phase 4: Rendering Pipeline
- [x] Implement Vertex Buffer and Index Buffer management with DOD alignment
- [x] Implement Shader loading and SPIR-V integration
- [x] Implement Pipeline State Object (PSO) caching
- [x] Implement Batch-oriented Geometry rendering (Transformation pipelines)

### Phase 5: Optimization
- [ ] Implement Bit-Packing for vertex data (32-bit compressed formats)
- [ ] Implement Procedural Vertex Generation using `gl_VertexIndex`
- [ ] Implement Shader Storage Buffer Objects (SSBOs) for per-instance data
- [ ] Implement Multi-Draw Indirect (MDI) for single-call batching
- [ ] Implement Greedy Meshing algorithms for voxel/grid data

### Phase 6: Batch Rendering
- [ ] Implement 2D Sprite batching system (utilizing MDI)
- [ ] Implement Text rendering using signed-distance fields (SDF)
- [ ] Implement Primitive batching (Circles, Lines, Gradients)
- [ ] Implement Render-to-texture support
- [ ] Implement Post-processing pipeline

### Phase 7: Parallelism & Concurrency
- [ ] Implement Worker Thread Pool and Job Scheduler
- [ ] Implement Dependency Graph for task synchronization
- [ ] Implement Thread-Local Arena management
- [ ] Implement Parallel Vulkan Command Buffer recording
- [ ] Implement Data-Parallel array transformation pipelines

## Future Considerations
- macOS Platform Layer (Cocoa) and MoltenVK integration
- Android/NDK Platform support
- WebAssembly (Wasm) backend via WebGPU
- Audio HAL and Backend
- Advanced Multi-threaded command recording
