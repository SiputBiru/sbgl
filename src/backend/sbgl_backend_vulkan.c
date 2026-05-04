#include "core/sbgl_platform.h"
#include "core/sbl_arena.h"
#include "sbgl_graphics_hal.h"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#ifdef SBGL_PLATFORM_WAYLAND
#include <dlfcn.h>
#include <vulkan/vulkan_wayland.h>
#define SBGL_VK_LIB "libvulkan.so.1"
#elif defined(SBGL_PLATFORM_X11)
#include <X11/Xlib.h>
#include <dlfcn.h>
#include <vulkan/vulkan_xlib.h>
#define SBGL_VK_LIB "libvulkan.so.1"
#elif defined(_WIN32)
#include <vulkan/vulkan_win32.h>
#include <windows.h>
#define SBGL_VK_LIB "vulkan-1.dll"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Vulkan Function Pointers ---

typedef struct {
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
	PFN_vkCreateInstance vkCreateInstance;
	PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
	PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
	PFN_vkCreateDevice vkCreateDevice;
	PFN_vkDestroyInstance vkDestroyInstance;
	PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
	PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;

#ifdef SBGL_PLATFORM_WAYLAND
	PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
#elif defined(SBGL_PLATFORM_X11)
	PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
#elif defined(_WIN32)
	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#endif

	PFN_vkGetDeviceQueue vkGetDeviceQueue;
	PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
	PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
	PFN_vkCreateImageView vkCreateImageView;
	PFN_vkDestroyImageView vkDestroyImageView;
	PFN_vkCreateCommandPool vkCreateCommandPool;
	PFN_vkDestroyCommandPool vkDestroyCommandPool;
	PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
	PFN_vkCreateSemaphore vkCreateSemaphore;
	PFN_vkDestroySemaphore vkDestroySemaphore;
	PFN_vkCreateFence vkCreateFence;
	PFN_vkDestroyFence vkDestroyFence;
	PFN_vkWaitForFences vkWaitForFences;
	PFN_vkResetFences vkResetFences;
	PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
	PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
	PFN_vkEndCommandBuffer vkEndCommandBuffer;
	PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
	PFN_vkCmdBeginRendering vkCmdBeginRendering;
	PFN_vkCmdEndRendering vkCmdEndRendering;
	PFN_vkResetCommandBuffer vkResetCommandBuffer;
	PFN_vkQueueSubmit vkQueueSubmit;
	PFN_vkQueuePresentKHR vkQueuePresentKHR;
	PFN_vkDestroyDevice vkDestroyDevice;
	PFN_vkDeviceWaitIdle vkDeviceWaitIdle;

	PFN_vkCreateBuffer vkCreateBuffer;
	PFN_vkDestroyBuffer vkDestroyBuffer;
	PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
	PFN_vkAllocateMemory vkAllocateMemory;
	PFN_vkFreeMemory vkFreeMemory;
	PFN_vkBindBufferMemory vkBindBufferMemory;
	PFN_vkMapMemory vkMapMemory;
	PFN_vkUnmapMemory vkUnmapMemory;

	PFN_vkCreateShaderModule vkCreateShaderModule;
	PFN_vkDestroyShaderModule vkDestroyShaderModule;

	PFN_vkCreateImage vkCreateImage;
	PFN_vkDestroyImage vkDestroyImage;
	PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
	PFN_vkBindImageMemory vkBindImageMemory;
	PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;

	PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
	PFN_vkDestroyPipeline vkDestroyPipeline;
	PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
	PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;

	PFN_vkCmdBindPipeline vkCmdBindPipeline;
	PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
	PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
	PFN_vkCmdDraw vkCmdDraw;
	PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
	PFN_vkCmdSetViewport vkCmdSetViewport;
	PFN_vkCmdSetScissor vkCmdSetScissor;
	PFN_vkCmdPushConstants vkCmdPushConstants;
} SBGL_VulkanDispatch;

#define SBGL_MAX_BUFFERS 1024
#define SBGL_MAX_SHADERS 256
#define SBGL_MAX_PIPELINES 256
#define SBGL_MAX_FRAMES_IN_FLIGHT 2
#define SBGL_MAX_SWAPCHAIN_IMAGES 8

typedef struct {
	VkBuffer handle;
	VkDeviceMemory memory;
	size_t size;
	bool active;
} SBGL_VulkanBuffer;

typedef struct {
	VkShaderModule module;
	bool active;
} SBGL_VulkanShader;

typedef struct {
	VkPipeline handle;
	VkPipelineLayout layout;
	bool active;
} SBGL_VulkanPipeline;

struct sbgl_GfxContext {
	void* libHandle;
	SBGL_VulkanDispatch vk;

	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;

	sbgl_Window* window;
	SblArena* arena;
	VkSwapchainKHR swapchain;
	VkFormat swapchainFormat;
	VkExtent2D swapchainExtent;
	uint32_t imageCount;
	VkImage* images;
	VkImageView* imageViews;
	SblArenaMark swapchainMark;

	VkImage depthImage;
	VkDeviceMemory depthMemory;
	VkImageView depthImageView;
	VkFormat depthFormat;

	VkCommandPool commandPool;
	VkCommandBuffer commandBuffers[SBGL_MAX_FRAMES_IN_FLIGHT];
	VkSemaphore imageAvailableSemaphores[SBGL_MAX_FRAMES_IN_FLIGHT];
	VkSemaphore renderFinishedSemaphores[SBGL_MAX_SWAPCHAIN_IMAGES];
	VkFence inFlightFences[SBGL_MAX_FRAMES_IN_FLIGHT];

	uint32_t currentImageIndex;
	uint32_t currentFrame;

	SBGL_VulkanBuffer buffers[SBGL_MAX_BUFFERS];
	SBGL_VulkanShader shaders[SBGL_MAX_SHADERS];
	SBGL_VulkanPipeline pipelines[SBGL_MAX_PIPELINES];
	sbgl_Pipeline boundPipeline;
};

static void cleanup_swapchain(sbgl_GfxContext* ctx) {
	ctx->vk.vkDestroyImageView(ctx->device, ctx->depthImageView, NULL);
	ctx->vk.vkDestroyImage(ctx->device, ctx->depthImage, NULL);
	ctx->vk.vkFreeMemory(ctx->device, ctx->depthMemory, NULL);

	for (uint32_t i = 0; i < ctx->imageCount; i++) {
		ctx->vk.vkDestroyImageView(ctx->device, ctx->imageViews[i], NULL);
	}
	ctx->vk.vkDestroySwapchainKHR(ctx->device, ctx->swapchain, NULL);
	sbl_arena_rewind(ctx->arena, ctx->swapchainMark);
}

static bool create_swapchain(sbgl_GfxContext* ctx, sbgl_Window* window);

static void recreate_swapchain(sbgl_GfxContext* ctx) {
	int w = 0, h = 0;
	sbgl_os_GetWindowSize(ctx->window, &w, &h);
	while (w == 0 || h == 0) {
		sbgl_os_GetWindowSize(ctx->window, &w, &h);
		sbgl_os_PollEvents(ctx->window);
	}

	ctx->vk.vkDeviceWaitIdle(ctx->device);

	cleanup_swapchain(ctx);
	create_swapchain(ctx, ctx->window);
}

static bool load_vulkan_library(sbgl_GfxContext* ctx) {
#ifdef _WIN32
	ctx->libHandle = LoadLibraryA(SBGL_VK_LIB);
#else
	ctx->libHandle = dlopen(SBGL_VK_LIB, RTLD_NOW);
#endif

	if (!ctx->libHandle) {
		fprintf(stderr, "[Vulkan] Failed to load %s\n", SBGL_VK_LIB);
		return false;
	}

#ifdef _WIN32
	void* addr = GetProcAddress((HMODULE)ctx->libHandle, "vkGetInstanceProcAddr");
	memcpy(&ctx->vk.vkGetInstanceProcAddr, &addr, sizeof(addr));
#else
	void* sym = dlsym(ctx->libHandle, "vkGetInstanceProcAddr");
	memcpy(&ctx->vk.vkGetInstanceProcAddr, &sym, sizeof(sym));
#endif

	if (!ctx->vk.vkGetInstanceProcAddr)
		return false;

	ctx->vk.vkCreateInstance = (PFN_vkCreateInstance)ctx->vk.vkGetInstanceProcAddr(NULL, "vkCreateInstance");
	if (!ctx->vk.vkCreateInstance)
		return false;

	return true;
}

static bool load_instance_functions(sbgl_GfxContext* ctx) {
#define LOAD_INST(name)                                                                            \
	ctx->vk.name = (PFN_##name)ctx->vk.vkGetInstanceProcAddr(ctx->instance, #name);                \
	if (!ctx->vk.name) {                                                                           \
		fprintf(stderr, "[Vulkan] Failed to load instance function: %s\n", #name);                 \
		return false;                                                                              \
	}

	LOAD_INST(vkGetDeviceProcAddr);
	LOAD_INST(vkDestroyInstance);
	LOAD_INST(vkDestroySurfaceKHR);
	LOAD_INST(vkEnumeratePhysicalDevices);
	LOAD_INST(vkGetPhysicalDeviceProperties);
	LOAD_INST(vkGetPhysicalDeviceQueueFamilyProperties);
	LOAD_INST(vkGetPhysicalDeviceSurfaceSupportKHR);
	LOAD_INST(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
	LOAD_INST(vkGetPhysicalDeviceSurfaceFormatsKHR);
	LOAD_INST(vkGetPhysicalDeviceMemoryProperties);
	LOAD_INST(vkGetPhysicalDeviceFormatProperties);
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

static bool load_device_functions(sbgl_GfxContext* ctx) {
#define LOAD_DEV(name)                                                                             \
	ctx->vk.name = (PFN_##name)ctx->vk.vkGetDeviceProcAddr(ctx->device, #name);                    \
	if (!ctx->vk.name) {                                                                           \
		fprintf(stderr, "[Vulkan] Failed to load device function: %s\n", #name);                   \
		return false;                                                                              \
	}

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

	LOAD_DEV(vkCreateImage);
	LOAD_DEV(vkDestroyImage);
	LOAD_DEV(vkGetImageMemoryRequirements);
	LOAD_DEV(vkBindImageMemory);

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

static uint32_t find_memory_type(sbgl_GfxContext* ctx, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	ctx->vk.vkGetPhysicalDeviceMemoryProperties(ctx->physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	return 0;
}

static bool create_instance(sbgl_GfxContext* ctx) {
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

#ifndef NDEBUG
	const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
	createInfo.enabledLayerCount = 1;
	createInfo.ppEnabledLayerNames = layers;
	printf("[Vulkan] Enabling Validation Layers\n");
#endif

	if (ctx->vk.vkCreateInstance(&createInfo, NULL, &ctx->instance) != VK_SUCCESS) {
		fprintf(stderr, "[Vulkan] Failed to create instance\n");
		return false;
	}

	if (!load_instance_functions(ctx))
		return false;
	printf("[Vulkan] Instance created successfully (v1.3)\n");
	return true;
}

static bool create_surface(sbgl_GfxContext* ctx, sbgl_Window* window) {
#ifdef SBGL_PLATFORM_WAYLAND
	VkWaylandSurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
		.display = (struct wl_display*)sbgl_os_GetNativeDisplayHandle(window),
		.surface = (struct wl_surface*)sbgl_os_GetNativeWindowHandle(window),
	};
	if (ctx->vk.vkCreateWaylandSurfaceKHR(ctx->instance, &createInfo, NULL, &ctx->surface) != VK_SUCCESS) {
		fprintf(stderr, "[Vulkan] Failed to create Wayland surface\n");
		return false;
	}
#elif defined(SBGL_PLATFORM_X11)
	VkXlibSurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
		.dpy = (Display*)sbgl_os_GetNativeDisplayHandle(window),
		.window = (Window)(uintptr_t)sbgl_os_GetNativeWindowHandle(window),
	};
	if (ctx->vk.vkCreateXlibSurfaceKHR(ctx->instance, &createInfo, NULL, &ctx->surface) != VK_SUCCESS) {
		fprintf(stderr, "[Vulkan] Failed to create Xlib surface\n");
		return false;
	}
#elif defined(_WIN32)
	VkWin32SurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = (HINSTANCE)sbgl_os_GetNativeInstanceHandle(window),
		.hwnd = (HWND)sbgl_os_GetNativeWindowHandle(window),
	};
	if (ctx->vk.vkCreateWin32SurfaceKHR(ctx->instance, &createInfo, NULL, &ctx->surface) != VK_SUCCESS) {
		fprintf(stderr, "[Vulkan] Failed to create Win32 surface\n");
		return false;
	}
#endif

	printf("[Vulkan] Surface created successfully\n");
	return true;
}

static bool select_physical_device(sbgl_GfxContext* ctx) {
	uint32_t deviceCount = 0;
	ctx->vk.vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, NULL);
	if (deviceCount == 0) {
		fprintf(stderr, "[Vulkan] No physical devices found\n");
		return false;
	}

	SblArenaMark mark = sbl_arena_mark(ctx->arena);
	VkPhysicalDevice* devices = SBL_ARENA_PUSH_ARRAY(ctx->arena, VkPhysicalDevice, deviceCount);
	if (!devices) return false;
	ctx->vk.vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, devices);

	for (uint32_t i = 0; i < deviceCount; i++) {
		VkPhysicalDeviceProperties props;
		ctx->vk.vkGetPhysicalDeviceProperties(devices[i], &props);
		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			ctx->physicalDevice = devices[i];
			printf("[Vulkan] Selected Discrete GPU: %s\n", props.deviceName);
			break;
		}
	}

	if (!ctx->physicalDevice) {
		ctx->physicalDevice = devices[0];
		VkPhysicalDeviceProperties props;
		ctx->vk.vkGetPhysicalDeviceProperties(ctx->physicalDevice, &props);
		printf("[Vulkan] Selected GPU: %s\n", props.deviceName);
	}

	sbl_arena_rewind(ctx->arena, mark);
	return true;
}

static bool create_logical_device(sbgl_GfxContext* ctx) {
	uint32_t queueFamilyCount = 0;
	ctx->vk.vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, NULL);

	SblArenaMark mark = sbl_arena_mark(ctx->arena);
	VkQueueFamilyProperties* queueFamilies =
		SBL_ARENA_PUSH_ARRAY(ctx->arena, VkQueueFamilyProperties, queueFamilyCount);
	if (!queueFamilies) return false;
	ctx->vk.vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, queueFamilies);

	int graphicsFamily = -1;
	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		VkBool32 presentSupport = false;
		ctx->vk.vkGetPhysicalDeviceSurfaceSupportKHR(ctx->physicalDevice, i, ctx->surface, &presentSupport);
		if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
			graphicsFamily = i;
			break;
		}
	}
	sbl_arena_rewind(ctx->arena, mark);

	if (graphicsFamily == -1) {
		fprintf(stderr, "[Vulkan] No suitable queue family found\n");
		return false;
	}
	ctx->graphicsQueueFamily = (uint32_t)graphicsFamily;

	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = ctx->graphicsQueueFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority,
	};

	const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
									   VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME };

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

	if (ctx->vk.vkCreateDevice(ctx->physicalDevice, &createInfo, NULL, &ctx->device) != VK_SUCCESS) {
		fprintf(stderr, "[Vulkan] Failed to create logical device\n");
		return false;
	}

	if (!load_device_functions(ctx))
		return false;
	ctx->vk.vkGetDeviceQueue(ctx->device, ctx->graphicsQueueFamily, 0, &ctx->graphicsQueue);

	printf("[Vulkan] Logical Device created (Dynamic Rendering enabled)\n");
	return true;
}

static bool find_depth_format(sbgl_GfxContext* ctx) {
	VkFormat candidates[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
							  VK_FORMAT_D24_UNORM_S8_UINT };
	for (uint32_t i = 0; i < 3; i++) {
		VkFormatProperties props;
		ctx->vk.vkGetPhysicalDeviceFormatProperties(ctx->physicalDevice, candidates[i], &props);
		if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			ctx->depthFormat = candidates[i];
			return true;
		}
	}
	return false;
}

static bool create_depth_resources(sbgl_GfxContext* ctx) {
	if (!find_depth_format(ctx))
		return false;

	VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent = { .width = ctx->swapchainExtent.width,
					.height = ctx->swapchainExtent.height,
					.depth = 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.format = ctx->depthFormat,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	if (ctx->vk.vkCreateImage(ctx->device, &imageInfo, NULL, &ctx->depthImage) != VK_SUCCESS)
		return false;

	VkMemoryRequirements memRequirements;
	ctx->vk.vkGetImageMemoryRequirements(ctx->device, ctx->depthImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = find_memory_type(ctx, memRequirements.memoryTypeBits,
											VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	if (ctx->vk.vkAllocateMemory(ctx->device, &allocInfo, NULL, &ctx->depthMemory) != VK_SUCCESS)
		return false;

	ctx->vk.vkBindImageMemory(ctx->device, ctx->depthImage, ctx->depthMemory, 0);

	VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = ctx->depthImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = ctx->depthFormat,
		.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
							  .levelCount = 1,
							  .layerCount = 1 },
	};

	if (ctx->vk.vkCreateImageView(ctx->device, &viewInfo, NULL, &ctx->depthImageView) != VK_SUCCESS)
		return false;

	return true;
}

static bool create_swapchain(sbgl_GfxContext* ctx, sbgl_Window* window) {
	int w, h;
	sbgl_os_GetWindowSize(window, &w, &h);

	VkSurfaceCapabilitiesKHR capabilities;
	ctx->vk.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physicalDevice, ctx->surface, &capabilities);

	VkExtent2D extent = { (uint32_t)w, (uint32_t)h };
	if (capabilities.currentExtent.width != 0xFFFFFFFF) {
		extent = capabilities.currentExtent;
	}

	if (extent.width == 0 || extent.height == 0) {
		return false;
	}

	uint32_t formatCount;
	ctx->vk.vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physicalDevice, ctx->surface, &formatCount, NULL);
	if (formatCount == 0) {
		fprintf(stderr, "[Vulkan] No supported surface formats found\n");
		return false;
	}
	VkSurfaceFormatKHR formats[64];
	if (formatCount > 64)
		formatCount = 64;
	ctx->vk.vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physicalDevice, ctx->surface, &formatCount, formats);

	VkSurfaceFormatKHR selectedFormat = formats[0];
	for (uint32_t i = 0; i < formatCount; i++) {
		if ((formats[i].format == VK_FORMAT_B8G8R8A8_SRGB ||
			 formats[i].format == VK_FORMAT_R8G8B8A8_SRGB) &&
			formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			selectedFormat = formats[i];
			break;
		}
	}

	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
		imageCount = capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = ctx->surface,
		.minImageCount = imageCount,
		.imageFormat = selectedFormat.format,
		.imageColorSpace = selectedFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
		.clipped = VK_TRUE,
	};

	if (ctx->vk.vkCreateSwapchainKHR(ctx->device, &createInfo, NULL, &ctx->swapchain) != VK_SUCCESS) {
		fprintf(stderr, "[Vulkan] Failed to create swapchain\n");
		return false;
	}

	ctx->swapchainExtent = createInfo.imageExtent;
	ctx->swapchainFormat = createInfo.imageFormat;

	ctx->vk.vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->imageCount, NULL);

	ctx->swapchainMark = sbl_arena_mark(ctx->arena);
	ctx->images = SBL_ARENA_PUSH_ARRAY(ctx->arena, VkImage, ctx->imageCount);
	if (!ctx->images) return false;
	ctx->vk.vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->imageCount, ctx->images);

	ctx->imageViews = SBL_ARENA_PUSH_ARRAY(ctx->arena, VkImageView, ctx->imageCount);
	if (!ctx->imageViews) return false;
	for (uint32_t i = 0; i < ctx->imageCount; i++) {
		VkImageViewCreateInfo viewInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = ctx->images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = ctx->swapchainFormat,
			.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
								  .levelCount = 1,
								  .layerCount = 1 },
		};
		ctx->vk.vkCreateImageView(ctx->device, &viewInfo, NULL, &ctx->imageViews[i]);
	}

	printf(
		"[Vulkan] Swapchain created (%dx%d, %u images, format: %d)\n",
		ctx->swapchainExtent.width,
		ctx->swapchainExtent.height,
		ctx->imageCount,
		ctx->swapchainFormat
	);

	if (!create_depth_resources(ctx))
		return false;

	return true;
}

static bool create_sync_and_command(sbgl_GfxContext* ctx) {
	VkCommandPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = ctx->graphicsQueueFamily,
	};
	if (ctx->vk.vkCreateCommandPool(ctx->device, &poolInfo, NULL, &ctx->commandPool) != VK_SUCCESS)
		return false;

	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = ctx->commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = SBGL_MAX_FRAMES_IN_FLIGHT,
	};
	if (ctx->vk.vkAllocateCommandBuffers(ctx->device, &allocInfo, ctx->commandBuffers) != VK_SUCCESS)
		return false;

	VkSemaphoreCreateInfo semInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
									.flags = VK_FENCE_CREATE_SIGNALED_BIT };

	for (uint32_t i = 0; i < SBGL_MAX_FRAMES_IN_FLIGHT; i++) {
		if (ctx->vk.vkCreateSemaphore(ctx->device, &semInfo, NULL, &ctx->imageAvailableSemaphores[i]) !=
				VK_SUCCESS ||
			ctx->vk.vkCreateFence(ctx->device, &fenceInfo, NULL, &ctx->inFlightFences[i]) != VK_SUCCESS)
			return false;
	}

	for (uint32_t i = 0; i < SBGL_MAX_SWAPCHAIN_IMAGES; i++) {
		if (ctx->vk.vkCreateSemaphore(ctx->device, &semInfo, NULL, &ctx->renderFinishedSemaphores[i]) !=
			VK_SUCCESS)
			return false;
	}

	return true;
}

sbgl_GfxContext* sbgl_gfx_Init(sbgl_Window* window, struct SblArena* arena) {
	sbgl_GfxContext* ctx = SBL_ARENA_PUSH_STRUCT_ZERO(arena, sbgl_GfxContext);
	if (!ctx) return NULL;

	ctx->window = window;
	ctx->arena = arena;

	if (!load_vulkan_library(ctx) ||
		!create_instance(ctx) ||
		!create_surface(ctx, window) ||
		!select_physical_device(ctx) ||
		!create_logical_device(ctx) ||
		!create_swapchain(ctx, window) ||
		!create_sync_and_command(ctx)) {
		sbgl_gfx_Shutdown(ctx);
		return NULL;
	}
	return ctx;
}

void sbgl_gfx_Shutdown(sbgl_GfxContext* ctx) {
	if (!ctx) return;

	if (ctx->device) {
		ctx->vk.vkDeviceWaitIdle(ctx->device);
		for (uint32_t i = 0; i < SBGL_MAX_FRAMES_IN_FLIGHT; i++) {
			ctx->vk.vkDestroySemaphore(ctx->device, ctx->imageAvailableSemaphores[i], NULL);
			ctx->vk.vkDestroyFence(ctx->device, ctx->inFlightFences[i], NULL);
		}
		for (uint32_t i = 0; i < SBGL_MAX_SWAPCHAIN_IMAGES; i++) {
			ctx->vk.vkDestroySemaphore(ctx->device, ctx->renderFinishedSemaphores[i], NULL);
		}
		ctx->vk.vkDestroyCommandPool(ctx->device, ctx->commandPool, NULL);
		cleanup_swapchain(ctx);
		ctx->vk.vkDestroyDevice(ctx->device, NULL);
	}
	if (ctx->instance) {
		ctx->vk.vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
		ctx->vk.vkDestroyInstance(ctx->instance, NULL);
	}
	if (ctx->libHandle) {
#ifdef _WIN32
		FreeLibrary((HMODULE)ctx->libHandle);
#else
		dlclose(ctx->libHandle);
#endif
	}
}

bool sbgl_gfx_BeginFrame(sbgl_GfxContext* ctx, float r, float g, float b, float a) {
	ctx->vk.vkWaitForFences(ctx->device, 1, &ctx->inFlightFences[ctx->currentFrame], VK_TRUE, UINT64_MAX);

	if (sbgl_os_WasWindowResized(ctx->window)) {
		recreate_swapchain(ctx);
	}

	VkResult result = ctx->vk.vkAcquireNextImageKHR(
		ctx->device,
		ctx->swapchain,
		UINT64_MAX,
		ctx->imageAvailableSemaphores[ctx->currentFrame],
		VK_NULL_HANDLE,
		&ctx->currentImageIndex
	);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_swapchain(ctx);
		return false;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		return false;
	}

	ctx->vk.vkResetFences(ctx->device, 1, &ctx->inFlightFences[ctx->currentFrame]);
	ctx->vk.vkResetCommandBuffer(ctx->commandBuffers[ctx->currentFrame], 0);
	VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	ctx->vk.vkBeginCommandBuffer(ctx->commandBuffers[ctx->currentFrame], &beginInfo);

	VkImageMemoryBarrier barriers[2] = { 0 };
	barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barriers[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barriers[0].image = ctx->images[ctx->currentImageIndex];
	barriers[0].subresourceRange = (VkImageSubresourceRange){ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
															  .levelCount = 1,
															  .layerCount = 1 };
	barriers[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barriers[1].newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	barriers[1].image = ctx->depthImage;
	barriers[1].subresourceRange = (VkImageSubresourceRange){ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
															  .levelCount = 1,
															  .layerCount = 1 };
	barriers[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	ctx->vk.vkCmdPipelineBarrier(
		ctx->commandBuffers[ctx->currentFrame],
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		0,
		0,
		NULL,
		0,
		NULL,
		2,
		barriers
	);

	VkRenderingAttachmentInfo colorAttachment = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = ctx->imageViews[ctx->currentImageIndex],
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = { { { r, g, b, a } } },
	};

	VkRenderingAttachmentInfo depthAttachment = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = ctx->depthImageView,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = { .depthStencil = { 1.0f, 0 } },
	};

	VkRenderingInfo renderingInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = { .extent = ctx->swapchainExtent },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachment,
		.pDepthAttachment = &depthAttachment,
	};

	ctx->vk.vkCmdBeginRendering(ctx->commandBuffers[ctx->currentFrame], &renderingInfo);
	return true;
}

void sbgl_gfx_EndFrame(sbgl_GfxContext* ctx) {
	ctx->vk.vkCmdEndRendering(ctx->commandBuffers[ctx->currentFrame]);

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.image = ctx->images[ctx->currentImageIndex],
		.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							  .levelCount = 1,
							  .layerCount = 1 },
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = 0,
	};
	ctx->vk.vkCmdPipelineBarrier(
		ctx->commandBuffers[ctx->currentFrame],
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0,
		0,
		NULL,
		0,
		NULL,
		1,
		&barrier
	);

	ctx->vk.vkEndCommandBuffer(ctx->commandBuffers[ctx->currentFrame]);

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &ctx->imageAvailableSemaphores[ctx->currentFrame],
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &ctx->commandBuffers[ctx->currentFrame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &ctx->renderFinishedSemaphores[ctx->currentImageIndex],
	};
	ctx->vk.vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo, ctx->inFlightFences[ctx->currentFrame]);

	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &ctx->renderFinishedSemaphores[ctx->currentImageIndex],
		.swapchainCount = 1,
		.pSwapchains = &ctx->swapchain,
		.pImageIndices = &ctx->currentImageIndex,
	};
	VkResult result = ctx->vk.vkQueuePresentKHR(ctx->graphicsQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreate_swapchain(ctx);
	}

	ctx->currentFrame = (ctx->currentFrame + 1) % SBGL_MAX_FRAMES_IN_FLIGHT;
}

void sbgl_gfx_DeviceWaitIdle(sbgl_GfxContext* ctx) {
	if (ctx && ctx->device) {
		ctx->vk.vkDeviceWaitIdle(ctx->device);
	}
}

sbgl_Buffer sbgl_gfx_CreateBuffer(sbgl_GfxContext* ctx, sbgl_BufferUsage usage, size_t size, const void* data) {
	uint32_t index = 0;
	for (; index < SBGL_MAX_BUFFERS; index++) {
		if (!ctx->buffers[index].active)
			break;
	}
	if (index == SBGL_MAX_BUFFERS)
		return SBGL_INVALID_HANDLE;

	VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = (usage & SBGL_BUFFER_USAGE_VERTEX ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0) |
				 (usage & SBGL_BUFFER_USAGE_INDEX ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0),
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	SBGL_VulkanBuffer* buffer = &ctx->buffers[index];
	if (ctx->vk.vkCreateBuffer(ctx->device, &bufferInfo, NULL, &buffer->handle) != VK_SUCCESS) {
		return SBGL_INVALID_HANDLE;
	}

	VkMemoryRequirements memRequirements;
	ctx->vk.vkGetBufferMemoryRequirements(ctx->device, buffer->handle, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = find_memory_type(ctx, 
			memRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		),
	};

	if (ctx->vk.vkAllocateMemory(ctx->device, &allocInfo, NULL, &buffer->memory) != VK_SUCCESS) {
		ctx->vk.vkDestroyBuffer(ctx->device, buffer->handle, NULL);
		return SBGL_INVALID_HANDLE;
	}

	ctx->vk.vkBindBufferMemory(ctx->device, buffer->handle, buffer->memory, 0);

	if (data) {
		void* mapped;
		ctx->vk.vkMapMemory(ctx->device, buffer->memory, 0, size, 0, &mapped);
		memcpy(mapped, data, size);
		ctx->vk.vkUnmapMemory(ctx->device, buffer->memory);
	}

	buffer->size = size;
	buffer->active = true;
	return (sbgl_Buffer)(index + 1);
}

void sbgl_gfx_DestroyBuffer(sbgl_GfxContext* ctx, sbgl_Buffer handle) {
	if (handle == SBGL_INVALID_HANDLE)
		return;
	uint32_t index = (uint32_t)handle - 1;
	if (index >= SBGL_MAX_BUFFERS || !ctx->buffers[index].active)
		return;

	ctx->vk.vkDestroyBuffer(ctx->device, ctx->buffers[index].handle, NULL);
	ctx->vk.vkFreeMemory(ctx->device, ctx->buffers[index].memory, NULL);
	ctx->buffers[index].active = false;
}

sbgl_Shader sbgl_gfx_LoadShader(sbgl_GfxContext* ctx, sbgl_ShaderStage stage, const uint32_t* bytecode, size_t size) {
	(void)stage;
	uint32_t index = 0;
	for (; index < SBGL_MAX_SHADERS; index++) {
		if (!ctx->shaders[index].active)
			break;
	}
	if (index == SBGL_MAX_SHADERS)
		return SBGL_INVALID_HANDLE;

	VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = size,
		.pCode = bytecode,
	};

	if (ctx->vk.vkCreateShaderModule(ctx->device, &createInfo, NULL, &ctx->shaders[index].module) !=
		VK_SUCCESS) {
		return SBGL_INVALID_HANDLE;
	}

	ctx->shaders[index].active = true;
	return (sbgl_Shader)(index + 1);
}

void sbgl_gfx_DestroyShader(sbgl_GfxContext* ctx, sbgl_Shader handle) {
	if (handle == SBGL_INVALID_HANDLE)
		return;
	uint32_t index = (uint32_t)handle - 1;
	if (index >= SBGL_MAX_SHADERS || !ctx->shaders[index].active)
		return;

	ctx->vk.vkDestroyShaderModule(ctx->device, ctx->shaders[index].module, NULL);
	ctx->shaders[index].active = false;
}

sbgl_Pipeline sbgl_gfx_CreatePipeline(sbgl_GfxContext* ctx, const sbgl_PipelineConfig* config) {
	uint32_t index = 0;
	for (; index < SBGL_MAX_PIPELINES; index++) {
		if (!ctx->pipelines[index].active)
			break;
	}
	if (index == SBGL_MAX_PIPELINES)
		return SBGL_INVALID_HANDLE;

	VkPipelineShaderStageCreateInfo shaderStages[2] = { 0 };
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = ctx->shaders[config->vertexShader - 1].module;
	shaderStages[0].pName = "main";

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = ctx->shaders[config->fragmentShader - 1].module;
	shaderStages[1].pName = "main";

	VkVertexInputBindingDescription bindingDescription = {
		.binding = 0,
		.stride = config->vertexLayout.stride,
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};

	SblArenaMark mark = sbl_arena_mark(ctx->arena);
	VkVertexInputAttributeDescription* attributeDescriptions = SBL_ARENA_PUSH_ARRAY(
		ctx->arena,
		VkVertexInputAttributeDescription,
		config->vertexLayout.attributeCount
	);
	if (!attributeDescriptions && config->vertexLayout.attributeCount > 0) {
		return SBGL_INVALID_HANDLE;
	}
	for (uint32_t i = 0; i < config->vertexLayout.attributeCount; i++) {
		attributeDescriptions[i].binding = 0;
		attributeDescriptions[i].location = config->vertexLayout.attributes[i].location;
		attributeDescriptions[i].format =
			VK_FORMAT_R32G32B32_SFLOAT; // TODO: Map from attribute config
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
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
	};

	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};

	VkPipelineDepthStencilStateCreateInfo depthStencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
	};

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
						  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
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

	if (ctx->vk.vkCreatePipelineLayout(
			ctx->device,
			&pipelineLayoutInfo,
			NULL,
			&ctx->pipelines[index].layout
		) != VK_SUCCESS) {
		sbl_arena_rewind(ctx->arena, mark);
		return SBGL_INVALID_HANDLE;
	}

	VkPipelineRenderingCreateInfo renderingCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &ctx->swapchainFormat,
		.depthAttachmentFormat = ctx->depthFormat,
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
		.pDepthStencilState = &depthStencil,
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamicState,
		.layout = ctx->pipelines[index].layout,
		.renderPass = VK_NULL_HANDLE,
		.subpass = 0,
	};

	if (ctx->vk.vkCreateGraphicsPipelines(
			ctx->device,
			VK_NULL_HANDLE,
			1,
			&pipelineInfo,
			NULL,
			&ctx->pipelines[index].handle
		) != VK_SUCCESS) {
		ctx->vk.vkDestroyPipelineLayout(ctx->device, ctx->pipelines[index].layout, NULL);
		sbl_arena_rewind(ctx->arena, mark);
		return SBGL_INVALID_HANDLE;
	}

	sbl_arena_rewind(ctx->arena, mark);
	ctx->pipelines[index].active = true;
	return (sbgl_Pipeline)(index + 1);
}

void sbgl_gfx_DestroyPipeline(sbgl_GfxContext* ctx, sbgl_Pipeline handle) {
	if (handle == SBGL_INVALID_HANDLE)
		return;
	uint32_t index = (uint32_t)handle - 1;
	if (index >= SBGL_MAX_PIPELINES || !ctx->pipelines[index].active)
		return;

	ctx->vk.vkDestroyPipeline(ctx->device, ctx->pipelines[index].handle, NULL);
	ctx->vk.vkDestroyPipelineLayout(ctx->device, ctx->pipelines[index].layout, NULL);
	ctx->pipelines[index].active = false;
}

void sbgl_gfx_BindPipeline(sbgl_GfxContext* ctx, sbgl_Pipeline handle) {
	if (handle == SBGL_INVALID_HANDLE)
		return;
	uint32_t index = (uint32_t)handle - 1;
	if (index >= SBGL_MAX_PIPELINES || !ctx->pipelines[index].active)
		return;

	ctx->vk.vkCmdBindPipeline(
		ctx->commandBuffers[ctx->currentFrame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		ctx->pipelines[index].handle
	);
	ctx->boundPipeline = handle;

	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)ctx->swapchainExtent.width,
		.height = (float)ctx->swapchainExtent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	ctx->vk.vkCmdSetViewport(ctx->commandBuffers[ctx->currentFrame], 0, 1, &viewport);

	VkRect2D scissor = { .offset = { 0, 0 }, .extent = ctx->swapchainExtent };
	ctx->vk.vkCmdSetScissor(ctx->commandBuffers[ctx->currentFrame], 0, 1, &scissor);
}

void sbgl_gfx_BindBuffer(sbgl_GfxContext* ctx, sbgl_Buffer handle, sbgl_BufferUsage usage) {
	if (handle == SBGL_INVALID_HANDLE)
		return;
	uint32_t index = (uint32_t)handle - 1;
	if (index >= SBGL_MAX_BUFFERS || !ctx->buffers[index].active)
		return;

	if (usage == SBGL_BUFFER_USAGE_VERTEX) {
		VkDeviceSize offsets[] = { 0 };
		ctx->vk.vkCmdBindVertexBuffers(
			ctx->commandBuffers[ctx->currentFrame],
			0,
			1,
			&ctx->buffers[index].handle,
			offsets
		);
	} else if (usage == SBGL_BUFFER_USAGE_INDEX) {
		ctx->vk.vkCmdBindIndexBuffer(
			ctx->commandBuffers[ctx->currentFrame],
			ctx->buffers[index].handle,
			0,
			VK_INDEX_TYPE_UINT32
		);
	}
}

void sbgl_gfx_Draw(sbgl_GfxContext* ctx, uint32_t vertexCount, uint32_t firstVertex) {
	ctx->vk.vkCmdDraw(ctx->commandBuffers[ctx->currentFrame], vertexCount, 1, firstVertex, 0);
}

void sbgl_gfx_DrawIndexed(sbgl_GfxContext* ctx, uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset) {
	ctx->vk.vkCmdDrawIndexed(
		ctx->commandBuffers[ctx->currentFrame],
		indexCount,
		1,
		firstIndex,
		vertexOffset,
		0
	);
}

void sbgl_gfx_PushConstants(sbgl_GfxContext* ctx, size_t size, const void* data) {
	if (ctx->boundPipeline == SBGL_INVALID_HANDLE)
		return;
	uint32_t index = (uint32_t)ctx->boundPipeline - 1;
	ctx->vk.vkCmdPushConstants(
		ctx->commandBuffers[ctx->currentFrame],
		ctx->pipelines[index].layout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		(uint32_t)size,
		data
	);
}
