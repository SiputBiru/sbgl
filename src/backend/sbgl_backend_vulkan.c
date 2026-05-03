#include "sbgl_graphics_hal.h"
#include "core/sbgl_platform.h"
#include "core/sbl_arena.h"

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

static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = NULL;
static PFN_vkCreateInstance vkCreateInstance = NULL;
static PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = NULL;
static PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = NULL;
static PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = NULL;
static PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = NULL;
static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = NULL;
static PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = NULL;
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

static PFN_vkCreateBuffer vkCreateBuffer = NULL;
static PFN_vkDestroyBuffer vkDestroyBuffer = NULL;
static PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements = NULL;
static PFN_vkAllocateMemory vkAllocateMemory = NULL;
static PFN_vkFreeMemory vkFreeMemory = NULL;
static PFN_vkBindBufferMemory vkBindBufferMemory = NULL;
static PFN_vkMapMemory vkMapMemory = NULL;
static PFN_vkUnmapMemory vkUnmapMemory = NULL;

static PFN_vkCreateShaderModule vkCreateShaderModule = NULL;
static PFN_vkDestroyShaderModule vkDestroyShaderModule = NULL;

static PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines = NULL;
static PFN_vkDestroyPipeline vkDestroyPipeline = NULL;
static PFN_vkCreatePipelineLayout vkCreatePipelineLayout = NULL;
static PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout = NULL;

static PFN_vkCmdBindPipeline vkCmdBindPipeline = NULL;
static PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers = NULL;
static PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer = NULL;
static PFN_vkCmdDraw vkCmdDraw = NULL;
static PFN_vkCmdDrawIndexed vkCmdDrawIndexed = NULL;
static PFN_vkCmdSetViewport vkCmdSetViewport = NULL;
static PFN_vkCmdSetScissor vkCmdSetScissor = NULL;
static PFN_vkCmdPushConstants vkCmdPushConstants = NULL;

#define SBGL_MAX_BUFFERS 1024
#define SBGL_MAX_SHADERS 256
#define SBGL_MAX_PIPELINES 256

typedef struct {
    VkBuffer       handle;
    VkDeviceMemory memory;
    size_t         size;
    bool           active;
} SBGL_VulkanBuffer;

typedef struct {
    VkShaderModule module;
    bool           active;
} SBGL_VulkanShader;

typedef struct {
    VkPipeline       handle;
    VkPipelineLayout layout;
    bool             active;
} SBGL_VulkanPipeline;

typedef struct {
    void*            libHandle;
    VkInstance       instance;
    VkSurfaceKHR     surface;
    VkPhysicalDevice physicalDevice;
    VkDevice         device;
    VkQueue          graphicsQueue;
    uint32_t         graphicsQueueFamily;
    
    sbgl_Window*     window;
    SblArena*        arena;
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

    SBGL_VulkanBuffer   buffers[SBGL_MAX_BUFFERS];
    SBGL_VulkanShader   shaders[SBGL_MAX_SHADERS];
    SBGL_VulkanPipeline pipelines[SBGL_MAX_PIPELINES];
    sbgl_Pipeline       boundPipeline;
} SBGL_VulkanContext;

static SBGL_VulkanContext g_vk = {0};
static SblArenaMark g_swapchain_mark;

static void cleanup_swapchain(void) {
    for (uint32_t i = 0; i < g_vk.imageCount; i++) {
        vkDestroyImageView(g_vk.device, g_vk.imageViews[i], NULL);
    }
    vkDestroySwapchainKHR(g_vk.device, g_vk.swapchain, NULL);
    sbl_arena_rewind(g_vk.arena, g_swapchain_mark);
}

static bool create_swapchain(sbgl_Window* window);

static void recreate_swapchain(void) {
    int w = 0, h = 0;
    sbgl_os_GetWindowSize(g_vk.window, &w, &h);
    while (w == 0 || h == 0) {
        sbgl_os_GetWindowSize(g_vk.window, &w, &h);
        sbgl_os_PollEvents(g_vk.window);
    }

    vkDeviceWaitIdle(g_vk.device);

    cleanup_swapchain();
    create_swapchain(g_vk.window);
}

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
    LOAD_INST(vkGetPhysicalDeviceMemoryProperties);
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

    LOAD_DEV(vkCreateBuffer);
    LOAD_DEV(vkDestroyBuffer);
    LOAD_DEV(vkGetBufferMemoryRequirements);
    LOAD_DEV(vkAllocateMemory);
    LOAD_DEV(vkFreeMemory);
    LOAD_DEV(vkBindBufferMemory);
    LOAD_DEV(vkMapMemory);
    LOAD_DEV(vkUnmapMemory);

    LOAD_DEV(vkCreateShaderModule);
    LOAD_DEV(vkDestroyShaderModule);

    LOAD_DEV(vkCreateGraphicsPipelines);
    LOAD_DEV(vkDestroyPipeline);
    LOAD_DEV(vkCreatePipelineLayout);
    LOAD_DEV(vkDestroyPipelineLayout);

    LOAD_DEV(vkCmdBindPipeline);
    LOAD_DEV(vkCmdBindVertexBuffers);
    LOAD_DEV(vkCmdBindIndexBuffer);
    LOAD_DEV(vkCmdDraw);
    LOAD_DEV(vkCmdDrawIndexed);
    LOAD_DEV(vkCmdSetViewport);
    LOAD_DEV(vkCmdSetScissor);
    LOAD_DEV(vkCmdPushConstants);

    #undef LOAD_DEV
    return true;
}

static uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(g_vk.physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
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

    SblArenaMark mark = sbl_arena_mark(g_vk.arena);
    VkPhysicalDevice* devices = SBL_ARENA_PUSH_ARRAY(g_vk.arena, VkPhysicalDevice, deviceCount);
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

    sbl_arena_rewind(g_vk.arena, mark);
    return true;
}

static bool create_logical_device(void) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(g_vk.physicalDevice, &queueFamilyCount, NULL);
    
    SblArenaMark mark = sbl_arena_mark(g_vk.arena);
    VkQueueFamilyProperties* queueFamilies = SBL_ARENA_PUSH_ARRAY(g_vk.arena, VkQueueFamilyProperties, queueFamilyCount);
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
    sbl_arena_rewind(g_vk.arena, mark);

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

    VkExtent2D extent = { (uint32_t)w, (uint32_t)h };
    if (capabilities.currentExtent.width != 0xFFFFFFFF) {
        extent = capabilities.currentExtent;
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = g_vk.surface,
        .minImageCount = capabilities.minImageCount + 1,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = extent,
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
    
    g_swapchain_mark = sbl_arena_mark(g_vk.arena);
    g_vk.images = SBL_ARENA_PUSH_ARRAY(g_vk.arena, VkImage, g_vk.imageCount);
    vkGetSwapchainImagesKHR(g_vk.device, g_vk.swapchain, &g_vk.imageCount, g_vk.images);

    g_vk.imageViews = SBL_ARENA_PUSH_ARRAY(g_vk.arena, VkImageView, g_vk.imageCount);
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

    printf("[Vulkan] Swapchain created (%dx%d, %u images)\n", g_vk.swapchainExtent.width, g_vk.swapchainExtent.height, g_vk.imageCount);
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

bool sbgl_gfx_Init(sbgl_Window* window, struct SblArena* arena) {
    g_vk.window = window;
    g_vk.arena = arena;
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
        cleanup_swapchain();
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

bool sbgl_gfx_BeginFrame(float r, float g, float b, float a) {
    vkWaitForFences(g_vk.device, 1, &g_vk.inFlightFence, VK_TRUE, UINT64_MAX);

    if (sbgl_os_WasWindowResized(g_vk.window)) {
        recreate_swapchain();
    }

    VkResult result = vkAcquireNextImageKHR(g_vk.device, g_vk.swapchain, UINT64_MAX, g_vk.imageAvailableSemaphore, VK_NULL_HANDLE, &g_vk.currentImageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return false;
    }

    vkResetFences(g_vk.device, 1, &g_vk.inFlightFence);
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
    return true;
}

void sbgl_gfx_EndFrame(void) {
    vkCmdEndRendering(g_vk.commandBuffer);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = g_vk.images[g_vk.currentImageIndex],
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = 0,
    };
    vkCmdPipelineBarrier(g_vk.commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    vkEndCommandBuffer(g_vk.commandBuffer);

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
    VkResult result = vkQueuePresentKHR(g_vk.graphicsQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreate_swapchain();
    }
}

sbgl_Buffer sbgl_gfx_CreateBuffer(sbgl_BufferUsage usage, size_t size, const void* data) {
    uint32_t index = 0;
    for (; index < SBGL_MAX_BUFFERS; index++) {
        if (!g_vk.buffers[index].active) break;
    }
    if (index == SBGL_MAX_BUFFERS) return SBGL_INVALID_HANDLE;

    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = (usage & SBGL_BUFFER_USAGE_VERTEX ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0) |
                 (usage & SBGL_BUFFER_USAGE_INDEX ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    SBGL_VulkanBuffer* buffer = &g_vk.buffers[index];
    if (vkCreateBuffer(g_vk.device, &bufferInfo, NULL, &buffer->handle) != VK_SUCCESS) {
        return SBGL_INVALID_HANDLE;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(g_vk.device, buffer->handle, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };

    if (vkAllocateMemory(g_vk.device, &allocInfo, NULL, &buffer->memory) != VK_SUCCESS) {
        vkDestroyBuffer(g_vk.device, buffer->handle, NULL);
        return SBGL_INVALID_HANDLE;
    }

    vkBindBufferMemory(g_vk.device, buffer->handle, buffer->memory, 0);

    if (data) {
        void* mapped;
        vkMapMemory(g_vk.device, buffer->memory, 0, size, 0, &mapped);
        memcpy(mapped, data, size);
        vkUnmapMemory(g_vk.device, buffer->memory);
    }

    buffer->size = size;
    buffer->active = true;
    return (sbgl_Buffer)(index + 1);
}

void sbgl_gfx_DestroyBuffer(sbgl_Buffer handle) {
    if (handle == SBGL_INVALID_HANDLE) return;
    uint32_t index = (uint32_t)handle - 1;
    if (index >= SBGL_MAX_BUFFERS || !g_vk.buffers[index].active) return;

    vkDestroyBuffer(g_vk.device, g_vk.buffers[index].handle, NULL);
    vkFreeMemory(g_vk.device, g_vk.buffers[index].memory, NULL);
    g_vk.buffers[index].active = false;
}

sbgl_Shader sbgl_gfx_LoadShader(sbgl_ShaderStage stage, const uint32_t* bytecode, size_t size) {
    (void)stage;
    uint32_t index = 0;
    for (; index < SBGL_MAX_SHADERS; index++) {
        if (!g_vk.shaders[index].active) break;
    }
    if (index == SBGL_MAX_SHADERS) return SBGL_INVALID_HANDLE;

    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = bytecode,
    };

    if (vkCreateShaderModule(g_vk.device, &createInfo, NULL, &g_vk.shaders[index].module) != VK_SUCCESS) {
        return SBGL_INVALID_HANDLE;
    }

    g_vk.shaders[index].active = true;
    return (sbgl_Shader)(index + 1);
}

void sbgl_gfx_DestroyShader(sbgl_Shader handle) {
    if (handle == SBGL_INVALID_HANDLE) return;
    uint32_t index = (uint32_t)handle - 1;
    if (index >= SBGL_MAX_SHADERS || !g_vk.shaders[index].active) return;

    vkDestroyShaderModule(g_vk.device, g_vk.shaders[index].module, NULL);
    g_vk.shaders[index].active = false;
}

sbgl_Pipeline sbgl_gfx_CreatePipeline(const sbgl_PipelineConfig* config) {
    uint32_t index = 0;
    for (; index < SBGL_MAX_PIPELINES; index++) {
        if (!g_vk.pipelines[index].active) break;
    }
    if (index == SBGL_MAX_PIPELINES) return SBGL_INVALID_HANDLE;

    VkPipelineShaderStageCreateInfo shaderStages[2] = {0};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = g_vk.shaders[config->vertexShader - 1].module;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = g_vk.shaders[config->fragmentShader - 1].module;
    shaderStages[1].pName = "main";

    VkVertexInputBindingDescription bindingDescription = {
        .binding = 0,
        .stride = config->vertexLayout.stride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    SblArenaMark mark = sbl_arena_mark(g_vk.arena);
    VkVertexInputAttributeDescription* attributeDescriptions = SBL_ARENA_PUSH_ARRAY(g_vk.arena, VkVertexInputAttributeDescription, config->vertexLayout.attributeCount);
    for (uint32_t i = 0; i < config->vertexLayout.attributeCount; i++) {
        attributeDescriptions[i].binding = 0;
        attributeDescriptions[i].location = config->vertexLayout.attributes[i].location;
        attributeDescriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT; // TODO: Map from attribute config
        attributeDescriptions[i].offset = config->vertexLayout.attributes[i].offset;
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = config->vertexLayout.attributeCount,
        .pVertexAttributeDescriptions = attributeDescriptions,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };

    VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = 64, // Sufficient for basic data (vec2 mousePos, float time, etc.)
    };
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(g_vk.device, &pipelineLayoutInfo, NULL, &g_vk.pipelines[index].layout) != VK_SUCCESS) {
        sbl_arena_rewind(g_vk.arena, mark);
        return SBGL_INVALID_HANDLE;
    }

    VkPipelineRenderingCreateInfo renderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &g_vk.swapchainFormat,
    };

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingCreateInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = g_vk.pipelines[index].layout,
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
    };

    if (vkCreateGraphicsPipelines(g_vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &g_vk.pipelines[index].handle) != VK_SUCCESS) {
        vkDestroyPipelineLayout(g_vk.device, g_vk.pipelines[index].layout, NULL);
        sbl_arena_rewind(g_vk.arena, mark);
        return SBGL_INVALID_HANDLE;
    }

    sbl_arena_rewind(g_vk.arena, mark);
    g_vk.pipelines[index].active = true;
    return (sbgl_Pipeline)(index + 1);
}

void sbgl_gfx_DestroyPipeline(sbgl_Pipeline handle) {
    if (handle == SBGL_INVALID_HANDLE) return;
    uint32_t index = (uint32_t)handle - 1;
    if (index >= SBGL_MAX_PIPELINES || !g_vk.pipelines[index].active) return;

    vkDestroyPipeline(g_vk.device, g_vk.pipelines[index].handle, NULL);
    vkDestroyPipelineLayout(g_vk.device, g_vk.pipelines[index].layout, NULL);
    g_vk.pipelines[index].active = false;
}

void sbgl_gfx_BindPipeline(sbgl_Pipeline handle) {
    if (handle == SBGL_INVALID_HANDLE) return;
    uint32_t index = (uint32_t)handle - 1;
    if (index >= SBGL_MAX_PIPELINES || !g_vk.pipelines[index].active) return;

    vkCmdBindPipeline(g_vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vk.pipelines[index].handle);
    g_vk.boundPipeline = handle;

    VkViewport viewport = {
        .x = 0.0f, .y = 0.0f,
        .width = (float)g_vk.swapchainExtent.width,
        .height = (float)g_vk.swapchainExtent.height,
        .minDepth = 0.0f, .maxDepth = 1.0f,
    };
    vkCmdSetViewport(g_vk.commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = { .offset = {0, 0}, .extent = g_vk.swapchainExtent };
    vkCmdSetScissor(g_vk.commandBuffer, 0, 1, &scissor);
}

void sbgl_gfx_BindBuffer(sbgl_Buffer handle, sbgl_BufferUsage usage) {
    if (handle == SBGL_INVALID_HANDLE) return;
    uint32_t index = (uint32_t)handle - 1;
    if (index >= SBGL_MAX_BUFFERS || !g_vk.buffers[index].active) return;

    if (usage == SBGL_BUFFER_USAGE_VERTEX) {
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(g_vk.commandBuffer, 0, 1, &g_vk.buffers[index].handle, offsets);
    } else if (usage == SBGL_BUFFER_USAGE_INDEX) {
        vkCmdBindIndexBuffer(g_vk.commandBuffer, g_vk.buffers[index].handle, 0, VK_INDEX_TYPE_UINT32);
    }
}

void sbgl_gfx_Draw(uint32_t vertexCount, uint32_t firstVertex) {
    vkCmdDraw(g_vk.commandBuffer, vertexCount, 1, firstVertex, 0);
}

void sbgl_gfx_DrawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset) {
    vkCmdDrawIndexed(g_vk.commandBuffer, indexCount, 1, firstIndex, vertexOffset, 0);
}

void sbgl_gfx_PushConstants(size_t size, const void* data) {
    if (g_vk.boundPipeline == SBGL_INVALID_HANDLE) return;
    uint32_t index = (uint32_t)g_vk.boundPipeline - 1;
    vkCmdPushConstants(g_vk.commandBuffer, g_vk.pipelines[index].layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, (uint32_t)size, data);
}
