#include "sbgl_graphics_hal.h"
#include "core/sbgl_platform.h"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#ifdef SBGL_PLATFORM_WAYLAND
#include <vulkan/vulkan_wayland.h>
#include <dlfcn.h>
#define SBGL_VK_LIB "libvulkan.so.1"
#elif defined(SBGL_PLATFORM_X11)
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#include <dlfcn.h>
#define SBGL_VK_LIB "libvulkan.so.1"
#elif defined(_WIN32)
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#define SBGL_VK_LIB "vulkan-1.dll"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Vulkan Function Pointers ---

// 1. Global Functions (can be loaded with NULL instance)
static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = NULL;
static PFN_vkCreateInstance vkCreateInstance = NULL;

// 2. Instance Functions (require valid instance)
static PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = NULL;
static PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = NULL;
static PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = NULL;
static PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = NULL;
static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = NULL;
static PFN_vkCreateDevice vkCreateDevice = NULL;
static PFN_vkDestroyInstance vkDestroyInstance = NULL;
static PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = NULL;
static PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = NULL;

#ifdef SBGL_PLATFORM_WAYLAND
static PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR = NULL;
#elif defined(SBGL_PLATFORM_X11)
static PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR = NULL;
#elif defined(_WIN32)
static PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = NULL;
#endif

// 3. Device Functions (require valid device)
static PFN_vkGetDeviceQueue vkGetDeviceQueue = NULL;
static PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = NULL;
static PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = NULL;
static PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = NULL;
static PFN_vkCreateImageView vkCreateImageView = NULL;
static PFN_vkDestroyImageView vkDestroyImageView = NULL;
static PFN_vkCreateCommandPool vkCreateCommandPool = NULL;
static PFN_vkDestroyCommandPool vkDestroyCommandPool = NULL;
static PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = NULL;
static PFN_vkCreateSemaphore vkCreateSemaphore = NULL;
static PFN_vkDestroySemaphore vkDestroySemaphore = NULL;
static PFN_vkCreateFence vkCreateFence = NULL;
static PFN_vkDestroyFence vkDestroyFence = NULL;
static PFN_vkWaitForFences vkWaitForFences = NULL;
static PFN_vkResetFences vkResetFences = NULL;
static PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = NULL;
static PFN_vkBeginCommandBuffer vkBeginCommandBuffer = NULL;
static PFN_vkEndCommandBuffer vkEndCommandBuffer = NULL;
static PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = NULL;
static PFN_vkCmdBeginRendering vkCmdBeginRendering = NULL;
static PFN_vkCmdEndRendering vkCmdEndRendering = NULL;
static PFN_vkResetCommandBuffer vkResetCommandBuffer = NULL;
static PFN_vkQueueSubmit vkQueueSubmit = NULL;
static PFN_vkQueuePresentKHR vkQueuePresentKHR = NULL;
static PFN_vkDestroyDevice vkDestroyDevice = NULL;
static PFN_vkDeviceWaitIdle vkDeviceWaitIdle = NULL;

typedef struct {
    void*            libHandle;
    VkInstance       instance;
    VkSurfaceKHR     surface;
    VkPhysicalDevice physicalDevice;
    VkDevice         device;
    VkQueue          graphicsQueue;
    uint32_t         graphicsQueueFamily;
    
    VkSwapchainKHR   swapchain;
    VkFormat         swapchainFormat;
    VkExtent2D       swapchainExtent;
    uint32_t         imageCount;
    VkImage*         images;
    VkImageView*     imageViews;

    VkCommandPool    commandPool;
    VkCommandBuffer  commandBuffer;
    VkSemaphore      imageAvailableSemaphore;
    VkSemaphore      renderFinishedSemaphore;
    VkFence          inFlightFence;

    uint32_t         currentImageIndex;
} SBGL_VulkanContext;

static SBGL_VulkanContext g_vk = {0};

static bool load_vulkan_library(void) {
#ifdef _WIN32
    g_vk.libHandle = LoadLibraryA(SBGL_VK_LIB);
#else
    g_vk.libHandle = dlopen(SBGL_VK_LIB, RTLD_NOW);
#endif

    if (!g_vk.libHandle) {
        fprintf(stderr, "[Vulkan] Failed to load %s\n", SBGL_VK_LIB);
        return false;
    }

#ifdef _WIN32
    void* addr = GetProcAddress((HMODULE)g_vk.libHandle, "vkGetInstanceProcAddr");
    memcpy(&vkGetInstanceProcAddr, &addr, sizeof(addr));
#else
    void* sym = dlsym(g_vk.libHandle, "vkGetInstanceProcAddr");
    memcpy(&vkGetInstanceProcAddr, &sym, sizeof(sym));
#endif

    if (!vkGetInstanceProcAddr) return false;

    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (!vkCreateInstance) return false;

    return true;
}

static bool load_instance_functions(void) {
    #define LOAD_INST(name) \
        name = (PFN_##name)vkGetInstanceProcAddr(g_vk.instance, #name); \
        if (!name) { fprintf(stderr, "[Vulkan] Failed to load instance function: %s\n", #name); return false; }

    LOAD_INST(vkGetDeviceProcAddr);
    LOAD_INST(vkDestroyInstance);
    LOAD_INST(vkDestroySurfaceKHR);
    LOAD_INST(vkEnumeratePhysicalDevices);
    LOAD_INST(vkGetPhysicalDeviceProperties);
    LOAD_INST(vkGetPhysicalDeviceQueueFamilyProperties);
    LOAD_INST(vkGetPhysicalDeviceSurfaceSupportKHR);
    LOAD_INST(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    LOAD_INST(vkCreateDevice);

#ifdef SBGL_PLATFORM_WAYLAND
    LOAD_INST(vkCreateWaylandSurfaceKHR);
#elif defined(SBGL_PLATFORM_X11)
    LOAD_INST(vkCreateXlibSurfaceKHR);
#elif defined(_WIN32)
    LOAD_INST(vkCreateWin32SurfaceKHR);
#endif

    #undef LOAD_INST
    return true;
}

static bool load_device_functions(void) {
    #define LOAD_DEV(name) \
        name = (PFN_##name)vkGetDeviceProcAddr(g_vk.device, #name); \
        if (!name) { fprintf(stderr, "[Vulkan] Failed to load device function: %s\n", #name); return false; }

    LOAD_DEV(vkGetDeviceQueue);
    LOAD_DEV(vkCreateSwapchainKHR);
    LOAD_DEV(vkDestroySwapchainKHR);
    LOAD_DEV(vkGetSwapchainImagesKHR);
    LOAD_DEV(vkCreateImageView);
    LOAD_DEV(vkDestroyImageView);
    LOAD_DEV(vkCreateCommandPool);
    LOAD_DEV(vkDestroyCommandPool);
    LOAD_DEV(vkAllocateCommandBuffers);
    LOAD_DEV(vkCreateSemaphore);
    LOAD_DEV(vkDestroySemaphore);
    LOAD_DEV(vkCreateFence);
    LOAD_DEV(vkDestroyFence);
    LOAD_DEV(vkWaitForFences);
    LOAD_DEV(vkResetFences);
    LOAD_DEV(vkAcquireNextImageKHR);
    LOAD_DEV(vkBeginCommandBuffer);
    LOAD_DEV(vkEndCommandBuffer);
    LOAD_DEV(vkCmdPipelineBarrier);
    LOAD_DEV(vkCmdBeginRendering);
    LOAD_DEV(vkCmdEndRendering);
    LOAD_DEV(vkResetCommandBuffer);
    LOAD_DEV(vkQueueSubmit);
    LOAD_DEV(vkQueuePresentKHR);
    LOAD_DEV(vkDestroyDevice);
    LOAD_DEV(vkDeviceWaitIdle);

    #undef LOAD_DEV
    return true;
}

static bool create_instance(void) {
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "SBgl Application",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "SBgl",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef SBGL_PLATFORM_WAYLAND
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#elif defined(SBGL_PLATFORM_X11)
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#elif defined(_WIN32)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]),
        .ppEnabledExtensionNames = extensions,
    };

    if (vkCreateInstance(&createInfo, NULL, &g_vk.instance) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create instance\n");
        return false;
    }

    if (!load_instance_functions()) return false;
    printf("[Vulkan] Instance created successfully (v1.3)\n");
    return true;
}

static bool create_surface(sbgl_Window* window) {
#ifdef SBGL_PLATFORM_WAYLAND
    VkWaylandSurfaceCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = (struct wl_display*)sbgl_os_GetNativeDisplayHandle(),
        .surface = (struct wl_surface*)sbgl_os_GetNativeWindowHandle(window),
    };
    if (vkCreateWaylandSurfaceKHR(g_vk.instance, &createInfo, NULL, &g_vk.surface) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create Wayland surface\n");
        return false;
    }
#elif defined(SBGL_PLATFORM_X11)
    VkXlibSurfaceCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .dpy = (Display*)sbgl_os_GetNativeDisplayHandle(),
        .window = (Window)(uintptr_t)sbgl_os_GetNativeWindowHandle(window),
    };
    if (vkCreateXlibSurfaceKHR(g_vk.instance, &createInfo, NULL, &g_vk.surface) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create Xlib surface\n");
        return false;
    }
#elif defined(_WIN32)
    VkWin32SurfaceCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = (HINSTANCE)sbgl_os_GetNativeInstanceHandle(),
        .hwnd = (HWND)sbgl_os_GetNativeWindowHandle(window),
    };
    if (vkCreateWin32SurfaceKHR(g_vk.instance, &createInfo, NULL, &g_vk.surface) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create Win32 surface\n");
        return false;
    }
#endif

    printf("[Vulkan] Surface created successfully\n");
    return true;
}

static bool select_physical_device(void) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(g_vk.instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        fprintf(stderr, "[Vulkan] No physical devices found\n");
        return false;
    }

    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(g_vk.instance, &deviceCount, devices);

    for (uint32_t i = 0; i < deviceCount; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            g_vk.physicalDevice = devices[i];
            printf("[Vulkan] Selected Discrete GPU: %s\n", props.deviceName);
            break;
        }
    }

    if (!g_vk.physicalDevice) {
        g_vk.physicalDevice = devices[0];
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(g_vk.physicalDevice, &props);
        printf("[Vulkan] Selected GPU: %s\n", props.deviceName);
    }

    free(devices);
    return true;
}

static bool create_logical_device(void) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(g_vk.physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(g_vk.physicalDevice, &queueFamilyCount, queueFamilies);

    int graphicsFamily = -1;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(g_vk.physicalDevice, i, g_vk.surface, &presentSupport);
        if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
            graphicsFamily = i;
            break;
        }
    }
    free(queueFamilies);

    if (graphicsFamily == -1) {
        fprintf(stderr, "[Vulkan] No suitable queue family found\n");
        return false;
    }
    g_vk.graphicsQueueFamily = (uint32_t)graphicsFamily;

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = g_vk.graphicsQueueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME };
    
    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .dynamicRendering = VK_TRUE,
    };

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &dynamicRenderingFeatures,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = deviceExtensions,
    };

    if (vkCreateDevice(g_vk.physicalDevice, &createInfo, NULL, &g_vk.device) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create logical device\n");
        return false;
    }
    
    if (!load_device_functions()) return false;
    vkGetDeviceQueue(g_vk.device, g_vk.graphicsQueueFamily, 0, &g_vk.graphicsQueue);
    
    printf("[Vulkan] Logical Device created (Dynamic Rendering enabled)\n");
    return true;
}

static bool create_swapchain(sbgl_Window* window) {
    int w, h;
    sbgl_os_GetWindowSize(window, &w, &h);
    if (w <= 0) w = 800;
    if (h <= 0) h = 600;

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_vk.physicalDevice, g_vk.surface, &capabilities);

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = g_vk.surface,
        .minImageCount = capabilities.minImageCount + 1,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = { (uint32_t)w, (uint32_t)h },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
    };

    if (vkCreateSwapchainKHR(g_vk.device, &createInfo, NULL, &g_vk.swapchain) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create swapchain\n");
        return false;
    }
    
    g_vk.swapchainExtent = createInfo.imageExtent;
    g_vk.swapchainFormat = createInfo.imageFormat;

    vkGetSwapchainImagesKHR(g_vk.device, g_vk.swapchain, &g_vk.imageCount, NULL);
    g_vk.images = malloc(sizeof(VkImage) * g_vk.imageCount);
    vkGetSwapchainImagesKHR(g_vk.device, g_vk.swapchain, &g_vk.imageCount, g_vk.images);

    g_vk.imageViews = malloc(sizeof(VkImageView) * g_vk.imageCount);
    for (uint32_t i = 0; i < g_vk.imageCount; i++) {
        VkImageViewCreateInfo viewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = g_vk.images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = g_vk.swapchainFormat,
            .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
        };
        vkCreateImageView(g_vk.device, &viewInfo, NULL, &g_vk.imageViews[i]);
    }

    printf("[Vulkan] Swapchain created (%dx%d, %u images)\n", w, h, g_vk.imageCount);
    return true;
}

static bool create_sync_and_command(void) {
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = g_vk.graphicsQueueFamily,
    };
    if (vkCreateCommandPool(g_vk.device, &poolInfo, NULL, &g_vk.commandPool) != VK_SUCCESS) return false;

    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = g_vk.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    if (vkAllocateCommandBuffers(g_vk.device, &allocInfo, &g_vk.commandBuffer) != VK_SUCCESS) return false;

    VkSemaphoreCreateInfo semInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };

    if (vkCreateSemaphore(g_vk.device, &semInfo, NULL, &g_vk.imageAvailableSemaphore) != VK_SUCCESS) return false;
    if (vkCreateSemaphore(g_vk.device, &semInfo, NULL, &g_vk.renderFinishedSemaphore) != VK_SUCCESS) return false;
    if (vkCreateFence(g_vk.device, &fenceInfo, NULL, &g_vk.inFlightFence) != VK_SUCCESS) return false;

    return true;
}

bool sbgl_gfx_Init(sbgl_Window* window) {
    if (!load_vulkan_library()) return false;
    if (!create_instance()) return false;
    if (!create_surface(window)) return false;
    if (!select_physical_device()) return false;
    if (!create_logical_device()) return false;
    if (!create_swapchain(window)) return false;
    if (!create_sync_and_command()) return false;
    return true;
}

void sbgl_gfx_Shutdown(void) {
    if (g_vk.device) {
        vkDeviceWaitIdle(g_vk.device);
        vkDestroySemaphore(g_vk.device, g_vk.imageAvailableSemaphore, NULL);
        vkDestroySemaphore(g_vk.device, g_vk.renderFinishedSemaphore, NULL);
        vkDestroyFence(g_vk.device, g_vk.inFlightFence, NULL);
        vkDestroyCommandPool(g_vk.device, g_vk.commandPool, NULL);
        for (uint32_t i = 0; i < g_vk.imageCount; i++) vkDestroyImageView(g_vk.device, g_vk.imageViews[i], NULL);
        free(g_vk.images);
        free(g_vk.imageViews);
        vkDestroySwapchainKHR(g_vk.device, g_vk.swapchain, NULL);
        vkDestroyDevice(g_vk.device, NULL);
    }
    if (g_vk.instance) {
        vkDestroySurfaceKHR(g_vk.instance, g_vk.surface, NULL);
        vkDestroyInstance(g_vk.instance, NULL);
    }
    if (g_vk.libHandle) {
#ifdef _WIN32
        FreeLibrary((HMODULE)g_vk.libHandle);
#else
        dlclose(g_vk.libHandle);
#endif
    }
    memset(&g_vk, 0, sizeof(g_vk));
}

void sbgl_gfx_BeginFrame(float r, float g, float b, float a) {
    vkWaitForFences(g_vk.device, 1, &g_vk.inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(g_vk.device, 1, &g_vk.inFlightFence);
    vkAcquireNextImageKHR(g_vk.device, g_vk.swapchain, UINT64_MAX, g_vk.imageAvailableSemaphore, VK_NULL_HANDLE, &g_vk.currentImageIndex);

    vkResetCommandBuffer(g_vk.commandBuffer, 0);
    VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(g_vk.commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .image = g_vk.images[g_vk.currentImageIndex],
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };
    vkCmdPipelineBarrier(g_vk.commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    VkRenderingAttachmentInfo colorAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = g_vk.imageViews[g_vk.currentImageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {{{ r, g, b, a }}},
    };

    VkRenderingInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = { .extent = g_vk.swapchainExtent },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
    };

    vkCmdBeginRendering(g_vk.commandBuffer, &renderingInfo);
    vkCmdEndRendering(g_vk.commandBuffer);

    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = 0;
    vkCmdPipelineBarrier(g_vk.commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    vkEndCommandBuffer(g_vk.commandBuffer);
}

void sbgl_gfx_EndFrame(void) {
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &g_vk.imageAvailableSemaphore,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &g_vk.commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &g_vk.renderFinishedSemaphore,
    };
    vkQueueSubmit(g_vk.graphicsQueue, 1, &submitInfo, g_vk.inFlightFence);

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &g_vk.renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &g_vk.swapchain,
        .pImageIndices = &g_vk.currentImageIndex,
    };
    vkQueuePresentKHR(g_vk.graphicsQueue, &presentInfo);
}
