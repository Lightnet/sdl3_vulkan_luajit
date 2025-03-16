#include "vulkan_luajit.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdio.h>

// # instance
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
    VkPhysicalDevice physicalDevice;
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
  VkImageView imageView;
  VkDevice device;
} VulkanFramebuffer;

typedef struct {
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDevice device;
} VulkanBuffer;

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

typedef struct {
  VkDescriptorPool pool;
  VkDevice device;
} VulkanDescriptorPool;

typedef struct {
  VkDescriptorSet set;
  VkDevice device;
  VkDescriptorPool pool;
} VulkanDescriptorSet;


typedef struct {
  VkDescriptorSetLayout layout;
  VkDevice device;  // Optional, for __gc safety
} VulkanDescriptorSetLayout;


static int l_vulkan_VK_DestroyDescriptorPool(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanDescriptorPool *pool_ptr = (VulkanDescriptorPool *)luaL_checkudata(L, 2, "VulkanDescriptorPool");
  printf("l_vulkan_VK_DestroyDescriptorPool: device_ptr=%p, device=%p, pool_ptr=%p, pool=%p\n",
         (void*)device_ptr, (void*)device_ptr->device, (void*)pool_ptr, (void*)pool_ptr->pool);
  if (!device_ptr->device) {
      printf("l_vulkan_VK_DestroyDescriptorPool: Error - device is NULL\n");
      lua_pushstring(L, "Device handle is NULL");
      lua_error(L);
  }
  if (!pool_ptr->pool) {
      printf("l_vulkan_VK_DestroyDescriptorPool: Warning - descriptor pool is already NULL\n");
      return 0;
  }
  vkDestroyDescriptorPool(device_ptr->device, pool_ptr->pool, NULL);
  pool_ptr->pool = VK_NULL_HANDLE;
  printf("l_vulkan_VK_DestroyDescriptorPool: Descriptor pool destroyed successfully\n");
  return 0;
}


static int l_vulkan_VK_AllocateDescriptorSet(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanDescriptorPool *pool_ptr = (VulkanDescriptorPool *)luaL_checkudata(L, 2, "VulkanDescriptorPool");
  VulkanDescriptorSetLayout *layout_ptr = (VulkanDescriptorSetLayout *)luaL_checkudata(L, 3, "VulkanDescriptorSetLayout");
  VkDescriptorSetAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool_ptr->pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &layout_ptr->layout
  };
  VulkanDescriptorSet *set_ptr = (VulkanDescriptorSet *)lua_newuserdata(L, sizeof(VulkanDescriptorSet));
  VkResult result = vkAllocateDescriptorSets(device_ptr->device, &allocInfo, &set_ptr->set);
  if (result != VK_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to allocate descriptor set");
      return 2;
  }
  set_ptr->device = device_ptr->device;
  set_ptr->pool = pool_ptr->pool;  // Store the pool
  luaL_getmetatable(L, "VulkanDescriptorSet");
  lua_setmetatable(L, -2);
  return 1;
}



static int l_vulkan_VK_UpdateDescriptorSet(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanDescriptorSet *set_ptr = (VulkanDescriptorSet *)luaL_checkudata(L, 2, "VulkanDescriptorSet");
  VulkanBuffer *buffer_ptr = (VulkanBuffer *)luaL_checkudata(L, 3, "VulkanBuffer");
  VkDeviceSize offset = (VkDeviceSize)luaL_checkinteger(L, 4);
  VkDeviceSize range = (VkDeviceSize)luaL_checkinteger(L, 5);

  VkDescriptorBufferInfo bufferInfo = {
      .buffer = buffer_ptr->buffer,
      .offset = offset,
      .range = range
  };

  VkWriteDescriptorSet descriptorWrite = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = set_ptr->set,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pBufferInfo = &bufferInfo
  };

  vkUpdateDescriptorSets(device_ptr->device, 1, &descriptorWrite, 0, NULL);
  return 0;
}


static int l_vulkan_VK_CmdBindDescriptorSet(lua_State *L) {
  VulkanCommandBuffer *cmd_ptr = (VulkanCommandBuffer *)luaL_checkudata(L, 1, "VulkanCommandBuffer");
  VulkanPipeline *pipeline_ptr = (VulkanPipeline *)luaL_checkudata(L, 2, "VulkanPipeline");
  VulkanDescriptorSet *set_ptr = (VulkanDescriptorSet *)luaL_checkudata(L, 3, "VulkanDescriptorSet");
  uint32_t firstSet = (uint32_t)luaL_checkinteger(L, 4);

  if (!cmd_ptr->commandBuffer) {
      luaL_error(L, "Invalid command buffer");
      return 0;
  }

  vkCmdBindDescriptorSets(cmd_ptr->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_ptr->pipelineLayout, firstSet, 1, &set_ptr->set, 0, NULL);
  return 0;
}



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

static int l_vulkan_VK_Surface_gc(lua_State *L) {
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
  uint32_t queueFamilyIndex = (uint32_t)luaL_checkinteger(L, 2);

  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = queueFamilyIndex,
      .queueCount = 1,
      .pQueuePriorities = &queuePriority
  };

  const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queueCreateInfo,
      .enabledExtensionCount = 1,
      .ppEnabledExtensionNames = deviceExtensions
  };

  VkDevice device;
  VkResult result = vkCreateDevice(physicalDevice, &createInfo, NULL, &device);
  if (result != VK_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create logical device");
      return 2;
  }

  VulkanDevice *device_ptr = (VulkanDevice *)lua_newuserdata(L, sizeof(VulkanDevice));
  device_ptr->device = device;
  device_ptr->physicalDevice = physicalDevice;  // Store physical device
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
  VulkanDevice *device = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanSurface *surface_ptr = (VulkanSurface *)luaL_checkudata(L, 2, "VulkanSurface");  // Expect userdata
  SDL_Window *window = (SDL_Window *)lua_touserdata(L, 3);
  VkPhysicalDevice physicalDevice = (VkPhysicalDevice)lua_touserdata(L, 4);

  VkSurfaceCapabilitiesKHR caps;
  VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface_ptr->surface, &caps);
  if (result != VK_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to get surface capabilities");
      return 2;
  }

  VkSwapchainCreateInfoKHR info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface_ptr->surface,  // Use the surface field
      .minImageCount = caps.minImageCount,
      .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
      .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
      .imageExtent = caps.currentExtent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .preTransform = caps.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR,
      .clipped = VK_TRUE,
  };

  VulkanSwapchain *swapchain = (VulkanSwapchain *)lua_newuserdata(L, sizeof(VulkanSwapchain));
  result = vkCreateSwapchainKHR(device->device, &info, NULL, &swapchain->swapchain);
  if (result != VK_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create swapchain");
      return 2;
  }
  swapchain->device = device->device;
  luaL_getmetatable(L, "VulkanSwapchain");
  lua_setmetatable(L, -2);

  lua_pushinteger(L, caps.currentExtent.width);
  lua_pushinteger(L, caps.currentExtent.height);
  return 3;  // Swapchain, width, height
}

static int l_vulkan_VK_DestroySwapchainKHR(lua_State *L) {
  VulkanDevice *device = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanSwapchain *swapchain = (VulkanSwapchain *)luaL_checkudata(L, 2, "VulkanSwapchain");
  vkDestroySwapchainKHR(device->device, swapchain->swapchain, NULL);
  swapchain->swapchain = VK_NULL_HANDLE;
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
  VulkanDevice *device = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanSwapchain *swapchain = (VulkanSwapchain *)luaL_checkudata(L, 2, "VulkanSwapchain");
  uint32_t count;
  vkGetSwapchainImagesKHR(device->device, swapchain->swapchain, &count, NULL);
  VkImage *images = malloc(count * sizeof(VkImage));
  vkGetSwapchainImagesKHR(device->device, swapchain->swapchain, &count, images);
  lua_newtable(L);
  for (uint32_t i = 0; i < count; i++) {
      lua_pushlightuserdata(L, (void *)images[i]);
      lua_rawseti(L, -2, i + 1);
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
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanRenderPass *rp_ptr = (VulkanRenderPass *)luaL_checkudata(L, 2, "VulkanRenderPass");
  printf("l_vulkan_VK_DestroyRenderPass: device_ptr=%p, device=%p, rp_ptr=%p, renderPass=%p\n",
         (void*)device_ptr, (void*)device_ptr->device, (void*)rp_ptr, (void*)rp_ptr->renderPass);
  if (!device_ptr->device) {
      printf("l_vulkan_VK_DestroyRenderPass: Error - device is NULL\n");
      lua_pushstring(L, "Device handle is NULL");
      lua_error(L);
  }
  if (!rp_ptr->renderPass) {
      printf("l_vulkan_VK_DestroyRenderPass: Warning - render pass is already NULL\n");
      return 0;
  }
  vkDestroyRenderPass(device_ptr->device, rp_ptr->renderPass, NULL);
  rp_ptr->renderPass = VK_NULL_HANDLE;
  printf("l_vulkan_VK_DestroyRenderPass: Render pass destroyed successfully\n");
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
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanFramebuffer *fb_ptr = (VulkanFramebuffer *)luaL_checkudata(L, 2, "VulkanFramebuffer");
  printf("l_vulkan_VK_DestroyFramebuffer: device_ptr=%p, device=%p, fb_ptr=%p, framebuffer=%p, imageView=%p\n",
         (void*)device_ptr, (void*)device_ptr->device, (void*)fb_ptr, (void*)fb_ptr->framebuffer, (void*)fb_ptr->imageView);
  if (!device_ptr->device) {
      printf("l_vulkan_VK_DestroyFramebuffer: Error - device is NULL\n");
      lua_pushstring(L, "Device handle is NULL");
      lua_error(L);
  }
  if (fb_ptr->framebuffer) {
      vkDestroyFramebuffer(device_ptr->device, fb_ptr->framebuffer, NULL);
      fb_ptr->framebuffer = VK_NULL_HANDLE;
      printf("l_vulkan_VK_DestroyFramebuffer: Framebuffer destroyed successfully\n");
  }
  if (fb_ptr->imageView) {  // Destroy image view
      vkDestroyImageView(device_ptr->device, fb_ptr->imageView, NULL);
      fb_ptr->imageView = VK_NULL_HANDLE;
      printf("l_vulkan_VK_DestroyFramebuffer: Image view destroyed successfully\n");
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
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanShaderModule *shader_ptr = (VulkanShaderModule *)luaL_checkudata(L, 2, "VulkanShaderModule");
  printf("l_vulkan_VK_DestroyShaderModule: device_ptr=%p, device=%p, shader_ptr=%p, shaderModule=%p\n",
         (void*)device_ptr, (void*)device_ptr->device, (void*)shader_ptr, (void*)shader_ptr->shaderModule);
  if (!device_ptr->device) {
      printf("l_vulkan_VK_DestroyShaderModule: Error - device is NULL\n");
      lua_pushstring(L, "Device handle is NULL");
      lua_error(L);
  }
  if (!shader_ptr->shaderModule) {
      printf("l_vulkan_VK_DestroyShaderModule: Warning - shader module is already NULL\n");
      return 0;
  }
  vkDestroyShaderModule(device_ptr->device, shader_ptr->shaderModule, NULL);
  shader_ptr->shaderModule = VK_NULL_HANDLE;
  printf("l_vulkan_VK_DestroyShaderModule: Shader module destroyed successfully\n");
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



static int l_vulkan_VK_CreateBuffer(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VkDeviceSize size = (VkDeviceSize)luaL_checkinteger(L, 2);

  VkBufferCreateInfo bufferInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  VulkanBuffer *buffer_ptr = (VulkanBuffer *)lua_newuserdata(L, sizeof(VulkanBuffer));
  buffer_ptr->device = device_ptr->device;
  VkResult result = vkCreateBuffer(device_ptr->device, &bufferInfo, NULL, &buffer_ptr->buffer);
  if (result != VK_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create buffer");
      return 2;
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device_ptr->device, buffer_ptr->buffer, &memRequirements);

  // Get physical device memory properties
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(device_ptr->physicalDevice, &memProperties);

  // Find a suitable memory type
  uint32_t memoryTypeIndex = UINT32_MAX;
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((memRequirements.memoryTypeBits & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & 
           (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) == 
           (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
          memoryTypeIndex = i;
          break;
      }
  }

  if (memoryTypeIndex == UINT32_MAX) {
      vkDestroyBuffer(device_ptr->device, buffer_ptr->buffer, NULL);
      lua_pushnil(L);
      lua_pushstring(L, "Failed to find suitable memory type");
      return 2;
  }

  VkMemoryAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex = memoryTypeIndex
  };

  result = vkAllocateMemory(device_ptr->device, &allocInfo, NULL, &buffer_ptr->memory);
  if (result != VK_SUCCESS) {
      vkDestroyBuffer(device_ptr->device, buffer_ptr->buffer, NULL);
      lua_pushnil(L);
      lua_pushstring(L, "Failed to allocate buffer memory");
      return 2;
  }

  vkBindBufferMemory(device_ptr->device, buffer_ptr->buffer, buffer_ptr->memory, 0);

  luaL_getmetatable(L, "VulkanBuffer");
  lua_setmetatable(L, -2);
  return 1;
}


static int l_vulkan_VK_DestroyBuffer(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanBuffer *buf_ptr = (VulkanBuffer *)luaL_checkudata(L, 2, "VulkanBuffer");
  printf("l_vulkan_VK_DestroyBuffer: device_ptr=%p, device=%p, buf_ptr=%p, buffer=%p, memory=%p\n",
         (void*)device_ptr, (void*)device_ptr->device, (void*)buf_ptr, (void*)buf_ptr->buffer, (void*)buf_ptr->memory);
  if (!device_ptr->device) {
      printf("l_vulkan_VK_DestroyBuffer: Error - device is NULL\n");
      lua_pushstring(L, "Device handle is NULL");
      lua_error(L);
  }
  if (buf_ptr->memory) {  // Free memory first
      vkFreeMemory(device_ptr->device, buf_ptr->memory, NULL);
      buf_ptr->memory = VK_NULL_HANDLE;
      printf("l_vulkan_VK_DestroyBuffer: Memory freed successfully\n");
  }
  if (buf_ptr->buffer) {
      vkDestroyBuffer(device_ptr->device, buf_ptr->buffer, NULL);
      buf_ptr->buffer = VK_NULL_HANDLE;
      printf("l_vulkan_VK_DestroyBuffer: Buffer destroyed successfully\n");
  }
  return 0;
}

static int l_vulkan_VK_UpdateBuffer(lua_State *L) {
  VulkanBuffer *buffer_ptr = (VulkanBuffer *)luaL_checkudata(L, 1, "VulkanBuffer");
  luaL_checktype(L, 2, LUA_TTABLE);

  lua_getfield(L, 2, "x");
  float x = (float)luaL_checknumber(L, -1);
  lua_pop(L, 1);
  lua_getfield(L, 2, "y");
  float y = (float)luaL_checknumber(L, -1);
  lua_pop(L, 1);

  float uniformData[2] = {x, y};
  void* data;
  vkMapMemory(buffer_ptr->device, buffer_ptr->memory, 0, sizeof(uniformData), 0, &data);
  memcpy(data, uniformData, sizeof(uniformData));
  vkUnmapMemory(buffer_ptr->device, buffer_ptr->memory);
  return 0;
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

  // Create descriptor set layout
  VkDescriptorSetLayoutBinding uboLayoutBinding = {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
  };
  VkDescriptorSetLayoutCreateInfo layoutInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1,
      .pBindings = &uboLayoutBinding
  };
  VkDescriptorSetLayout descriptorSetLayout;
  VkResult result = vkCreateDescriptorSetLayout(device_ptr->device, &layoutInfo, NULL, &descriptorSetLayout);
  if (result != VK_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create descriptor set layout");
      return 2;
  }

  // Create pipeline layout with descriptor set
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &descriptorSetLayout
  };
  VkPipelineLayout pipelineLayout;
  result = vkCreatePipelineLayout(device_ptr->device, &pipelineLayoutInfo, NULL, &pipelineLayout);
  if (result != VK_SUCCESS) {
      vkDestroyDescriptorSetLayout(device_ptr->device, descriptorSetLayout, NULL);
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create pipeline layout");
      return 2;
  }

  // Pipeline creation (unchanged except for layout)
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
      .frontFace = VK_FRONT_FACE_CLOCKWISE
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
      .layout = pipelineLayout,
      .renderPass = renderPass_ptr->renderPass,
      .subpass = 0
  };

  VkPipeline pipeline;
  result = vkCreateGraphicsPipelines(device_ptr->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline);
  if (result != VK_SUCCESS) {
      vkDestroyPipelineLayout(device_ptr->device, pipelineLayout, NULL);
      vkDestroyDescriptorSetLayout(device_ptr->device, descriptorSetLayout, NULL);
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create graphics pipeline");
      return 2;
  }

  VulkanPipeline *pipeline_ptr = (VulkanPipeline *)lua_newuserdata(L, sizeof(VulkanPipeline));
  pipeline_ptr->pipeline = pipeline;
  pipeline_ptr->pipelineLayout = pipelineLayout;
  pipeline_ptr->device = device_ptr->device;
  luaL_getmetatable(L, "VulkanPipeline");
  lua_setmetatable(L, -2);

  // Return descriptor set layout as proper userdata
  VulkanDescriptorSetLayout *layout_ptr = (VulkanDescriptorSetLayout *)lua_newuserdata(L, sizeof(VulkanDescriptorSetLayout));
  layout_ptr->layout = descriptorSetLayout;
  layout_ptr->device = device_ptr->device;
  luaL_getmetatable(L, "VulkanDescriptorSetLayout");
  lua_setmetatable(L, -2);

  return 2;  // Return pipeline and descriptor set layout
}


static int l_vulkan_VK_DestroyPipeline(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanPipeline *pipe_ptr = (VulkanPipeline *)luaL_checkudata(L, 2, "VulkanPipeline");
  printf("l_vulkan_VK_DestroyPipeline: device_ptr=%p, device=%p, pipe_ptr=%p, pipeline=%p, pipelineLayout=%p\n",
         (void*)device_ptr, (void*)device_ptr->device, (void*)pipe_ptr, (void*)pipe_ptr->pipeline, (void*)pipe_ptr->pipelineLayout);
  if (!device_ptr->device) {
      printf("l_vulkan_VK_DestroyPipeline: Error - device is NULL\n");
      lua_pushstring(L, "Device handle is NULL");
      lua_error(L);
  }
  if (pipe_ptr->pipeline) {
      vkDestroyPipeline(device_ptr->device, pipe_ptr->pipeline, NULL);
      pipe_ptr->pipeline = VK_NULL_HANDLE;
      printf("l_vulkan_VK_DestroyPipeline: Pipeline destroyed successfully\n");
  }
  if (pipe_ptr->pipelineLayout) {  // Destroy pipeline layout
      vkDestroyPipelineLayout(device_ptr->device, pipe_ptr->pipelineLayout, NULL);
      pipe_ptr->pipelineLayout = VK_NULL_HANDLE;
      printf("l_vulkan_VK_DestroyPipeline: Pipeline layout destroyed successfully\n");
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
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanCommandPool *pool_ptr = (VulkanCommandPool *)luaL_checkudata(L, 2, "VulkanCommandPool");
  printf("l_vulkan_VK_DestroyCommandPool: device_ptr=%p, device=%p, pool_ptr=%p, commandPool=%p\n",
         (void*)device_ptr, (void*)device_ptr->device, (void*)pool_ptr, (void*)pool_ptr->commandPool);
  if (!device_ptr->device) {
      printf("l_vulkan_VK_DestroyCommandPool: Error - device is NULL\n");
      lua_pushstring(L, "Device handle is NULL");
      lua_error(L);
  }
  if (!pool_ptr->commandPool) {
      printf("l_vulkan_VK_DestroyCommandPool: Warning - command pool is already NULL\n");
      return 0;
  }
  vkDestroyCommandPool(device_ptr->device, pool_ptr->commandPool, NULL);
  pool_ptr->commandPool = VK_NULL_HANDLE;
  printf("l_vulkan_VK_DestroyCommandPool: Command pool destroyed successfully\n");
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


static int l_vulkan_VK_CmdPipelineBarrier(lua_State *L) {
  VulkanCommandBuffer *cmd = (VulkanCommandBuffer *)luaL_checkudata(L, 1, "VulkanCommandBuffer");
  VkPipelineStageFlags srcStage = (VkPipelineStageFlags)luaL_checkinteger(L, 2);
  VkPipelineStageFlags dstStage = (VkPipelineStageFlags)luaL_checkinteger(L, 3);
  VkDependencyFlags dependencyFlags = (VkDependencyFlags)luaL_checkinteger(L, 4);
  luaL_checktype(L, 5, LUA_TTABLE);  // Memory barriers (empty)
  luaL_checktype(L, 6, LUA_TTABLE);  // Buffer barriers (empty)
  luaL_checktype(L, 7, LUA_TTABLE);  // Image barriers

  VkImageMemoryBarrier imageBarriers[1];
  lua_rawgeti(L, 7, 1);  // Replace lua_geti with lua_rawgeti
  imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageBarriers[0].pNext = NULL;

  lua_getfield(L, -1, "srcAccessMask");
  imageBarriers[0].srcAccessMask = (lua_type(L, -1) == LUA_TNUMBER) ? (VkAccessFlags)lua_tointeger(L, -1) : 0;
  lua_pop(L, 1);

  lua_getfield(L, -1, "dstAccessMask");
  imageBarriers[0].dstAccessMask = (lua_type(L, -1) == LUA_TNUMBER) ? (VkAccessFlags)lua_tointeger(L, -1) : 0;
  lua_pop(L, 1);

  lua_getfield(L, -1, "oldLayout");
  imageBarriers[0].oldLayout = (lua_type(L, -1) == LUA_TNUMBER) ? (VkImageLayout)lua_tointeger(L, -1) : VK_IMAGE_LAYOUT_UNDEFINED;
  lua_pop(L, 1);

  lua_getfield(L, -1, "newLayout");
  imageBarriers[0].newLayout = (lua_type(L, -1) == LUA_TNUMBER) ? (VkImageLayout)lua_tointeger(L, -1) : VK_IMAGE_LAYOUT_UNDEFINED;
  lua_pop(L, 1);

  lua_getfield(L, -1, "image");
  imageBarriers[0].image = (lua_type(L, -1) == LUA_TLIGHTUSERDATA) ? (VkImage)lua_touserdata(L, -1) : VK_NULL_HANDLE;
  lua_pop(L, 1);

  imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageBarriers[0].subresourceRange.baseMipLevel = 0;
  imageBarriers[0].subresourceRange.levelCount = 1;
  imageBarriers[0].subresourceRange.baseArrayLayer = 0;
  imageBarriers[0].subresourceRange.layerCount = 1;

  lua_pop(L, 1);  // Pop the image barrier table

  vkCmdPipelineBarrier(cmd->commandBuffer, srcStage, dstStage, dependencyFlags, 0, NULL, 0, NULL, 1, imageBarriers);
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
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanSemaphore *sem_ptr = (VulkanSemaphore *)luaL_checkudata(L, 2, "VulkanSemaphore");
  printf("l_vulkan_VK_DestroySemaphore: device_ptr=%p, device=%p, sem_ptr=%p, semaphore=%p\n",
         (void*)device_ptr, (void*)device_ptr->device, (void*)sem_ptr, (void*)sem_ptr->semaphore);
  if (!device_ptr->device) {
      printf("l_vulkan_VK_DestroySemaphore: Error - device is NULL\n");
      lua_pushstring(L, "Device handle is NULL");
      lua_error(L);
  }
  if (!sem_ptr->semaphore) {
      printf("l_vulkan_VK_DestroySemaphore: Warning - semaphore is already NULL\n");
      return 0;
  }
  vkDestroySemaphore(device_ptr->device, sem_ptr->semaphore, NULL);
  sem_ptr->semaphore = VK_NULL_HANDLE;
  printf("l_vulkan_VK_DestroySemaphore: Semaphore destroyed successfully\n");
  return 0;
}

static int l_vulkan_VK_AcquireNextImageKHR(lua_State *L) {
  VulkanDevice *device = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanSwapchain *swapchain = (VulkanSwapchain *)luaL_checkudata(L, 2, "VulkanSwapchain");  // Check for userdata
  VulkanSemaphore *semaphore = (VulkanSemaphore *)luaL_checkudata(L, 3, "VulkanSemaphore");
  uint64_t timeout = luaL_optinteger(L, 4, UINT64_MAX);
  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(device->device, swapchain->swapchain, timeout, semaphore->semaphore, VK_NULL_HANDLE, &imageIndex);
  if (result == VK_SUCCESS) {
      lua_pushinteger(L, imageIndex);
      lua_pushboolean(L, false);
  } else if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
      lua_pushinteger(L, -1);
      lua_pushboolean(L, true);
  } else {
      lua_pushnil(L);
      lua_pushboolean(L, false);
  }
  return 2;
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
  VulkanSwapchain *swapchain = (VulkanSwapchain *)luaL_checkudata(L, 2, "VulkanSwapchain");
  uint32_t imageIndex = (uint32_t)luaL_checkinteger(L, 3);
  VulkanSemaphore *semaphore = (VulkanSemaphore *)luaL_checkudata(L, 4, "VulkanSemaphore");
  VkPresentInfoKHR info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &semaphore->semaphore,
      .swapchainCount = 1,
      .pSwapchains = &swapchain->swapchain,
      .pImageIndices = &imageIndex,
  };
  VkResult result = vkQueuePresentKHR(queue, &info);
  lua_pushboolean(L, result == VK_SUCCESS);
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

// Keep the explicit destroy function
static int l_vulkan_VK_DestroyFence(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanFence *fence_ptr = (VulkanFence *)luaL_checkudata(L, 2, "VulkanFence");
  printf("l_vulkan_VK_DestroyFence: device_ptr=%p, device=%p, fence_ptr=%p, fence=%p\n",
         (void*)device_ptr, (void*)device_ptr->device, (void*)fence_ptr, (void*)fence_ptr->fence);
  if (!device_ptr->device) {
      printf("l_vulkan_VK_DestroyFence: Error - device is NULL\n");
      lua_pushstring(L, "Device handle is NULL");
      lua_error(L);
  }
  if (!fence_ptr->fence) {
      printf("l_vulkan_VK_DestroyFence: Warning - fence is already NULL, skipping destruction\n");
      return 0;
  }
  vkDestroyFence(device_ptr->device, fence_ptr->fence, NULL);
  fence_ptr->fence = VK_NULL_HANDLE;
  printf("l_vulkan_VK_DestroyFence: Fence destroyed successfully\n");
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

static int l_vulkan_VK_DestroyDescriptorSetLayout(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanDescriptorSetLayout *layout_ptr = (VulkanDescriptorSetLayout *)luaL_checkudata(L, 2, "VulkanDescriptorSetLayout");
  printf("l_vulkan_VK_DestroyDescriptorSetLayout: device_ptr=%p, device=%p, layout_ptr=%p, layout=%p\n",
         (void*)device_ptr, (void*)device_ptr->device, (void*)layout_ptr, (void*)layout_ptr->layout);
  if (!device_ptr->device) {
      printf("l_vulkan_VK_DestroyDescriptorSetLayout: Error - device is NULL\n");
      lua_pushstring(L, "Device handle is NULL");
      lua_error(L);
  }
  if (!layout_ptr->layout) {
      printf("l_vulkan_VK_DestroyDescriptorSetLayout: Warning - descriptor set layout is already NULL\n");
      return 0;
  }
  vkDestroyDescriptorSetLayout(device_ptr->device, layout_ptr->layout, NULL);
  layout_ptr->layout = VK_NULL_HANDLE;
  printf("l_vulkan_VK_DestroyDescriptorSetLayout: Descriptor set layout destroyed successfully\n");
  return 0;
}

// Add near other metatable definitions (e.g., after descriptorpool_mt)
static int l_vulkan_VK_FreeDescriptorSet(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanDescriptorPool *pool_ptr = (VulkanDescriptorPool *)luaL_checkudata(L, 2, "VulkanDescriptorPool");
  VulkanDescriptorSet *set_ptr = (VulkanDescriptorSet *)luaL_checkudata(L, 3, "VulkanDescriptorSet");
  if (set_ptr->set) {
      vkFreeDescriptorSets(device_ptr->device, pool_ptr->pool, 1, &set_ptr->set);
      set_ptr->set = VK_NULL_HANDLE;
      printf("l_vulkan_VK_FreeDescriptorSet: Descriptor set freed successfully\n");
  }
  return 0;
}


static int l_vulkan_VK_FreeCommandBuffers(lua_State *L) {
  VulkanDevice *device_ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanCommandPool *pool_ptr = (VulkanCommandPool *)luaL_checkudata(L, 2, "VulkanCommandPool");
  luaL_checktype(L, 3, LUA_TTABLE);
  uint32_t count = lua_objlen(L, 3);

  VkCommandBuffer *buffers = malloc(count * sizeof(VkCommandBuffer));
  for (uint32_t i = 1; i <= count; i++) {
      lua_rawgeti(L, 3, i);
      VulkanCommandBuffer *cmd_ptr = (VulkanCommandBuffer *)luaL_checkudata(L, -1, "VulkanCommandBuffer");
      buffers[i - 1] = cmd_ptr->commandBuffer;
      lua_pop(L, 1);
  }

  vkFreeCommandBuffers(device_ptr->device, pool_ptr->commandPool, count, buffers);
  free(buffers);
  return 0;
}


static int l_vulkan_VK_CreateDescriptorPool(lua_State *L) {
  VulkanDevice *device = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  luaL_checktype(L, 2, LUA_TTABLE);
  VkDescriptorPoolCreateInfo info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };

  lua_getfield(L, 2, "flags");
  info.flags = (lua_type(L, -1) == LUA_TNUMBER) ? (VkDescriptorPoolCreateFlags)lua_tointeger(L, -1) : 0;
  lua_pop(L, 1);

  lua_getfield(L, 2, "maxSets");
  info.maxSets = (lua_type(L, -1) == LUA_TNUMBER) ? (uint32_t)lua_tointeger(L, -1) : 0;
  lua_pop(L, 1);

  lua_getfield(L, 2, "poolSizes");
  info.poolSizeCount = lua_objlen(L, -1);  // Replace lua_rawlen with lua_objlen
  VkDescriptorPoolSize *sizes = malloc(info.poolSizeCount * sizeof(VkDescriptorPoolSize));
  for (int i = 1; i <= info.poolSizeCount; i++) {
      lua_rawgeti(L, -1, i);  // Replace lua_geti with lua_rawgeti
      lua_getfield(L, -1, "type");
      sizes[i-1].type = (lua_type(L, -1) == LUA_TNUMBER) ? (VkDescriptorType)lua_tointeger(L, -1) : 0;
      lua_pop(L, 1);
      lua_getfield(L, -1, "descriptorCount");
      sizes[i-1].descriptorCount = (lua_type(L, -1) == LUA_TNUMBER) ? (uint32_t)lua_tointeger(L, -1) : 0;
      lua_pop(L, 1);
      lua_pop(L, 1);
  }
  info.pPoolSizes = sizes;

  VulkanDescriptorPool *pool = (VulkanDescriptorPool *)lua_newuserdata(L, sizeof(VulkanDescriptorPool));
  if (vkCreateDescriptorPool(device->device, &info, NULL, &pool->pool) != VK_SUCCESS) {
      free(sizes);
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create descriptor pool");
      return 2;
  }
  pool->device = device->device;
  free(sizes);
  luaL_getmetatable(L, "VulkanDescriptorPool");
  lua_setmetatable(L, -2);
  return 1;
}


static int l_vulkan_VK_FreeDescriptorSets(lua_State *L) {
  VulkanDevice *device = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanDescriptorPool *pool = (VulkanDescriptorPool *)luaL_checkudata(L, 2, "VulkanDescriptorPool");
  VulkanDescriptorSet *set = (VulkanDescriptorSet *)luaL_checkudata(L, 3, "VulkanDescriptorSet");
  VkResult result = vkFreeDescriptorSets(device->device, pool->pool, 1, &set->set);
  if (result != VK_SUCCESS) {
      lua_pushstring(L, "Failed to free descriptor set");
      return lua_error(L);
  }
  return 0;
}


static int l_vulkan_VK_Semaphore_gc(lua_State *L) {
  VulkanSemaphore *sem_ptr = (VulkanSemaphore *)luaL_checkudata(L, 1, "VulkanSemaphore");
  printf("l_vulkan_VK_Semaphore_gc: sem_ptr=%p, semaphore=%p, device=%p\n",
         (void*)sem_ptr, (void*)sem_ptr->semaphore, (void*)sem_ptr->device);
  if (sem_ptr->semaphore && sem_ptr->device) {
      vkDestroySemaphore(sem_ptr->device, sem_ptr->semaphore, NULL);
      sem_ptr->semaphore = VK_NULL_HANDLE;
  }
  return 0;
}

// VulkanDescriptorPool
static int l_vulkan_VK_DescriptorPool_gc(lua_State *L) {
  VulkanDescriptorPool *ptr = (VulkanDescriptorPool *)luaL_checkudata(L, 1, "VulkanDescriptorPool");
  if (ptr->pool && ptr->device) {
      vkDestroyDescriptorPool(ptr->device, ptr->pool, NULL);
      ptr->pool = VK_NULL_HANDLE;
  }
  return 0;
}

// VulkanDescriptorSetLayout
static int l_vulkan_VK_DescriptorSetLayout_gc(lua_State *L) {
  VulkanDescriptorSetLayout *ptr = (VulkanDescriptorSetLayout *)luaL_checkudata(L, 1, "VulkanDescriptorSetLayout");
  if (ptr->layout && ptr->device) {
      vkDestroyDescriptorSetLayout(ptr->device, ptr->layout, NULL);
      ptr->layout = VK_NULL_HANDLE;
  }
  return 0;
}


// VulkanDevice (already correct, but included for completeness)
static int l_vulkan_VK_Device_gc(lua_State *L) {
  VulkanDevice *ptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  if (ptr->device) {
      vkDestroyDevice(ptr->device, NULL);
      ptr->device = VK_NULL_HANDLE;
  }
  return 0;
}


// Define the __gc-specific function for VulkanFence
static int l_vulkan_VK_Fence_gc(lua_State *L) {
  VulkanFence *fence_ptr = (VulkanFence *)luaL_checkudata(L, 1, "VulkanFence");
  printf("l_vulkan_VK_Fence_gc: fence_ptr=%p, fence=%p, device=%p\n",
         (void*)fence_ptr, (void*)fence_ptr->fence, (void*)fence_ptr->device);
  if (fence_ptr->fence && fence_ptr->device) {
      vkDestroyFence(fence_ptr->device, fence_ptr->fence, NULL);
      fence_ptr->fence = VK_NULL_HANDLE;
      printf("l_vulkan_VK_Fence_gc: Fence destroyed successfully\n");
  }
  return 0;
}

// VulkanSwapchain
static int l_vulkan_VK_Swapchain_gc(lua_State *L) {
  VulkanSwapchain *swapchain = (VulkanSwapchain *)luaL_checkudata(L, 1, "VulkanSwapchain");
  if (swapchain->swapchain != VK_NULL_HANDLE) {
      vkDestroySwapchainKHR(swapchain->device, swapchain->swapchain, NULL);
      swapchain->swapchain = VK_NULL_HANDLE;
  }
  return 0;
}


// VulkanRenderPass
static int l_vulkan_VK_RenderPass_gc(lua_State *L) {
  VulkanRenderPass *ptr = (VulkanRenderPass *)luaL_checkudata(L, 1, "VulkanRenderPass");
  if (ptr->renderPass && ptr->device) {
      vkDestroyRenderPass(ptr->device, ptr->renderPass, NULL);
      ptr->renderPass = VK_NULL_HANDLE;
  }
  return 0;
}

// VulkanFramebuffer
static int l_vulkan_VK_Framebuffer_gc(lua_State *L) {
  VulkanFramebuffer *ptr = (VulkanFramebuffer *)luaL_checkudata(L, 1, "VulkanFramebuffer");
  if (ptr->framebuffer && ptr->device) {
      vkDestroyFramebuffer(ptr->device, ptr->framebuffer, NULL);
      ptr->framebuffer = VK_NULL_HANDLE;
  }
  if (ptr->imageView && ptr->device) {
      vkDestroyImageView(ptr->device, ptr->imageView, NULL);
      ptr->imageView = VK_NULL_HANDLE;
  }
  return 0;
}

// VulkanBuffer
static int l_vulkan_VK_Buffer_gc(lua_State *L) {
  VulkanBuffer *ptr = (VulkanBuffer *)luaL_checkudata(L, 1, "VulkanBuffer");
  if (ptr->memory && ptr->device) {
      vkFreeMemory(ptr->device, ptr->memory, NULL);
      ptr->memory = VK_NULL_HANDLE;
  }
  if (ptr->buffer && ptr->device) {
      vkDestroyBuffer(ptr->device, ptr->buffer, NULL);
      ptr->buffer = VK_NULL_HANDLE;
  }
  return 0;
}

// VulkanShaderModule
static int l_vulkan_VK_ShaderModule_gc(lua_State *L) {
  VulkanShaderModule *ptr = (VulkanShaderModule *)luaL_checkudata(L, 1, "VulkanShaderModule");
  if (ptr->shaderModule && ptr->device) {
      vkDestroyShaderModule(ptr->device, ptr->shaderModule, NULL);
      ptr->shaderModule = VK_NULL_HANDLE;
  }
  return 0;
}

// VulkanPipeline
static int l_vulkan_VK_Pipeline_gc(lua_State *L) {
  VulkanPipeline *ptr = (VulkanPipeline *)luaL_checkudata(L, 1, "VulkanPipeline");
  if (ptr->pipeline && ptr->device) {
      vkDestroyPipeline(ptr->device, ptr->pipeline, NULL);
      ptr->pipeline = VK_NULL_HANDLE;
  }
  if (ptr->pipelineLayout && ptr->device) {
      vkDestroyPipelineLayout(ptr->device, ptr->pipelineLayout, NULL);
      ptr->pipelineLayout = VK_NULL_HANDLE;
  }
  return 0;
}

// VulkanCommandPool
static int l_vulkan_VK_CommandPool_gc(lua_State *L) {
  VulkanCommandPool *ptr = (VulkanCommandPool *)luaL_checkudata(L, 1, "VulkanCommandPool");
  if (ptr->commandPool && ptr->device) {
      vkDestroyCommandPool(ptr->device, ptr->commandPool, NULL);
      ptr->commandPool = VK_NULL_HANDLE;
  }
  return 0;
}

static int l_vulkan_VK_DescriptorSet_gc(lua_State *L) {
  VulkanDescriptorSet *ptr = (VulkanDescriptorSet *)luaL_checkudata(L, 1, "VulkanDescriptorSet");
  printf("l_vulkan_VK_DescriptorSet_gc: set_ptr=%p, set=%p, device=%p\n",
         (void*)ptr, (void*)ptr->set, (void*)ptr->device);
  if (ptr->set && ptr->device && ptr->pool) {
      vkFreeDescriptorSets(ptr->device, ptr->pool, 1, &ptr->set);
      ptr->set = VK_NULL_HANDLE;
  }
  return 0;
}


static const luaL_Reg vulkan_funcs[] = {
    {"VK_FreeDescriptorSets", l_vulkan_VK_FreeDescriptorSets},
    {"VK_FreeCommandBuffers", l_vulkan_VK_FreeCommandBuffers},
    {"VK_DestroyDescriptorSetLayout", l_vulkan_VK_DestroyDescriptorSetLayout},
    {"VK_CreateDescriptorPool", l_vulkan_VK_CreateDescriptorPool},
    {"VK_DestroyDescriptorPool", l_vulkan_VK_DestroyDescriptorPool},
    {"VK_AllocateDescriptorSet", l_vulkan_VK_AllocateDescriptorSet},
    {"VK_UpdateDescriptorSet", l_vulkan_VK_UpdateDescriptorSet},
    {"VK_CmdBindDescriptorSet", l_vulkan_VK_CmdBindDescriptorSet},
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
    {"VK_CreateBuffer", l_vulkan_VK_CreateBuffer},
    {"VK_DestroyBuffer", l_vulkan_VK_DestroyBuffer},
    {"VK_UpdateBuffer", l_vulkan_VK_UpdateBuffer},
    {"VK_CreateGraphicsPipelines", l_vulkan_VK_CreateGraphicsPipelines},
    {"VK_DestroyPipeline", l_vulkan_VK_DestroyPipeline},
    {"VK_CreateCommandPool", l_vulkan_VK_CreateCommandPool},
    {"VK_DestroyCommandPool", l_vulkan_VK_DestroyCommandPool},
    {"VK_AllocateCommandBuffers", l_vulkan_VK_AllocateCommandBuffers},
    {"VK_BeginCommandBuffer", l_vulkan_VK_BeginCommandBuffer},
    {"VK_EndCommandBuffer", l_vulkan_VK_EndCommandBuffer},
    {"VK_CmdBeginRenderPass", l_vulkan_VK_CmdBeginRenderPass},
    {"VK_CmdEndRenderPass", l_vulkan_VK_CmdEndRenderPass},
    {"VK_CmdPipelineBarrier", l_vulkan_VK_CmdPipelineBarrier},
    {"VK_CmdBindPipeline", l_vulkan_VK_CmdBindPipeline},
    {"VK_CmdDraw", l_vulkan_VK_CmdDraw},
    {"VK_CreateSemaphore", l_vulkan_VK_CreateSemaphore},
    {"VK_DestroySemaphore", l_vulkan_VK_DestroySemaphore},
    {"VK_AcquireNextImageKHR", l_vulkan_VK_AcquireNextImageKHR},
    {"VK_QueueSubmit", l_vulkan_VK_QueueSubmit},
    {"VK_QueuePresentKHR", l_vulkan_VK_QueuePresentKHR},
    {"VK_QueueWaitIdle", l_vulkan_VK_QueueWaitIdle},
    {"VK_CreateFence", l_vulkan_VK_CreateFence},           
    {"VK_DestroyFence", l_vulkan_VK_DestroyFence},         
    {"VK_WaitForFences", l_vulkan_VK_WaitForFences},       
    {"VK_ResetFences", l_vulkan_VK_ResetFences},           
    {NULL, NULL}
};

static const luaL_Reg fence_mt[] = {
  {"__gc", l_vulkan_VK_Fence_gc}, // For automatic cleanup
  {NULL, NULL}
};

static const luaL_Reg instance_mt[] = {
    {"__gc", l_vulkan_VK_DestroyInstance},
    {NULL, NULL}
};

static const luaL_Reg surface_mt[] = {
    {"__gc", l_vulkan_VK_Surface_gc},
    {NULL, NULL}
};

static const luaL_Reg device_mt[] = {
    {"__gc", l_vulkan_VK_Device_gc},
    {NULL, NULL}
};

static const luaL_Reg swapchain_mt[] = {
    {"__gc", l_vulkan_VK_Swapchain_gc},
    {NULL, NULL}
};

static const luaL_Reg renderpass_mt[] = {
    {"__gc", l_vulkan_VK_RenderPass_gc},
    {NULL, NULL}
};

static const luaL_Reg framebuffer_mt[] = {
    {"__gc", l_vulkan_VK_Framebuffer_gc},
    {NULL, NULL}
};

static const luaL_Reg buffer_mt[] = {
  {"__gc", l_vulkan_VK_Buffer_gc},
  {NULL, NULL}
};

static const luaL_Reg shadermodule_mt[] = {
    {"__gc", l_vulkan_VK_ShaderModule_gc},
    {NULL, NULL}
};

static const luaL_Reg pipeline_mt[] = {
    {"__gc", l_vulkan_VK_Pipeline_gc},
    {NULL, NULL}
};

static const luaL_Reg commandpool_mt[] = {
    {"__gc", l_vulkan_VK_CommandPool_gc},
    {NULL, NULL}
};

static const luaL_Reg commandbuffer_mt[] = {
    {NULL, NULL}  // No __gc, freed with command pool
};

static const luaL_Reg semaphore_mt[] = {
    {"__gc", l_vulkan_VK_Semaphore_gc},
    {NULL, NULL}
};

static const luaL_Reg descriptorpool_mt[] = {
  {"__gc", l_vulkan_VK_DescriptorPool_gc},
  {NULL, NULL}
};

static const luaL_Reg descriptorset_mt[] = {
  {"__gc", l_vulkan_VK_DescriptorSet_gc},
  {NULL, NULL}
};

static const luaL_Reg VulkanDescriptorSetLayout_meta[] = {
  {"__gc", l_vulkan_VK_DescriptorSetLayout_gc},
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

    luaL_newmetatable(L, "VulkanBuffer");
    luaL_setfuncs(L, buffer_mt, 0);
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

    luaL_newmetatable(L, "VulkanFence");
    luaL_setfuncs(L, fence_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanDescriptorPool");
    luaL_setfuncs(L, descriptorpool_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanDescriptorSet");
    luaL_setfuncs(L, descriptorset_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanDescriptorSetLayout");
    luaL_setfuncs(L, VulkanDescriptorSetLayout_meta, 0);
    lua_pop(L, 1);

    luaL_newlib(L, vulkan_funcs);
    lua_pushinteger(L, VK_API_VERSION_1_4);
    lua_setfield(L, -2, "API_VERSION_1_4");
    lua_pushinteger(L, VK_QUEUE_GRAPHICS_BIT);
    lua_setfield(L, -2, "VK_QUEUE_GRAPHICS_BIT");
    lua_pushinteger(L, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    lua_setfield(L, -2, "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER");
    lua_pushstring(L, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    lua_setfield(L, -2, "KHR_SWAPCHAIN_EXTENSION_NAME");

    lua_pushinteger(L, VK_IMAGE_LAYOUT_UNDEFINED);
    lua_setfield(L, -2, "VK_IMAGE_LAYOUT_UNDEFINED");
    lua_pushinteger(L, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    lua_setfield(L, -2, "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL");
    lua_pushinteger(L, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    lua_setfield(L, -2, "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR");
    lua_pushinteger(L, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    lua_setfield(L, -2, "VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT");
    lua_pushinteger(L, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    lua_setfield(L, -2, "VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT");
    lua_pushinteger(L, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
    lua_setfield(L, -2, "VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT");

    lua_pushinteger(L, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    lua_setfield(L, -2, "VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT");
    lua_pushinteger(L, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    lua_setfield(L, -2, "VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT");
    lua_pushinteger(L, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    lua_setfield(L, -2, "VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT");
    // Add descriptor pool flag
    lua_pushinteger(L, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
    lua_setfield(L, -2, "VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT");

    //printf("[[ VK_QUEUE_GRAPHICS_BIT ]]: %d\n", VK_QUEUE_GRAPHICS_BIT);


    return 1;
}