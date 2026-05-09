# Memory Management Architecture

SBgl utilizes a deterministic, arena-based memory architecture designed to eliminate runtime fragmentation and provide allocation with unified lifecycles.

---

## Memory Visualization

The following diagrams illustrate the core hierarchy and ownership.

### Core Hierarchy and Ownership

The public context shell encapsulates the internal state and the physical memory controller.
<img src="memory_ownership_flow.jpeg" width="600" alt="SBgl Ownership Flow" />

### Memory Block Layout

The linear allocation pattern and the mark/rewind recycling mechanic are used during runtime events like window resizing.
<img src="memory_block_layout.jpeg" width="600" alt="SBgl Memory Block Layout" />

---

## The Arena Allocator (SblArena)

The core of the memory system is the linear arena allocator defined in `sbl_arena.h`. This allocator manages contiguous blocks of memory and satisfies allocation requests by simply incrementing an offset pointer.

- **Complexity**: O(1) for both allocation and deallocation.
- **Safety**: Individual elements are never freed; instead, the entire arena is released at once, preventing dangling pointers and leaks.
- **Zero-Initialization**: The `SBL_ARENA_PUSH_STRUCT_ZERO` macro ensures that all allocated state starts in a known, valid zero-state.

## Context-Encapsulated Lifecycles

The `sbgl_Context` holds the primary `SblArena` for the entire engine instance. When `sbgl_Init` is called, the arena is initialized first, and then the context shell and all internal platform structures (like the window state) are allocated directly from it.

### The Transient Arena

While the primary arena manages the context lifecycle, SBgl utilizes a **Transient Arena** for data that only needs to exist for the duration of a single frame (e.g., sort keys, merged draw packets, temporary math buffers). This arena is reset at the beginning of every frame processing cycle, providing zero-latency "scratch" memory without the overhead of heap fragmentation.

### Arena Hierarchy (ASCII)

```text
[ Context Memory Block ]
|
|-- [ Persistent Pool ] (Allocated once at Init)
|   |-- Window State
|   |-- Graphics Backend State
|   |-- Render Queues (Fixed Capacity)
|
|-- [ Transient Pool ] (Reset every frame)
|   |-- Merged Draw Packets
|   |-- Radix Sort Key/Index Buffers
|   |-- Temporary Transformation Workspace
```

## Transient GPU Buffers

For high-frequency GPU data that changes every frame (such as instance transforms and indirect draw commands), SBgl avoids standard buffer creation and destruction. Instead, it utilizes a **GPU Transient Buffer** system.

### Ring-Buffer Sub-allocation

The Vulkan backend pre-allocates a large, persistently mapped buffer (default 16MB) for each frame in flight.

- **Allocation**: The `sbgl_gfx_AllocateTransient` function sub-allocates from the current frame's buffer using a linear offset.
- **Reset**: At the start of each frame, the offset for that frame's buffer is reset to zero.
- **Safety**: Because each frame in flight has its own dedicated buffer, the CPU can write new data for the next frame while the GPU is still reading data for the previous frame.

### CPU-GPU Interaction (ASCII)

```text
[ Frame 0 Buffer ] <--- GPU Reading (Render Loop)
[ Frame 1 Buffer ] <--- CPU Writing (Submission Loop)
      ^
      |-- Offset 0: Instances [ Chunk A ]
      |-- Offset N: Indirect Commands [ Draw 1 ]
      |-- Offset M: Instances [ Chunk B ]
```

### Ownership Transfer

The `sbgl_InternalContext` assumes ownership of the arena. This creates a single point of failure and success: calling `sbl_arena_free()` during shutdown automatically releases every byte of memory used by the core, platform, and graphics layers.

## Dynamic Resource Management (Mark/Rewind)

While arenas are typically linear, SBgl implements a **Mark and Rewind** pattern to handle dynamic resources that must be recreated, such as the Vulkan Swapchain.

- **The Mark**: A bookmark (`SblArenaMark`) is taken before a set of dynamic allocations.
- **The Rewind**: When the resources need to be updated (e.g., window resize), the arena is rewound to the bookmark.
- **The Recycle**: New resources are pushed onto the exact same memory location, keeping the memory footprint constant regardless of how many times the window is resized.

---

## Technical Summary

| Feature | Implementation | Benefit |
| :--- | :--- | :--- |
| **Primary Allocator** | Linear Arena (`SblArena`) | Zero fragmentation, O(1) allocation. |
| **Safety Pattern** | Mark/Rewind | Constant memory footprint for dynamic arrays. |
| **Initialization** | Push-Struct-Zero | Deterministic starting state without `memset`. |
| **Cleanup** | Unified Arena Free | Single-call total resource reclamation. |
