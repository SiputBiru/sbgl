# SBgl Vulkan Backend Architecture

This document explains the Vulkan 1.3 implementation used in SBgl. The goal is to use the API directly with separation between the OS and the GPU.

---

## 1. The Global Connection (The Instance)
Everything starts with the `VkInstance`. This is the application's connection to the Vulkan runtime.
- **Version:** **Vulkan 1.3** is required. This allows the use of features like *Dynamic Rendering* without needing abstractions like "Render Passes" or "Framebuffers."
- **Extensions:** `VK_KHR_surface` and a platform-specific extension are enabled:
    - **Wayland:** `VK_KHR_wayland_surface`
    - **X11:** `VK_KHR_xlib_surface`
    - **Windows:** `VK_KHR_win32_surface`

## 2. Bridging OS & GPU (The Surface)
Vulkan is platform-agnostic; it does not know what a "window" is. The `VkSurfaceKHR` acts as the bridge.
- Raw handles are retrieved from the **Platform HAL** (`sbgl_os_GetNativeWindowHandle`).
- On Wayland, the `wl_display` and `wl_surface` are passed.
- This tells the GPU driver which window should receive the rendered pixels.

## 3. Selecting the Brain (Physical Device)
The `select_physical_device()` function acts as an automated scout:
1. **Enumeration:** It queries all available GPUs (NVIDIA, AMD, Intel).
2. **Preference:** It searches for a `VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU` first.
3. **Queue Support:** It ensures the chosen GPU has a "Queue Family" that supports **both** Graphics commands and Surface Presentation.

## 4. The Work Horse (Logical Device & Queues)
Once a GPU is chosen, a `VkDevice` is created. 
- **Dynamic Rendering:** The `dynamicRendering` feature is enabled. This allows drawing directly to an image without the overhead of pre-defined RenderPass objects.
- **Queues:** A single Graphics Queue is requested. This is the "lane" where drawing instructions are sent to the hardware.

## 5. The Image Factory (The Swapchain)
To prevent flickering, drawing is never done directly to the screen. A **Swapchain** is used—a queue of images (usually 3 or 4).
- **Acquisition:** The Swapchain is asked for an available image index.
- **Synchronization:** `VkSemaphore` (GPU-to-GPU sync) and `VkFence` (CPU-to-GPU sync) are used to ensure the CPU does not start drawing before the GPU is finished with the previous frame.

## 6. Execution Pipeline (Begin/End Drawing)

### Phase A: `sbgl_gfx_BeginFrame`
1. **CPU Wait:** The `inFlightFence` is waited upon to ensure the previous frame is fully processed by the GPU.
2. **Image Grab:** The next image index is acquired from the swapchain.
3. **Command Recording:** The `VkCommandBuffer` is reset and a new "to-do list" is started.
4. **Layout Transition:** A `VkImageMemoryBarrier` is used to tell the GPU: "This image is no longer for the OS; pixels will now be written to it."

### Phase B: `sbgl_Clear` (Dynamic Rendering)
- `vkCmdBeginRendering` is used. 
- A `clearValue` (RGBA) is provided.
- The GPU hardware clears the memory area of that specific swapchain image to the requested color.

### Phase C: `sbgl_gfx_EndFrame`
1. **Layout Transition:** The image is transitioned back to `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`. This tells the GPU: "Drawing is finished; give this back to the OS."
2. **Submission:** The command buffer is closed and submitted to the Graphics Queue.
3. **Presentation:** `vkQueuePresentKHR` is called. This is the final step where the pixels are sent to the compositor to be displayed.

---

## Technical Summary
| Feature | Implementation | Benefit |
| :--- | :--- | :--- |
| **Vulkan API** | 1.3 Core | Access to simplified pipelines. |
| **Rendering** | Dynamic Rendering | No RenderPass/Framebuffer boilerplate. |
| **Sync** | Fences & Semaphores | Synchronization and stable frame rates. |
| **Memory** | Arena-backed | No runtime `malloc` during window/gfx init. |
