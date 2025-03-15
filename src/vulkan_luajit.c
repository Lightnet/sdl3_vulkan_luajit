#include "vulkan_luajit.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdio.h>

typedef struct {
    VkInstance instance;
} VulkanInstance;

typedef struct {
    VkSurfaceKHR surface;
    VkInstance instance;
} VulkanSurface;

typedef struct {
    SDL_Window *window;
} SDL3Window;

typedef struct {
    VkDevice device;
} VulkanDevice;

typedef struct {
    VkSwapchainKHR swapchain;
    VkDevice device;
} VulkanSwapchain;

typedef struct {
    VkRenderPass renderPass;
    VkDevice device;
} VulkanRenderPass;

typedef struct {
  VkFramebuffer framebuffer;
  VkImageView imageView;  // Add this to keep the image view alive
  VkDevice device;
} VulkanFramebuffer;

typedef struct {
    VkShaderModule shaderModule;
    VkDevice device;
} VulkanShaderModule;

typedef struct {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout; 
    VkDevice device;
} VulkanPipeline;

typedef struct {
    VkCommandPool commandPool;
    VkDevice device;
} VulkanCommandPool;

typedef struct {
    VkCommandBuffer commandBuffer;
    VkDevice device;
} VulkanCommandBuffer;

typedef struct {
    VkSemaphore semaphore;
    VkDevice device;
} VulkanSemaphore;

typedef struct {
  VkFence fence;
  VkDevice device;
} VulkanFence;

static int l_vulkan_VK_CreateInstanceHelper(lua_State *L) {
    SDL3Window *window_ptr = (SDL3Window *)luaL_checkudata(L, 1, "SDL3Window");
    const char *app_name = luaL_checkstring(L, 2);
    const char *engine_name = luaL_checkstring(L, 3);

    Uint32 extensionCount = 0;
    const char *const *extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if (!extensionNames) {
        lua_pushnil(L);
        lua_pushstring(L, SDL_GetError());
        return 2;
    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = app_name,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = engine_name,
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_4
    };

    
    // VkInstanceCreateInfo createInfo = {
    //     .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    //     .pApplicationInfo = &appInfo,
    //     .enabledExtensionCount = extensionCount,
    //     .ppEnabledExtensionNames = extensionNames,
    //     .enabledLayerCount = 0,
    //     .ppEnabledLayerNames = NULL
    // };

    const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = extensionCount,
        .ppEnabledExtensionNames = extensionNames,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = &validationLayerName
    };

    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, NULL, &instance);
    if (result != VK_SUCCESS) {
        lua_pushnil(L);
        switch (result) {
            case VK_ERROR_EXTENSION_NOT_PRESENT: lua_pushstring(L, "Extension not present"); break;
            case VK_ERROR_INCOMPATIBLE_DRIVER: lua_pushstring(L, "Incompatible driver"); break;
            default: lua_pushstring(L, "Failed to create Vulkan instance");
        }
        return 2;
    }

    VulkanInstance *instance_ptr = (VulkanInstance *)lua_newuserdata(L, sizeof(VulkanInstance));
    instance_ptr->instance = instance;
    luaL_getmetatable(L, "VulkanInstance");
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vulkan_VK_DestroyInstance(lua_State *L) {
    VulkanInstance *instance_ptr = (VulkanInstance *)luaL_checkudata(L, 1, "VulkanInstance");
    if (instance_ptr->instance) {
        vkDestroyInstance(instance_ptr->instance, NULL);
        instance_ptr->instance = VK_NULL_HANDLE;
    }
    return 0;
}

static int l_vulkan_SDL_Vulkan_CreateSurface(lua_State *L) {
    SDL3Window *window_ptr = (SDL3Window *)luaL_checkudata(L, 1, "SDL3Window");
    VulkanInstance *instance_ptr = (VulkanInstance *)luaL_checkudata(L, 2, "VulkanInstance");

    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(window_ptr->window, instance_ptr->instance, NULL, &surface)) {
        lua_pushnil(L);
        lua_pushstring(L, SDL_GetError());
        return 2;
    }

    VulkanSurface *surface_ptr = (VulkanSurface *)lua_newuserdata(L, sizeof(VulkanSurface));
    surface_ptr->surface = surface;
    surface_ptr->instance = instance_ptr->instance;
    luaL_getmetatable(L, "VulkanSurface");
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vulkan_VK_DestroySurfaceKHR(lua_State *L) {
    VulkanSurface *surface_ptr = (VulkanSurface *)luaL_checkudata(L, 1, "VulkanSurface");
    if (surface_ptr->surface && surface_ptr->instance) {
        vkDestroySurfaceKHR(surface_ptr->instance, surface_ptr->surface, NULL);
        surface_ptr->surface = VK_NULL_HANDLE;
    }
    return 0;
}

static int l_vulkan_SDL_Vulkan_GetInstanceExtensions(lua_State *L) {
    Uint32 extensionCount = 0;
    const char *const *extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if (!extensionNames) {
        lua_pushnil(L);
        lua_pushstring(L, SDL_GetError());
        return 2;
    }

    lua_newtable(L);
    for (Uint32 i = 0; i < extensionCount; i++) {
        lua_pushinteger(L, i + 1);
        lua_pushstring(L, extensionNames[i]);
        lua_settable(L, -3);
    }
    return 1;
}

static int l_vulkan_VK_EnumeratePhysicalDevices(lua_State *L) {
    VulkanInstance *instance_ptr = (VulkanInstance *)luaL_checkudata(L, 1, "VulkanInstance");

    Uint32 deviceCount = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance_ptr->instance, &deviceCount, NULL);
    if (result != VK_SUCCESS || deviceCount == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to enumerate physical devices or no devices found");
        return 2;
    }

    VkPhysicalDevice *devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    result = vkEnumeratePhysicalDevices(instance_ptr->instance, &deviceCount, devices);
    if (result != VK_SUCCESS) {
        free(devices);
        lua_pushnil(L);
        lua_pushstring(L, "Failed to enumerate physical devices");
        return 2;
    }

    lua_newtable(L);
    for (Uint32 i = 0; i < deviceCount; i++) {
        lua_pushinteger(L, i + 1);
        lua_pushlightuserdata(L, devices[i]);
        lua_settable(L, -3);
    }
    free(devices);
    return 1;
}

static int l_vulkan_VK_GetPhysicalDeviceQueueFamilyProperties(lua_State *L) {
    VkPhysicalDevice device = (VkPhysicalDevice)lua_touserdata(L, 1);

    Uint32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
    if (queueFamilyCount == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "No queue families found");
        return 2;
    }

    VkQueueFamilyProperties *queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    lua_newtable(L);
    for (Uint32 i = 0; i < queueFamilyCount; i++) {
        lua_pushinteger(L, i + 1);
        lua_newtable(L);
        lua_pushstring(L, "queueFlags");
        lua_pushinteger(L, queueFamilies[i].queueFlags);
        lua_settable(L, -3);
        lua_pushstring(L, "queueCount");
        lua_pushinteger(L, queueFamilies[i].queueCount);
        lua_settable(L, -3);
        lua_settable(L, -3);
    }
    free(queueFamilies);
    return 1;
}

static int l_vulkan_VK_CreateDevice(lua_State *L) {
    VkPhysicalDevice physicalDevice = (VkPhysicalDevice)lua_touserdata(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    lua_getfield(L, 2, "queueFamilyIndex");
    uint32_t queueFamilyIndex = (uint32_t)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, 2, "queueCount");
    uint32_t queueCount = (uint32_t)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    float *queuePriorities = malloc(queueCount * sizeof(float));
    for (uint32_t i = 0; i < queueCount; i++) {
        queuePriorities[i] = 1.0f;
    }

    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndex,
        .queueCount = queueCount,
        .pQueuePriorities = queuePriorities
    };

    const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = deviceExtensions
    };

    VkDevice device;
    VkResult result = vkCreateDevice(physicalDevice, &createInfo, NULL, &device);
    free(queuePriorities);
    if (result != VK_SUCCESS) {
        lua_pushnil(L);
        switch (result) {
            case VK_ERROR_DEVICE_LOST: lua_pushstring(L, "Device lost"); break;
            default: lua_pushstring(L, "Failed to create Vulkan device");
        }
        return 2;
    }

    VulkanDevice *device_ptr = (VulkanDevice *)lua_newuserdata(L, sizeof(VulkanDevice));
    device_ptr->device = device;
    luaL_getmetatable(L, "VulkanDevice");
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vulkan_VK_DestroyDevice(lua_State *L) {
    VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
    if (device_ptr->device) {
        vkDestroyDevice(device_ptr->device, NULL);
        device_ptr->device = VK_NULL_HANDLE;
    }
    return 0;
}

static int l_vulkan_VK_CreateSwapchainKHR(lua_State *L) {
    VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
    VulkanSurface *surface_ptr = (VulkanSurface *)luaL_checkudata(L, 2, "VulkanSurface");
    SDL3Window *window_ptr = (SDL3Window *)luaL_checkudata(L, 3, "SDL3Window");

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR((VkPhysicalDevice)lua_touserdata(L, 4), surface_ptr->surface, &capabilities);

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface_ptr->surface,
        .minImageCount = 2,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    VkSwapchainKHR swapchain;
    VkResult result = vkCreateSwapchainKHR(device_ptr->device, &createInfo, NULL, &swapchain);
    if (result != VK_SUCCESS) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to create swapchain");
        return 2;
    }

    VulkanSwapchain *swapchain_ptr = (VulkanSwapchain *)lua_newuserdata(L, sizeof(VulkanSwapchain));
    swapchain_ptr->swapchain = swapchain;
    swapchain_ptr->device = device_ptr->device;
    luaL_getmetatable(L, "VulkanSwapchain");
    lua_setmetatable(L, -2);

    lua_newtable(L);
    lua_pushinteger(L, capabilities.currentExtent.width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, capabilities.currentExtent.height);
    lua_setfield(L, -2, "height");
    return 2;
}

static int l_vulkan_VK_DestroySwapchainKHR(lua_State *L) {
    VulkanSwapchain *swapchain_ptr = (VulkanSwapchain *)luaL_checkudata(L, 1, "VulkanSwapchain");
    if (swapchain_ptr->swapchain && swapchain_ptr->device) {
        vkDestroySwapchainKHR(swapchain_ptr->device, swapchain_ptr->swapchain, NULL);
        swapchain_ptr->swapchain = VK_NULL_HANDLE;
    }
    return 0;
}

static int l_vulkan_VK_GetDeviceQueue(lua_State *L) {
    VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
    uint32_t queueFamilyIndex = (uint32_t)luaL_checkinteger(L, 2);
    uint32_t queueIndex = (uint32_t)luaL_checkinteger(L, 3);

    VkQueue queue;
    vkGetDeviceQueue(device_ptr->device, queueFamilyIndex, queueIndex, &queue);
    if (!queue) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to get device queue");
        return 2;
    }

    lua_pushlightuserdata(L, queue);
    return 1;
}

static int l_vulkan_VK_GetSwapchainImagesKHR(lua_State *L) {
    VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
    VulkanSwapchain *swapchain_ptr = (VulkanSwapchain *)luaL_checkudata(L, 2, "VulkanSwapchain");

    uint32_t imageCount = 0;
    VkResult result = vkGetSwapchainImagesKHR(device_ptr->device, swapchain_ptr->swapchain, &imageCount, NULL);
    if (result != VK_SUCCESS || imageCount == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to get swapchain image count");
        return 2;
    }

    VkImage *images = malloc(imageCount * sizeof(VkImage));
    result = vkGetSwapchainImagesKHR(device_ptr->device, swapchain_ptr->swapchain, &imageCount, images);
    if (result != VK_SUCCESS) {
        free(images);
        lua_pushnil(L);
        lua_pushstring(L, "Failed to get swapchain images");
        return 2;
    }

    lua_newtable(L);
    for (uint32_t i = 0; i < imageCount; i++) {
        lua_pushinteger(L, i + 1);
        lua_pushlightuserdata(L, images[i]);
        lua_settable(L, -3);
    }
    free(images);
    return 1;
}

static int l_vulkan_VK_CreateRenderPass(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VkAttachmentDescription colorAttachment = {
      .format = VK_FORMAT_B8G8R8A8_UNORM,  // Match swapchain format
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  };
  VkAttachmentReference colorAttachmentRef = {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  };
  VkSubpassDescription subpass = {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentRef
  };
  VkRenderPassCreateInfo renderPassInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &colorAttachment,
      .subpassCount = 1,
      .pSubpasses = &subpass
  };
  VkRenderPass renderPass;
  VkResult result = vkCreateRenderPass(device_ptr->device, &renderPassInfo, NULL, &renderPass);
  if (result != VK_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create render pass");
      return 2;
  }
  VulkanRenderPass *renderPass_ptr = (VulkanRenderPass *)lua_newuserdata(L, sizeof(VulkanRenderPass));
  renderPass_ptr->renderPass = renderPass;
  renderPass_ptr->device = device_ptr->device;
  luaL_getmetatable(L, "VulkanRenderPass");
  lua_setmetatable(L, -2);
  return 1;
}

static int l_vulkan_VK_DestroyRenderPass(lua_State *L) {
    VulkanRenderPass *renderPass_ptr = (VulkanRenderPass *)luaL_checkudata(L, 1, "VulkanRenderPass");
    if (renderPass_ptr->renderPass && renderPass_ptr->device) {
        vkDestroyRenderPass(renderPass_ptr->device, renderPass_ptr->renderPass, NULL);
        renderPass_ptr->renderPass = VK_NULL_HANDLE;
    }
    return 0;
}

static int l_vulkan_VK_CreateFramebuffer(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanRenderPass *renderPass_ptr = (VulkanRenderPass *)luaL_checkudata(L, 2, "VulkanRenderPass");
  VkImage image = (VkImage)lua_touserdata(L, 3);
  luaL_checktype(L, 4, LUA_TTABLE);

  lua_getfield(L, 4, "width");
  uint32_t width = (uint32_t)luaL_checkinteger(L, -1);
  lua_pop(L, 1);
  lua_getfield(L, 4, "height");
  uint32_t height = (uint32_t)luaL_checkinteger(L, -1);
  lua_pop(L, 1);

  VkImageViewCreateInfo viewInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_B8G8R8A8_UNORM,
      .components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
      .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1
      }
  };

  VkImageView imageView;
  VkResult result = vkCreateImageView(device_ptr->device, &viewInfo, NULL, &imageView);
  if (result != VK_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create image view");
      return 2;
  }

  VkFramebufferCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = renderPass_ptr->renderPass,
      .attachmentCount = 1,
      .pAttachments = &imageView,
      .width = width,
      .height = height,
      .layers = 1
  };

  VkFramebuffer framebuffer;
  result = vkCreateFramebuffer(device_ptr->device, &createInfo, NULL, &framebuffer);
  if (result != VK_SUCCESS) {
      vkDestroyImageView(device_ptr->device, imageView, NULL);
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create framebuffer");
      return 2;
  }

  VulkanFramebuffer *framebuffer_ptr = (VulkanFramebuffer *)lua_newuserdata(L, sizeof(VulkanFramebuffer));
  framebuffer_ptr->framebuffer = framebuffer;
  framebuffer_ptr->imageView = imageView;  // Store the image view
  framebuffer_ptr->device = device_ptr->device;
  luaL_getmetatable(L, "VulkanFramebuffer");
  lua_setmetatable(L, -2);
  return 1;
}



static int l_vulkan_VK_DestroyFramebuffer(lua_State *L) {
  VulkanFramebuffer *framebuffer_ptr = (VulkanFramebuffer *)luaL_checkudata(L, 1, "VulkanFramebuffer");
  if (framebuffer_ptr->framebuffer && framebuffer_ptr->device) {
      vkDestroyFramebuffer(framebuffer_ptr->device, framebuffer_ptr->framebuffer, NULL);
      framebuffer_ptr->framebuffer = VK_NULL_HANDLE;
  }
  if (framebuffer_ptr->imageView && framebuffer_ptr->device) {
      vkDestroyImageView(framebuffer_ptr->device, framebuffer_ptr->imageView, NULL);
      framebuffer_ptr->imageView = VK_NULL_HANDLE;
  }
  return 0;
}

static int l_vulkan_VK_CreateShaderModule(lua_State *L) {
    VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
    const char *filename = luaL_checkstring(L, 2);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to open shader file");
        return 2;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *code = malloc(size);
    fread(code, 1, size, file);
    fclose(file);

    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = (const uint32_t *)code
    };

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device_ptr->device, &createInfo, NULL, &shaderModule);
    free(code);
    if (result != VK_SUCCESS) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to create shader module");
        return 2;
    }

    VulkanShaderModule *shaderModule_ptr = (VulkanShaderModule *)lua_newuserdata(L, sizeof(VulkanShaderModule));
    shaderModule_ptr->shaderModule = shaderModule;
    shaderModule_ptr->device = device_ptr->device;
    luaL_getmetatable(L, "VulkanShaderModule");
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vulkan_VK_DestroyShaderModule(lua_State *L) {
    VulkanShaderModule *shaderModule_ptr = (VulkanShaderModule *)luaL_checkudata(L, 1, "VulkanShaderModule");
    if (shaderModule_ptr->shaderModule && shaderModule_ptr->device) {
        vkDestroyShaderModule(shaderModule_ptr->device, shaderModule_ptr->shaderModule, NULL);
        shaderModule_ptr->shaderModule = VK_NULL_HANDLE;
    }
    return 0;
}


static int l_vulkan_VK_CreatePipelineLayout(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");

  VkPipelineLayoutCreateInfo layoutInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,          // No descriptor sets
      .pSetLayouts = NULL,
      .pushConstantRangeCount = 0,  // No push constants
      .pPushConstantRanges = NULL
  };

  VkPipelineLayout pipelineLayout;
  VkResult result = vkCreatePipelineLayout(device_ptr->device, &layoutInfo, NULL, &pipelineLayout);
  if (result != VK_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create pipeline layout");
      return 2;
  }

  VulkanPipeline *pipeline_ptr = (VulkanPipeline *)lua_newuserdata(L, sizeof(VulkanPipeline));
  pipeline_ptr->pipeline = VK_NULL_HANDLE;  // Pipeline not created yet
  pipeline_ptr->pipelineLayout = pipelineLayout;
  pipeline_ptr->device = device_ptr->device;
  luaL_getmetatable(L, "VulkanPipeline");
  lua_setmetatable(L, -2);
  return 1;
}




static int l_vulkan_VK_CreateGraphicsPipelines(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanShaderModule *vertShader_ptr = (VulkanShaderModule *)luaL_checkudata(L, 2, "VulkanShaderModule");
  VulkanShaderModule *fragShader_ptr = (VulkanShaderModule *)luaL_checkudata(L, 3, "VulkanShaderModule");
  VulkanRenderPass *renderPass_ptr = (VulkanRenderPass *)luaL_checkudata(L, 4, "VulkanRenderPass");
  luaL_checktype(L, 5, LUA_TTABLE);

  lua_getfield(L, 5, "width");
  uint32_t width = (uint32_t)luaL_checkinteger(L, -1);
  lua_pop(L, 1);
  lua_getfield(L, 5, "height");
  uint32_t height = (uint32_t)luaL_checkinteger(L, -1);
  lua_pop(L, 1);

  // Create a pipeline layout
  VkPipelineLayoutCreateInfo layoutInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pSetLayouts = NULL,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = NULL
  };
  VkPipelineLayout pipelineLayout;
  VkResult result = vkCreatePipelineLayout(device_ptr->device, &layoutInfo, NULL, &pipelineLayout);
  if (result != VK_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create pipeline layout");
      return 2;
  }

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vertShader_ptr->shaderModule,
      .pName = "main"
  };
  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = fragShader_ptr->shaderModule,
      .pName = "main"
  };
  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 0,
      .vertexAttributeDescriptionCount = 0
  };
  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE
  };
  VkViewport viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
  VkRect2D scissor = {{0, 0}, {width, height}};
  VkPipelineViewportStateCreateInfo viewportState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor
  };
  VkPipelineRasterizationStateCreateInfo rasterizer = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .lineWidth = 1.0f,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE
  };
  VkPipelineMultisampleStateCreateInfo multisampling = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .sampleShadingEnable = VK_FALSE,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
  };
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable = VK_FALSE
  };
  VkPipelineColorBlendStateCreateInfo colorBlending = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment
  };
  VkGraphicsPipelineCreateInfo pipelineInfo = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = shaderStages,
      .pVertexInputState = &vertexInputInfo,
      .pInputAssemblyState = &inputAssembly,
      .pViewportState = &viewportState,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pColorBlendState = &colorBlending,
      .layout = pipelineLayout,  // Set the pipeline layout
      .renderPass = renderPass_ptr->renderPass,
      .subpass = 0
  };

  VkPipeline pipeline;
  result = vkCreateGraphicsPipelines(device_ptr->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline);
  if (result != VK_SUCCESS) {
      vkDestroyPipelineLayout(device_ptr->device, pipelineLayout, NULL);
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create graphics pipeline");
      return 2;
  }

  VulkanPipeline *pipeline_ptr = (VulkanPipeline *)lua_newuserdata(L, sizeof(VulkanPipeline));
  pipeline_ptr->pipeline = pipeline;
  pipeline_ptr->pipelineLayout = pipelineLayout;  // Store the layout
  pipeline_ptr->device = device_ptr->device;
  luaL_getmetatable(L, "VulkanPipeline");
  lua_setmetatable(L, -2);
  return 1;
}


static int l_vulkan_VK_DestroyPipeline(lua_State *L) {
  VulkanPipeline *pipeline_ptr = (VulkanPipeline *)luaL_checkudata(L, 1, "VulkanPipeline");
  if (pipeline_ptr->pipeline && pipeline_ptr->device) {
      vkDestroyPipeline(pipeline_ptr->device, pipeline_ptr->pipeline, NULL);
      pipeline_ptr->pipeline = VK_NULL_HANDLE;
  }
  if (pipeline_ptr->pipelineLayout && pipeline_ptr->device) {
      vkDestroyPipelineLayout(pipeline_ptr->device, pipeline_ptr->pipelineLayout, NULL);
      pipeline_ptr->pipelineLayout = VK_NULL_HANDLE;
  }
  return 0;
}





static int l_vulkan_VK_CreateCommandPool(lua_State *L) {
    VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
    uint32_t queueFamilyIndex = (uint32_t)luaL_checkinteger(L, 2);

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndex,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    };

    VkCommandPool commandPool;
    VkResult result = vkCreateCommandPool(device_ptr->device, &poolInfo, NULL, &commandPool);
    if (result != VK_SUCCESS) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to create command pool");
        return 2;
    }

    VulkanCommandPool *pool_ptr = (VulkanCommandPool *)lua_newuserdata(L, sizeof(VulkanCommandPool));
    pool_ptr->commandPool = commandPool;
    pool_ptr->device = device_ptr->device;
    luaL_getmetatable(L, "VulkanCommandPool");
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vulkan_VK_DestroyCommandPool(lua_State *L) {
    VulkanCommandPool *pool_ptr = (VulkanCommandPool *)luaL_checkudata(L, 1, "VulkanCommandPool");
    if (pool_ptr->commandPool && pool_ptr->device) {
        vkDestroyCommandPool(pool_ptr->device, pool_ptr->commandPool, NULL);
        pool_ptr->commandPool = VK_NULL_HANDLE;
    }
    return 0;
}

static int l_vulkan_VK_AllocateCommandBuffers(lua_State *L) {
    VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
    VulkanCommandPool *pool_ptr = (VulkanCommandPool *)luaL_checkudata(L, 2, "VulkanCommandPool");
    uint32_t count = (uint32_t)luaL_checkinteger(L, 3);

    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool_ptr->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = count
    };

    VkCommandBuffer *commandBuffers = malloc(count * sizeof(VkCommandBuffer));
    VkResult result = vkAllocateCommandBuffers(device_ptr->device, &allocInfo, commandBuffers);
    if (result != VK_SUCCESS) {
        free(commandBuffers);
        lua_pushnil(L);
        lua_pushstring(L, "Failed to allocate command buffers");
        return 2;
    }

    lua_newtable(L);
    for (uint32_t i = 0; i < count; i++) {
        VulkanCommandBuffer *cmd_ptr = (VulkanCommandBuffer *)lua_newuserdata(L, sizeof(VulkanCommandBuffer));
        cmd_ptr->commandBuffer = commandBuffers[i];
        cmd_ptr->device = device_ptr->device;
        luaL_getmetatable(L, "VulkanCommandBuffer");
        lua_setmetatable(L, -2);
        lua_rawseti(L, -2, i + 1);
    }
    free(commandBuffers);
    return 1;
}

static int l_vulkan_VK_BeginCommandBuffer(lua_State *L) {
  VulkanCommandBuffer *cmd_ptr = (VulkanCommandBuffer *)luaL_checkudata(L, 1, "VulkanCommandBuffer");

  if (!cmd_ptr->commandBuffer) {
      lua_pushboolean(L, false);
      lua_pushstring(L, "Command buffer is null");
      return 2;
  }

  VkCommandBufferBeginInfo beginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
  };

  VkResult result = vkBeginCommandBuffer(cmd_ptr->commandBuffer, &beginInfo);
  if (result != VK_SUCCESS) {
      lua_pushboolean(L, false);
      switch (result) {
          case VK_ERROR_OUT_OF_HOST_MEMORY: lua_pushstring(L, "Out of host memory"); break;
          case VK_ERROR_OUT_OF_DEVICE_MEMORY: lua_pushstring(L, "Out of device memory"); break;
          default: lua_pushstring(L, "Failed to begin command buffer");
      }
      return 2;
  }

  lua_pushboolean(L, true);
  return 1;
}



static int l_vulkan_VK_EndCommandBuffer(lua_State *L) {
  VulkanCommandBuffer *cmd_ptr = (VulkanCommandBuffer *)luaL_checkudata(L, 1, "VulkanCommandBuffer");

  if (!cmd_ptr->commandBuffer) {
      lua_pushboolean(L, false);
      lua_pushstring(L, "Command buffer is null");
      return 2;
  }

  VkResult result = vkEndCommandBuffer(cmd_ptr->commandBuffer);
  if (result != VK_SUCCESS) {
      lua_pushboolean(L, false);
      switch (result) {
          case VK_ERROR_OUT_OF_HOST_MEMORY: lua_pushstring(L, "Out of host memory"); break;
          case VK_ERROR_OUT_OF_DEVICE_MEMORY: lua_pushstring(L, "Out of device memory"); break;
          default: lua_pushstring(L, "Failed to end command buffer");
      }
      return 2;
  }

  lua_pushboolean(L, true);
  return 1;
}


static int l_vulkan_VK_CmdBeginRenderPass(lua_State *L) {
  VulkanCommandBuffer *cmd_ptr = (VulkanCommandBuffer *)luaL_checkudata(L, 1, "VulkanCommandBuffer");
  VulkanRenderPass *renderPass_ptr = (VulkanRenderPass *)luaL_checkudata(L, 2, "VulkanRenderPass");
  VulkanFramebuffer *framebuffer_ptr = (VulkanFramebuffer *)luaL_checkudata(L, 3, "VulkanFramebuffer");
  luaL_checktype(L, 4, LUA_TTABLE);

  if (!cmd_ptr->commandBuffer || !renderPass_ptr->renderPass || !framebuffer_ptr->framebuffer) {
      lua_pushnil(L);
      lua_pushstring(L, "Invalid command buffer, render pass, or framebuffer");
      return 2;
  }

  lua_getfield(L, 4, "width");
  uint32_t width = (uint32_t)luaL_checkinteger(L, -1);
  lua_pop(L, 1);
  lua_getfield(L, 4, "height");
  uint32_t height = (uint32_t)luaL_checkinteger(L, -1);
  lua_pop(L, 1);

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  VkRenderPassBeginInfo renderPassInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = renderPass_ptr->renderPass,
      .framebuffer = framebuffer_ptr->framebuffer,
      .renderArea = {{0, 0}, {width, height}},
      .clearValueCount = 1,
      .pClearValues = &clearColor
  };

  vkCmdBeginRenderPass(cmd_ptr->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  return 0;
}


static int l_vulkan_VK_CmdEndRenderPass(lua_State *L) {
  VulkanCommandBuffer *cmd_ptr = (VulkanCommandBuffer *)luaL_checkudata(L, 1, "VulkanCommandBuffer");

  if (!cmd_ptr->commandBuffer) {
      lua_pushnil(L);
      lua_pushstring(L, "Invalid command buffer");
      return 2;
  }

  vkCmdEndRenderPass(cmd_ptr->commandBuffer);
  return 0;
}

static int l_vulkan_VK_CmdBindPipeline(lua_State *L) {
  VulkanCommandBuffer *cmd_ptr = (VulkanCommandBuffer *)luaL_checkudata(L, 1, "VulkanCommandBuffer");
  VulkanPipeline *pipeline_ptr = (VulkanPipeline *)luaL_checkudata(L, 2, "VulkanPipeline");

  if (!cmd_ptr->commandBuffer || !pipeline_ptr->pipeline) {
      lua_pushnil(L);
      lua_pushstring(L, "Invalid command buffer or pipeline");
      return 2;
  }

  vkCmdBindPipeline(cmd_ptr->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_ptr->pipeline);
  return 0;
}

static int l_vulkan_VK_CmdDraw(lua_State *L) {
  VulkanCommandBuffer *cmd_ptr = (VulkanCommandBuffer *)luaL_checkudata(L, 1, "VulkanCommandBuffer");
  uint32_t vertexCount = (uint32_t)luaL_checkinteger(L, 2);
  uint32_t instanceCount = (uint32_t)luaL_checkinteger(L, 3);
  uint32_t firstVertex = (uint32_t)luaL_checkinteger(L, 4);
  uint32_t firstInstance = (uint32_t)luaL_checkinteger(L, 5);

  if (!cmd_ptr->commandBuffer) {
      lua_pushnil(L);
      lua_pushstring(L, "Invalid command buffer");
      return 2;
  }

  vkCmdDraw(cmd_ptr->commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
  return 0;
}

static int l_vulkan_VK_CreateSemaphore(lua_State *L) {
    VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");

    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkSemaphore semaphore;
    VkResult result = vkCreateSemaphore(device_ptr->device, &semaphoreInfo, NULL, &semaphore);
    if (result != VK_SUCCESS) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to create semaphore");
        return 2;
    }

    VulkanSemaphore *semaphore_ptr = (VulkanSemaphore *)lua_newuserdata(L, sizeof(VulkanSemaphore));
    semaphore_ptr->semaphore = semaphore;
    semaphore_ptr->device = device_ptr->device;
    luaL_getmetatable(L, "VulkanSemaphore");
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vulkan_VK_DestroySemaphore(lua_State *L) {
    VulkanSemaphore *semaphore_ptr = (VulkanSemaphore *)luaL_checkudata(L, 1, "VulkanSemaphore");
    if (semaphore_ptr->semaphore && semaphore_ptr->device) {
        vkDestroySemaphore(semaphore_ptr->device, semaphore_ptr->semaphore, NULL);
        semaphore_ptr->semaphore = VK_NULL_HANDLE;
    }
    return 0;
}

static int l_vulkan_VK_AcquireNextImageKHR(lua_State *L) {
    VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
    VulkanSwapchain *swapchain_ptr = (VulkanSwapchain *)luaL_checkudata(L, 2, "VulkanSwapchain");
    VulkanSemaphore *semaphore_ptr = (VulkanSemaphore *)luaL_checkudata(L, 3, "VulkanSemaphore");

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device_ptr->device, swapchain_ptr->swapchain, UINT64_MAX, semaphore_ptr->semaphore, VK_NULL_HANDLE, &imageIndex);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to acquire next image");
        return 2;
    }

    lua_pushinteger(L, imageIndex + 1);  // Lua indices start at 1
    return 1;
}

// Add this helper if not already present (for older LuaJIT compatibility)
#if !defined(luaL_testudata)
void *luaL_testudata(lua_State *L, int ud, const char *tname) {
    void *p = lua_touserdata(L, ud);
    if (p != NULL && lua_getmetatable(L, ud)) {
        luaL_getmetatable(L, tname);
        if (!lua_rawequal(L, -1, -2)) p = NULL;
        lua_pop(L, 2);
    }
    return p;
}
#endif

static int l_vulkan_VK_QueueSubmit(lua_State *L) {
  VkQueue queue = (VkQueue)lua_touserdata(L, 1);
  VulkanCommandBuffer *cmd_ptr = (VulkanCommandBuffer *)luaL_checkudata(L, 2, "VulkanCommandBuffer");
  VulkanSemaphore *waitSemaphore_ptr = (VulkanSemaphore *)luaL_checkudata(L, 3, "VulkanSemaphore");
  VulkanSemaphore *signalSemaphore_ptr = (VulkanSemaphore *)luaL_checkudata(L, 4, "VulkanSemaphore");
  VulkanFence *fence_ptr = NULL;

  // Check if 5th argument is a VulkanFence userdata or nil
  if (lua_gettop(L) >= 5 && !lua_isnil(L, 5)) {
      fence_ptr = (VulkanFence *)luaL_testudata(L, 5, "VulkanFence");
      if (!fence_ptr) {
          lua_pushboolean(L, false);
          lua_pushstring(L, "Argument 5 must be a VulkanFence or nil");
          return 2;
      }
  }

  VkSemaphore waitSemaphores[] = {waitSemaphore_ptr->semaphore};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signalSemaphores[] = {signalSemaphore_ptr->semaphore};

  VkSubmitInfo submitInfo = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = waitSemaphores,
      .pWaitDstStageMask = waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd_ptr->commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = signalSemaphores
  };

  VkResult result = vkQueueSubmit(queue, 1, &submitInfo, fence_ptr ? fence_ptr->fence : VK_NULL_HANDLE);
  if (result != VK_SUCCESS) {
      lua_pushboolean(L, false);
      lua_pushstring(L, "Failed to submit queue");
      return 2;
  }

  lua_pushboolean(L, true);
  return 1;
}



static int l_vulkan_VK_QueuePresentKHR(lua_State *L) {
    VkQueue queue = (VkQueue)lua_touserdata(L, 1);
    VulkanSwapchain *swapchain_ptr = (VulkanSwapchain *)luaL_checkudata(L, 2, "VulkanSwapchain");
    uint32_t imageIndex = (uint32_t)luaL_checkinteger(L, 3) - 1;  // Lua indices start at 1
    VulkanSemaphore *semaphore_ptr = (VulkanSemaphore *)luaL_checkudata(L, 4, "VulkanSemaphore");

    VkSemaphore semaphores[] = {semaphore_ptr->semaphore};
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = semaphores,
        .swapchainCount = 1,
        .pSwapchains = &swapchain_ptr->swapchain,
        .pImageIndices = &imageIndex
    };

    VkResult result = vkQueuePresentKHR(queue, &presentInfo);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        lua_pushboolean(L, false);
        lua_pushstring(L, "Failed to present queue");
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

static int l_vulkan_VK_QueueWaitIdle(lua_State *L) {
    VkQueue queue = (VkQueue)lua_touserdata(L, 1);
    VkResult result = vkQueueWaitIdle(queue);
    if (result != VK_SUCCESS) {
        lua_pushboolean(L, false);
        lua_pushstring(L, "Failed to wait for queue idle");
        return 2;
    }
    lua_pushboolean(L, true);
    return 1;
}

// New fence creation function
static int l_vulkan_VK_CreateFence(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");

  VkFenceCreateInfo fenceInfo = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT
  };

  VkFence fence;
  VkResult result = vkCreateFence(device_ptr->device, &fenceInfo, NULL, &fence);
  if (result != VK_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create fence");
      return 2;
  }

  VulkanFence *fence_ptr = (VulkanFence *)lua_newuserdata(L, sizeof(VulkanFence));
  fence_ptr->fence = fence;
  fence_ptr->device = device_ptr->device;
  luaL_getmetatable(L, "VulkanFence");
  lua_setmetatable(L, -2);
  return 1;
}

// New fence destruction function
static int l_vulkan_VK_DestroyFence(lua_State *L) {
  VulkanFence *fence_ptr = (VulkanFence *)luaL_checkudata(L, 1, "VulkanFence");
  if (fence_ptr->fence && fence_ptr->device) {
      vkDestroyFence(fence_ptr->device, fence_ptr->fence, NULL);
      fence_ptr->fence = VK_NULL_HANDLE;
  }
  return 0;
}

// New wait for fences function
static int l_vulkan_VK_WaitForFences(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanFence *fence_ptr = (VulkanFence *)luaL_checkudata(L, 2, "VulkanFence");
  uint64_t timeout = (uint64_t)luaL_checkinteger(L, 3);

  VkResult result = vkWaitForFences(device_ptr->device, 1, &fence_ptr->fence, VK_TRUE, timeout);
  if (result != VK_SUCCESS) {
      lua_pushboolean(L, false);
      lua_pushstring(L, result == VK_TIMEOUT ? "Fence wait timed out" : "Failed to wait for fence");
      return 2;
  }

  lua_pushboolean(L, true);
  return 1;
}



// New reset fences function
static int l_vulkan_VK_ResetFences(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanFence *fence_ptr = (VulkanFence *)luaL_checkudata(L, 2, "VulkanFence");

  VkResult result = vkResetFences(device_ptr->device, 1, &fence_ptr->fence);
  if (result != VK_SUCCESS) {
      lua_pushboolean(L, false);
      lua_pushstring(L, "Failed to reset fence");
      return 2;
  }

  lua_pushboolean(L, true);
  return 1;
}

static const luaL_Reg vulkan_funcs[] = {
    {"VK_CreateInstanceHelper", l_vulkan_VK_CreateInstanceHelper},
    {"VK_DestroyInstance", l_vulkan_VK_DestroyInstance},
    {"SDL_Vulkan_CreateSurface", l_vulkan_SDL_Vulkan_CreateSurface},
    {"VK_DestroySurfaceKHR", l_vulkan_VK_DestroySurfaceKHR},
    {"SDL_Vulkan_GetInstanceExtensions", l_vulkan_SDL_Vulkan_GetInstanceExtensions},
    {"VK_EnumeratePhysicalDevices", l_vulkan_VK_EnumeratePhysicalDevices},
    {"VK_GetPhysicalDeviceQueueFamilyProperties", l_vulkan_VK_GetPhysicalDeviceQueueFamilyProperties},
    {"VK_CreateDevice", l_vulkan_VK_CreateDevice},
    {"VK_DestroyDevice", l_vulkan_VK_DestroyDevice},
    {"VK_CreateSwapchainKHR", l_vulkan_VK_CreateSwapchainKHR},
    {"VK_DestroySwapchainKHR", l_vulkan_VK_DestroySwapchainKHR},
    {"VK_GetDeviceQueue", l_vulkan_VK_GetDeviceQueue},
    {"VK_GetSwapchainImagesKHR", l_vulkan_VK_GetSwapchainImagesKHR},
    {"VK_CreateRenderPass", l_vulkan_VK_CreateRenderPass},
    {"VK_DestroyRenderPass", l_vulkan_VK_DestroyRenderPass},
    {"VK_CreateFramebuffer", l_vulkan_VK_CreateFramebuffer},
    {"VK_DestroyFramebuffer", l_vulkan_VK_DestroyFramebuffer},
    {"VK_CreateShaderModule", l_vulkan_VK_CreateShaderModule},
    {"VK_DestroyShaderModule", l_vulkan_VK_DestroyShaderModule},
    {"VK_CreatePipelineLayout", l_vulkan_VK_CreatePipelineLayout},
    {"VK_CreateGraphicsPipelines", l_vulkan_VK_CreateGraphicsPipelines},
    {"VK_DestroyPipeline", l_vulkan_VK_DestroyPipeline},
    {"VK_CreateCommandPool", l_vulkan_VK_CreateCommandPool},
    {"VK_DestroyCommandPool", l_vulkan_VK_DestroyCommandPool},
    {"VK_AllocateCommandBuffers", l_vulkan_VK_AllocateCommandBuffers},
    {"VK_BeginCommandBuffer", l_vulkan_VK_BeginCommandBuffer},
    {"VK_EndCommandBuffer", l_vulkan_VK_EndCommandBuffer},
    {"VK_CmdBeginRenderPass", l_vulkan_VK_CmdBeginRenderPass},
    {"VK_CmdEndRenderPass", l_vulkan_VK_CmdEndRenderPass},
    {"VK_CmdBindPipeline", l_vulkan_VK_CmdBindPipeline},
    {"VK_CmdDraw", l_vulkan_VK_CmdDraw},
    {"VK_CreateSemaphore", l_vulkan_VK_CreateSemaphore},
    {"VK_DestroySemaphore", l_vulkan_VK_DestroySemaphore},
    {"VK_AcquireNextImageKHR", l_vulkan_VK_AcquireNextImageKHR},
    {"VK_QueueSubmit", l_vulkan_VK_QueueSubmit},
    {"VK_QueuePresentKHR", l_vulkan_VK_QueuePresentKHR},
    {"VK_QueueWaitIdle", l_vulkan_VK_QueueWaitIdle},
    {"VK_CreateFence", l_vulkan_VK_CreateFence},           // New
    {"VK_DestroyFence", l_vulkan_VK_DestroyFence},         // New
    {"VK_WaitForFences", l_vulkan_VK_WaitForFences},       // New
    {"VK_ResetFences", l_vulkan_VK_ResetFences},           // New
    {NULL, NULL}
};

static const luaL_Reg fence_mt[] = {
  {"__gc", l_vulkan_VK_DestroyFence},
  {NULL, NULL}
};

static const luaL_Reg instance_mt[] = {
    {"__gc", l_vulkan_VK_DestroyInstance},
    {NULL, NULL}
};

static const luaL_Reg surface_mt[] = {
    {NULL, NULL}
};

static const luaL_Reg device_mt[] = {
    {"__gc", l_vulkan_VK_DestroyDevice},
    {NULL, NULL}
};

static const luaL_Reg swapchain_mt[] = {
    {"__gc", l_vulkan_VK_DestroySwapchainKHR},
    {NULL, NULL}
};

static const luaL_Reg renderpass_mt[] = {
    {"__gc", l_vulkan_VK_DestroyRenderPass},
    {NULL, NULL}
};

static const luaL_Reg framebuffer_mt[] = {
    {"__gc", l_vulkan_VK_DestroyFramebuffer},
    {NULL, NULL}
};

static const luaL_Reg shadermodule_mt[] = {
    {"__gc", l_vulkan_VK_DestroyShaderModule},
    {NULL, NULL}
};

static const luaL_Reg pipeline_mt[] = {
    {"__gc", l_vulkan_VK_DestroyPipeline},
    {NULL, NULL}
};

static const luaL_Reg commandpool_mt[] = {
    {"__gc", l_vulkan_VK_DestroyCommandPool},
    {NULL, NULL}
};

static const luaL_Reg commandbuffer_mt[] = {
    {NULL, NULL}  // No __gc, freed with command pool
};

static const luaL_Reg semaphore_mt[] = {
    {"__gc", l_vulkan_VK_DestroySemaphore},
    {NULL, NULL}
};

int luaopen_vulkan(lua_State *L) {
    luaL_newmetatable(L, "VulkanInstance");
    luaL_setfuncs(L, instance_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanSurface");
    luaL_setfuncs(L, surface_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanDevice");
    luaL_setfuncs(L, device_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanSwapchain");
    luaL_setfuncs(L, swapchain_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanRenderPass");
    luaL_setfuncs(L, renderpass_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanFramebuffer");
    luaL_setfuncs(L, framebuffer_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanShaderModule");
    luaL_setfuncs(L, shadermodule_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanPipeline");
    luaL_setfuncs(L, pipeline_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanCommandPool");
    luaL_setfuncs(L, commandpool_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanCommandBuffer");
    luaL_setfuncs(L, commandbuffer_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanSemaphore");
    luaL_setfuncs(L, semaphore_mt, 0);
    lua_pop(L, 1);

    // New fence metatable
    luaL_newmetatable(L, "VulkanFence");
    luaL_setfuncs(L, fence_mt, 0);
    lua_pop(L, 1);

    luaL_newlib(L, vulkan_funcs);
    lua_pushinteger(L, VK_API_VERSION_1_4);
    lua_setfield(L, -2, "API_VERSION_1_4");
    lua_pushinteger(L, VK_QUEUE_GRAPHICS_BIT);
    lua_setfield(L, -2, "QUEUE_GRAPHICS_BIT");
    lua_pushstring(L, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    lua_setfield(L, -2, "KHR_SWAPCHAIN_EXTENSION_NAME");
    return 1;
}