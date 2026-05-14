# SBgl Vulkan Backend Architecture

This document explains the Vulkan 1.3 implementation used in SBgl. The goal is to use the API directly with separation between the OS and the GPU.

---

## Instance Initialization

Everything starts with the `VkInstance`. This is the application's connection to the Vulkan runtime.

*   **Version:** **Vulkan 1.3** is required. This allows the use of features like *Dynamic Rendering* without needing abstractions like "Render Passes" or "Framebuffers."
*   **Extensions:** `VK_KHR_surface` and a platform-specific extension are enabled:
    *   **Wayland:** `VK_KHR_wayland_surface`
    *   **X11:** `VK_KHR_xlib_surface`
    *   **Windows:** `VK_KHR_win32_surface`

## Surface Integration

Vulkan is platform-agnostic; it does not know what a "window" is. The `VkSurfaceKHR` acts as the bridge.

*   Raw handles are retrieved from the **Platform HAL** (`sbgl_os_GetNativeWindowHandle`).
*   On Wayland, the `wl_display` and `wl_surface` are passed.
*   This tells the GPU driver which window should receive the rendered pixels.

## Physical Device Selection

The `select_physical_device()` function identifies the most suitable hardware:

*   **Enumeration:** All available GPUs (NVIDIA, AMD, Intel) are queried.
*   **Preference:** A `VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU` is searched for first.
*   **Queue Support:** The chosen GPU must have a "Queue Family" that supports **both** Graphics commands and Surface Presentation.

## Logical Device and Queue Management

Once a GPU is chosen, a `VkDevice` is created.

*   **Dynamic Rendering:** The `dynamicRendering` feature is enabled. This allows drawing directly to an image without the overhead of pre-defined RenderPass objects.
*   **Queues:** A single Graphics Queue is requested. This is the "lane" where drawing instructions are sent to the hardware.

## Debugging & Validation

To ensure API correctness, SBgl automatically enables Vulkan Validation Layers in non-release builds:

*   **Condition:** If the code is compiled without the `NDEBUG` macro (standard in Debug builds), the backend requests the `"VK_LAYER_KHRONOS_validation"` layer.
*   **Reporting:** Validation errors and warnings are printed directly to `stdout`/`stderr` by the Vulkan runtime.
*   **Performance:** Validation is disabled in Release builds to eliminate the CPU overhead associated with real-time API checking.

## Swapchain Infrastructure

To prevent flickering, drawing is never done directly to the screen. A **Swapchain** is used—a queue of images (usually 3 or 4).

*   **Acquisition:** The Swapchain is asked for an available image index.
*   **Synchronization:** `VkSemaphore` (GPU-to-GPU sync) and `VkFence` (CPU-to-GPU sync) are used to ensure the CPU does not start drawing before the GPU is finished with the previous frame.

## Frame Execution Pipeline

### Frame Initiation

*   **CPU Wait:** The `inFlightFence` is waited upon to ensure the previous frame is fully processed by the GPU.
*   **Image Grab:** The next image index is acquired from the swapchain.
*   **Command Recording:** The `VkCommandBuffer` is reset and a new list of operations is started.
*   **Layout Transition:** A `VkImageMemoryBarrier` is used to signal that the image is no longer for the OS and pixels will now be written to it.

### Dynamic Clearing

* `vkCmdBeginRendering` is used.
* A `clearValue` (RGBA) is provided.
* The GPU hardware clears the memory area of that specific swapchain image to the requested color.

### Frame Presentation

*   **Layout Transition:** The image is transitioned back to `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`. This signals that drawing is finished and the image can be returned to the OS.
*   **Submission:** The command buffer is closed and submitted to the Graphics Queue.
*   **Presentation:** `vkQueuePresentKHR` is called. This is the final step where the pixels are sent to the compositor to be displayed.

---

## Memory Management & Lifecycle

SBgl strictly avoids standard `malloc`/`free` calls and individual `vkCreateBuffer` operations during its runtime to ensure deterministic performance and prevent memory fragmentation.

### Hybrid GPU Memory Manager

The backend implements a Hybrid GPU Memory Manager that handles all buffer allocations through sub-allocation from pre-allocated physical memory pools.

* **Sub-allocation via `sbgl_gfx_CreateBuffer`**: Instead of calling `vkCreateBuffer` for every request, `sbgl_gfx_CreateBuffer` identifies the target heap (Static, Dynamic, or Managed) and returns a sub-allocated range (offset and size) within a master buffer.
* **Reduced Driver Pressure**: By managing memory internally, the system reduces the number of memory-map operations and descriptor updates required per frame.

### Managed Heap Range Tracking

For resources with non-linear lifecycles, the Managed Heap utilizes an internal array-based tracker to maintain visibility of active and free ranges.

* **Metadata Storage**: Range metadata is stored in a flat array, allowing the memory manager to perform rapid searches for free space without pointer indirection.
* **Fragmentation Control**: The manager prioritizes contiguous blocks and can merge adjacent free ranges, ensuring high density for intermediate-lifetime data like voxel chunks.

### Dynamic Swapchain Recreation (Mark/Rewind)

When a window is resized, the Vulkan swapchain and its associated image views must be destroyed and recreated. To manage this without `free()`:

*   **The Mark**: Before creating the swapchain arrays, the backend creates an `SblArenaMark` (a bookmark of the current arena offset).
*   **The Rewind**: During the `cleanup_swapchain` phase, the backend calls `sbl_arena_rewind()` to reset the arena to that bookmark.
*   **The Recreation**: The new, appropriately sized arrays are then pushed back onto the arena at the same location.

This pattern allows the engine to handle resize events without increasing the memory footprint or leaking memory.

---

## Context-Based Backend (Multi-Context Support)

The graphics backend is fully decoupled from global state. All Vulkan function pointers and device state are stored within an opaque `sbgl_GfxContext` structure.

- **Initialization:** `sbgl_gfx_Init` accepts a `sbgl_Window*` and an `SblArena*` for all its internal state allocations.
- **Reentrancy:** Multiple SBgl contexts can exist simultaneously, each with its own independent Vulkan logical device and swapchain.

### Configurable Resource Limits

Resource limits for buffers, shaders, and pipelines are configurable at context initialization time via `sbgl_InitWithConfig()`:

```c
sbgl_InitConfig config = {
    .windowWidth = 1920,
    .windowHeight = 1080,
    .windowTitle = "Application",
    .limits = {
        .maxBuffers = 4096,      // Maximum GPU buffer handles
        .maxShaders = 512,       // Maximum shader module handles
        .maxPipelines = 1024     // Maximum pipeline state objects
    },
    .enableValidation = true
};

sbgl_InitResult res = sbgl_InitWithConfig(&config);
```

Alternatively, use `sbgl_DefaultInitConfig` to start with defaults and override specific fields:

```c
sbgl_InitConfig config = sbgl_DefaultInitConfig;
config.limits.maxBuffers = 4096;
config.limits.maxPipelines = 1024;

sbgl_InitResult res = sbgl_InitWithConfig(&config);
```

**Default Limits:**

| Resource | Default | Minimum | Memory Impact |
|----------|---------|---------|---------------|
| Buffers | 1024 | 64 | ~32KB per 1024 entries |
| Shaders | 256 | 16 | ~4KB per 256 entries |
| Pipelines | 256 | 16 | ~8KB per 256 entries |

Limits below the minimum are automatically clamped. Resource arrays are allocated from the persistent arena during initialization and remain fixed for the context lifetime to ensure O(1) handle lookups.

---

## GPU Synchronization & Teardown

> [!WARNING]
> **GPU Synchronization is Mandatory:** In Vulkan, the CPU and GPU operate asynchronously. It is illegal to destroy a resource (Pipeline, Buffer, Shader) that is currently being used by the GPU or is referenced by a command buffer that has been submitted but not yet finished execution.

### Safe Teardown Pattern

Before destroying resources during application shutdown or major state transitions, the CPU MUST wait for the GPU to become idle.

```c
// Correct shutdown sequence
while (!sbgl_WindowShouldClose(ctx)) {
    // ... main loop ...
}

// Wait for GPU to finish all work
sbgl_DeviceWaitIdle(ctx);

// Safely destroy resources
sbgl_DestroyPipeline(ctx, pipeline);
sbgl_DestroyBuffer(ctx, vbo);

// Shutdown engine
sbgl_Shutdown(ctx);
```

### Validation Errors

Failure to call `sbgl_DeviceWaitIdle()` before destruction will trigger Vulkan Validation Errors such as:

- `VUID-vkDestroyPipeline-pipeline-00765`: Pipeline currently in use.
- `VUID-vkDestroyBuffer-buffer-00922`: Buffer currently in use.

SBgl maintains this synchronization as explicit. This prevents unnecessary stalls during execution while ensuring full API correctness during cleanup.

## Buffer Device Address (BDA)

SBgl utilizes the `VK_KHR_buffer_device_address` extension (core in Vulkan 1.3) to achieve efficient memory access.

- **Pointer-Like Access:** BDA allows the GPU to access buffer memory via 64-bit GPU virtual addresses rather than traditional descriptor sets.
- **Reduced Binding Overhead:** By passing these addresses directly (e.g., via Push Constants), the engine eliminates the need to frequently update and rebind descriptor sets for every draw call.
- **DOD Alignment:** This enables more efficient data-oriented structures on the GPU, such as linked lists or trees stored in a single large buffer and traversed using raw addresses.

## Indirect Drawing

To maximize the "Where there is one, there are many" mandate, SBgl employs Indirect Drawing for its automated batching system.

- **CPU Submission:** The CPU records a single `vkCmdDrawIndirect` (or `vkCmdDrawIndexedIndirect`) command.
- **GPU Execution:** The actual draw parameters (vertex count, instance count, first vertex, etc.) are read by the GPU from a buffer at execution time.
- **Batch Processing:** This allows thousands of draw calls to be submitted with minimal CPU overhead, as the CPU only needs to update a single "draw command buffer" rather than recording individual commands for every object.

## Function Loading (volk)

The backend utilizes **volk** as its Vulkan meta-loader to eliminate static linking and provide device-level function dispatching.

- **Boilerplate Reduction:** `volk` automates the retrieval of function pointers, replacing manual `vkGetInstanceProcAddr` and `vkGetDeviceProcAddr` calls.
- **Context-Based Dispatching:** The system employs `VolkDeviceTable` to load device-specific function pointers. This enables multiple graphics contexts to operate with minimal overhead by avoiding the Vulkan loader's internal dispatching logic for high-frequency commands.

---

## Technical Summary

| Feature | Implementation | Benefit |
| :--- | :--- | :--- |
| **Vulkan API** | 1.3 Core | Access to simplified pipelines. |
| **Meta-loader** | volk (v1.4.350) | Device-level function dispatching via device tables. |
| **Rendering** | Dynamic Rendering | No RenderPass/Framebuffer boilerplate. |
| **Sync** | Fences & Semaphores | Synchronization and stable frame rates. |
| **GPU Memory** | Hybrid Sub-allocation | Reduced API overhead and fragmentation via heap partitioning. |
| **Arch** | Context-Based | No global singletons; supports multi-window. |

---

## References & Further Reading

For detailed technical specifications of the Vulkan API and its implementations, refer to the following official documentation:

- **Vulkan 1.3 Specification:** [The Khronos Group - Vulkan Spec](https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html)
- **Vulkan Guide:** [Khronos Vulkan Guide](https://github.com/KhronosGroup/Vulkan-Guide)
- **Dynamic Rendering:** [Vulkan Spec - Dynamic Rendering](https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#renderpass-and-framebuffer)
- **Synchronization Examples:** [Khronos - Synchronization Examples](https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples)
