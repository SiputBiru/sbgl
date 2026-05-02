# SBgl Vulkan Backend Architecture

This document explains the "bare-metal" Vulkan 1.3 implementation used in SBgl. Our goal is high performance, zero legacy boilerplate, and strict separation between the OS and the GPU.

---

## 1. The Global Connection (The Instance)
Everything starts with the `VkInstance`. This is our application's connection to the Vulkan runtime.
- **Version:** We strictly require **Vulkan 1.3**. This allows us to use modern features like *Dynamic Rendering* without needing older, complex abstractions like "Render Passes" or "Framebuffers."
- **Extensions:** We enable `VK_KHR_surface` and a platform-specific extension:
    - **Wayland:** `VK_KHR_wayland_surface`
    - **X11:** `VK_KHR_xlib_surface`
    - **Windows:** `VK_KHR_win32_surface`

## 2. Bridging OS & GPU (The Surface)
Vulkan is platform-agnostic; it doesn't know what a "window" is. The `VkSurfaceKHR` acts as the bridge.
- We retrieve raw handles from the **Platform HAL** (`sbgl_os_GetNativeWindowHandle`).
- On Wayland, we pass the `wl_display` and `wl_surface`.
- This "handshake" tells the GPU driver exactly which window on your desktop should receive the rendered pixels.

## 3. Selecting the Brain (Physical Device)
The `select_physical_device()` function acts as an automated scout:
1. **Enumeration:** It queries all available GPUs (NVIDIA, AMD, Intel).
2. **Preference:** It searches for a `VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU` first for maximum performance.
3. **Queue Support:** It ensures the chosen GPU has a "Queue Family" that supports **both** Graphics commands and Surface Presentation. This is mandatory for drawing to a window.

## 4. The Work Horse (Logical Device & Queues)
Once a GPU is chosen, we create a `VkDevice`. 
- **Dynamic Rendering:** We enable the `dynamicRendering` feature. This is a Vulkan 1.3 milestone that allows us to draw directly to an image without the overhead of pre-defined RenderPass objects.
- **Queues:** We request a single Graphics Queue. This is the "lane" where we send our drawing instructions to the hardware.

## 5. The Image Factory (The Swapchain)
To prevent flickering, we never draw directly to the screen. We use a **Swapchain**—a queue of images (usually 3 or 4).
- **Acquisition:** We ask the Swapchain for an available image index.
- **Synchronization:** We use `VkSemaphore` (GPU-to-GPU sync) and `VkFence` (CPU-to-GPU sync) to ensure the CPU doesn't start drawing before the GPU is finished with the previous frame.

## 6. The Execution Pipeline (Begin/End Drawing)

### Phase A: `sbgl_gfx_BeginFrame`
1. **CPU Wait:** We wait for the `inFlightFence` to ensure the previous frame is fully processed by the GPU.
2. **Image Grab:** We acquire the next image index from the swapchain.
3. **Command Recording:** We reset the `VkCommandBuffer` and start a new "to-do list."
4. **Layout Transition:** We use a `VkImageMemoryBarrier` to tell the GPU: "This image is no longer for the OS; I am going to write pixels to it now."

### Phase B: `sbgl_Clear` (Dynamic Rendering)
- We use `vkCmdBeginRendering`. 
- We provide a `clearValue` (RGBA).
- The GPU hardware clears the entire memory area of that specific swapchain image to the requested color.

### Phase C: `sbgl_gfx_EndFrame`
1. **Layout Transition:** We transition the image back to `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`. This tells the GPU: "I'm done drawing; give this back to the OS."
2. **Submission:** We "close" the command buffer and submit it to the Graphics Queue.
3. **Presentation:** We call `vkQueuePresentKHR`. This is the final step where the pixels are sent to the Wayland/Windows compositor to be displayed on your monitor.

---

## Technical Summary
| Feature | Implementation | Benefit |
| :--- | :--- | :--- |
| **Vulkan API** | 1.3 Core | Access to modern, simplified pipelines. |
| **Rendering** | Dynamic Rendering | No RenderPass/Framebuffer boilerplate. |
| **Sync** | Fences & Semaphores | Zero screen tearing and stable frame rates. |
| **Memory** | Arena-backed | No runtime `malloc` during window/gfx init. |
