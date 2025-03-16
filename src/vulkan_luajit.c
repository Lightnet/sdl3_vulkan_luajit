#include "vulkan_luajit.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdio.h>

// typedef struct {
//     VkPhysicalDevice physicalDevice;
// } VulkanPhysicalDevice;

// typedef struct {
//     VkDevice device;
// } VulkanDevice;

// typedef struct {
//   VkQueue queue;
// } VulkanQueue;


static int l_vk_make_version(lua_State *L) {
  uint32_t major = (uint32_t)luaL_checkinteger(L, 1);
  uint32_t minor = (uint32_t)luaL_checkinteger(L, 2);
  uint32_t patch = (uint32_t)luaL_checkinteger(L, 3);
  uint32_t version = VK_MAKE_VERSION(major, minor, patch);
  lua_pushinteger(L, version);
  return 1;
}

static int l_vk_CreateInstance(lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);

  VkApplicationInfo appInfo = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO };
  lua_getfield(L, 1, "application_info");
  if (lua_istable(L, -1)) {
      lua_getfield(L, -1, "application_name");
      appInfo.pApplicationName = luaL_optstring(L, -1, "LuaJIT Vulkan App");
      lua_pop(L, 1);

      lua_getfield(L, -1, "application_version");
      appInfo.applicationVersion = luaL_optinteger(L, -1, VK_MAKE_VERSION(1, 0, 0));
      lua_pop(L, 1);

      lua_getfield(L, -1, "engine_name");
      appInfo.pEngineName = luaL_optstring(L, -1, "LuaJIT Vulkan");
      lua_pop(L, 1);

      lua_getfield(L, -1, "engine_version");
      appInfo.engineVersion = luaL_optinteger(L, -1, VK_MAKE_VERSION(1, 0, 0));
      lua_pop(L, 1);

      lua_getfield(L, -1, "api_version");
      appInfo.apiVersion = luaL_optinteger(L, -1, VK_API_VERSION_1_0);
      lua_pop(L, 1);
  }
  lua_pop(L, 1);

  uint32_t layerCount = 0;
  const char **layerNames = NULL;
  lua_getfield(L, 1, "enabled_layer_names");
  if (lua_istable(L, -1)) {
      layerCount = (uint32_t)lua_objlen(L, -1);
      if (layerCount > 0) {
          layerNames = malloc(layerCount * sizeof(const char *));
          for (uint32_t i = 0; i < layerCount; i++) {
              lua_rawgeti(L, -1, i + 1);
              layerNames[i] = luaL_checkstring(L, -1);
              lua_pop(L, 1);
          }
      }
  }
  lua_pop(L, 1);

  uint32_t extensionCount = 0;
  const char **extensionNames = NULL;
  lua_getfield(L, 1, "enabled_extension_names");
  if (lua_istable(L, -1)) {
      extensionCount = (uint32_t)lua_objlen(L, -1);
      if (extensionCount > 0) {
          extensionNames = malloc(extensionCount * sizeof(const char *));
          for (uint32_t i = 0; i < extensionCount; i++) {
              lua_rawgeti(L, -1, i + 1);
              extensionNames[i] = luaL_checkstring(L, -1);
              lua_pop(L, 1);
          }
      }
  }
  lua_pop(L, 1);

  VkInstanceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = layerCount,
      .ppEnabledLayerNames = layerNames,
      .enabledExtensionCount = extensionCount,
      .ppEnabledExtensionNames = extensionNames,
  };

  VkInstance instance;
  VkResult result = vkCreateInstance(&createInfo, NULL, &instance);

  free(layerNames);
  free(extensionNames);

  if (result != VK_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to create Vulkan instance");
      return 2;
  }

  VulkanInstance *iptr = (VulkanInstance *)lua_newuserdata(L, sizeof(VulkanInstance));
  iptr->instance = instance;
  luaL_getmetatable(L, "VulkanInstance");
  lua_setmetatable(L, -2);
  return 1;
}

static int l_vk_EnumeratePhysicalDevices(lua_State *L) {
  VulkanInstance *iptr = (VulkanInstance *)luaL_checkudata(L, 1, "VulkanInstance");
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(iptr->instance, &deviceCount, NULL);

  if (deviceCount == 0) {
      lua_pushnil(L);
      lua_pushstring(L, "No physical devices found");
      return 2;
  }

  VkPhysicalDevice *devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
  vkEnumeratePhysicalDevices(iptr->instance, &deviceCount, devices);

  lua_newtable(L);
  for (uint32_t i = 0; i < deviceCount; i++) {
      VulkanPhysicalDevice *dptr = (VulkanPhysicalDevice *)lua_newuserdata(L, sizeof(VulkanPhysicalDevice));
      dptr->physicalDevice = devices[i];
      luaL_getmetatable(L, "VulkanPhysicalDevice");
      lua_setmetatable(L, -2);
      lua_rawseti(L, -2, i + 1);
  }
  free(devices);
  return 1;
}

static int l_vk_GetPhysicalDeviceProperties(lua_State *L) {
    VulkanPhysicalDevice *dptr = (VulkanPhysicalDevice *)luaL_checkudata(L, 1, "VulkanPhysicalDevice");
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(dptr->physicalDevice, &props);

    lua_newtable(L);
    lua_pushstring(L, props.deviceName);
    lua_setfield(L, -2, "deviceName");
    lua_pushinteger(L, props.apiVersion);
    lua_setfield(L, -2, "apiVersion");
    lua_pushinteger(L, props.driverVersion);
    lua_setfield(L, -2, "driverVersion");
    return 1;
}

static int l_vk_GetPhysicalDeviceQueueFamilyProperties(lua_State *L) {
  VulkanPhysicalDevice *dptr = (VulkanPhysicalDevice *)luaL_checkudata(L, 1, "VulkanPhysicalDevice");
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(dptr->physicalDevice, &queueFamilyCount, NULL);

  if (queueFamilyCount == 0) {
      lua_pushnil(L);
      lua_pushstring(L, "No queue families found");
      return 2;
  }

  VkQueueFamilyProperties *queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(dptr->physicalDevice, &queueFamilyCount, queueFamilies);

  lua_newtable(L);
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
      lua_newtable(L);
      lua_pushinteger(L, queueFamilies[i].queueCount);
      lua_setfield(L, -2, "queueCount");
      lua_pushinteger(L, queueFamilies[i].queueFlags);
      lua_setfield(L, -2, "queueFlags");
      lua_rawseti(L, -2, i + 1); // 1-based indexing for Lua
  }
  free(queueFamilies);
  return 1;
}

static int l_vk_GetPhysicalDeviceSurfaceSupportKHR(lua_State *L) {
  VulkanPhysicalDevice *dptr = (VulkanPhysicalDevice *)luaL_checkudata(L, 1, "VulkanPhysicalDevice");
  uint32_t queueFamilyIndex = (uint32_t)luaL_checkinteger(L, 2);
  VulkanSurface *sptr = (VulkanSurface *)luaL_checkudata(L, 3, "VulkanSurface");

  VkBool32 supported;
  VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(dptr->physicalDevice, queueFamilyIndex, sptr->surface, &supported);

  if (result != VK_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to check surface support");
      return 2;
  }

  lua_pushboolean(L, supported);
  return 1;
}

static int l_vk_CreateDevice(lua_State *L) {
  VulkanPhysicalDevice *dptr = (VulkanPhysicalDevice *)luaL_checkudata(L, 1, "VulkanPhysicalDevice");
  VulkanSurface *sptr = (VulkanSurface *)luaL_checkudata(L, 2, "VulkanSurface");
  luaL_checktype(L, 3, LUA_TTABLE); // Configuration table

  // Get queue family properties
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(dptr->physicalDevice, &queueFamilyCount, NULL);
  if (queueFamilyCount == 0) {
      lua_pushnil(L);
      lua_pushstring(L, "No queue families available");
      return 2;
  }

  VkQueueFamilyProperties *queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(dptr->physicalDevice, &queueFamilyCount, queueFamilies);

  // Find graphics and present queue families
  uint32_t graphicsFamily = UINT32_MAX;
  uint32_t presentFamily = UINT32_MAX;
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
          graphicsFamily = i;
      }
      VkBool32 presentSupport = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(dptr->physicalDevice, i, sptr->surface, &presentSupport);
      if (presentSupport) {
          presentFamily = i;
      }
      if (graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX) {
          break;
      }
  }
  free(queueFamilies);

  if (graphicsFamily == UINT32_MAX) {
      lua_pushnil(L);
      lua_pushstring(L, "No graphics queue family found");
      return 2;
  }
  if (presentFamily == UINT32_MAX) {
      lua_pushnil(L);
      lua_pushstring(L, "No present queue family found");
      return 2;
  }

  // Queue creation
  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfos[2];
  uint32_t queueCreateInfoCount = 0;

  queueCreateInfos[0] = (VkDeviceQueueCreateInfo){
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = graphicsFamily,
      .queueCount = 1,
      .pQueuePriorities = &queuePriority,
  };
  queueCreateInfoCount++;

  if (graphicsFamily != presentFamily) {
      queueCreateInfos[1] = (VkDeviceQueueCreateInfo){
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .queueFamilyIndex = presentFamily,
          .queueCount = 1,
          .pQueuePriorities = &queuePriority,
      };
      queueCreateInfoCount++;
  }

  // Device extensions from Lua table
  uint32_t extensionCount = 0;
  const char **extensionNames = NULL;
  lua_getfield(L, 3, "enabled_extension_names");
  if (lua_istable(L, -1)) {
      extensionCount = (uint32_t)lua_objlen(L, -1);
      if (extensionCount > 0) {
          extensionNames = malloc(extensionCount * sizeof(const char *));
          for (uint32_t i = 0; i < extensionCount; i++) {
              lua_rawgeti(L, -1, i + 1);
              extensionNames[i] = luaL_checkstring(L, -1);
              lua_pop(L, 1);
          }
      }
  }
  lua_pop(L, 1);

  VkPhysicalDeviceFeatures deviceFeatures = {0};

  VkDeviceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = queueCreateInfoCount,
      .pQueueCreateInfos = queueCreateInfos,
      .enabledExtensionCount = extensionCount,
      .ppEnabledExtensionNames = extensionNames,
      .pEnabledFeatures = &deviceFeatures,
  };

  VkDevice device;
  VkResult result = vkCreateDevice(dptr->physicalDevice, &createInfo, NULL, &device);

  free(extensionNames);

  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkCreateDevice failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  VulkanDevice *devptr = (VulkanDevice *)lua_newuserdata(L, sizeof(VulkanDevice));
  devptr->device = device;
  luaL_getmetatable(L, "VulkanDevice");
  lua_setmetatable(L, -2);

  lua_pushinteger(L, graphicsFamily);
  lua_pushinteger(L, presentFamily);
  return 3;
}

static int l_vk_GetDeviceQueue(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  uint32_t queueFamilyIndex = (uint32_t)luaL_checkinteger(L, 2);
  uint32_t queueIndex = (uint32_t)luaL_checkinteger(L, 3);

  VkQueue queue;
  vkGetDeviceQueue(dptr->device, queueFamilyIndex, queueIndex, &queue);
  if (queue == VK_NULL_HANDLE) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to get device queue");
      return 2;
  }

  VulkanQueue *qptr = (VulkanQueue *)lua_newuserdata(L, sizeof(VulkanQueue));
  qptr->queue = queue;
  luaL_getmetatable(L, "VulkanQueue");
  lua_setmetatable(L, -2);
  return 1;
}

static int l_vk_GetPhysicalDeviceSurfaceCapabilitiesKHR(lua_State *L) {
  VulkanPhysicalDevice *dptr = (VulkanPhysicalDevice *)luaL_checkudata(L, 1, "VulkanPhysicalDevice");
  VulkanSurface *sptr = (VulkanSurface *)luaL_checkudata(L, 2, "VulkanSurface");

  VkSurfaceCapabilitiesKHR caps;
  VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dptr->physicalDevice, sptr->surface, &caps);
  if (result != VK_SUCCESS) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to get surface capabilities");
      return 2;
  }

  lua_newtable(L);
  lua_pushinteger(L, caps.minImageCount);
  lua_setfield(L, -2, "minImageCount");
  lua_pushinteger(L, caps.maxImageCount);
  lua_setfield(L, -2, "maxImageCount");
  lua_pushinteger(L, caps.currentExtent.width);
  lua_setfield(L, -2, "currentWidth");
  lua_pushinteger(L, caps.currentExtent.height);
  lua_setfield(L, -2, "currentHeight");
  return 1;
}

static int l_vk_GetPhysicalDeviceSurfaceFormatsKHR(lua_State *L) {
  VulkanPhysicalDevice *dptr = (VulkanPhysicalDevice *)luaL_checkudata(L, 1, "VulkanPhysicalDevice");
  VulkanSurface *sptr = (VulkanSurface *)luaL_checkudata(L, 2, "VulkanSurface");

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(dptr->physicalDevice, sptr->surface, &formatCount, NULL);
  if (formatCount == 0) {
      lua_pushnil(L);
      lua_pushstring(L, "No surface formats available");
      return 2;
  }

  VkSurfaceFormatKHR *formats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
  vkGetPhysicalDeviceSurfaceFormatsKHR(dptr->physicalDevice, sptr->surface, &formatCount, formats);

  lua_newtable(L);
  for (uint32_t i = 0; i < formatCount; i++) {
      lua_newtable(L);
      lua_pushinteger(L, formats[i].format);
      lua_setfield(L, -2, "format");
      lua_pushinteger(L, formats[i].colorSpace);
      lua_setfield(L, -2, "colorSpace");
      lua_rawseti(L, -2, i + 1);
  }
  free(formats);
  return 1;
}

static int l_vk_GetPhysicalDeviceSurfacePresentModesKHR(lua_State *L) {
  VulkanPhysicalDevice *dptr = (VulkanPhysicalDevice *)luaL_checkudata(L, 1, "VulkanPhysicalDevice");
  VulkanSurface *sptr = (VulkanSurface *)luaL_checkudata(L, 2, "VulkanSurface");

  uint32_t modeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(dptr->physicalDevice, sptr->surface, &modeCount, NULL);
  if (modeCount == 0) {
      lua_pushnil(L);
      lua_pushstring(L, "No present modes available");
      return 2;
  }

  VkPresentModeKHR *modes = malloc(modeCount * sizeof(VkPresentModeKHR));
  vkGetPhysicalDeviceSurfacePresentModesKHR(dptr->physicalDevice, sptr->surface, &modeCount, modes);

  lua_newtable(L);
  for (uint32_t i = 0; i < modeCount; i++) {
      lua_pushinteger(L, modes[i]);
      lua_rawseti(L, -2, i + 1);
  }
  free(modes);
  return 1;
}

static int l_vk_CreateSwapchainKHR(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  luaL_checktype(L, 2, LUA_TTABLE);

  VkSwapchainCreateInfoKHR createInfo = { .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
  lua_getfield(L, 2, "surface");
  VulkanSurface *sptr = (VulkanSurface *)luaL_checkudata(L, -1, "VulkanSurface");
  createInfo.surface = sptr->surface;
  lua_pop(L, 1);

  lua_getfield(L, 2, "minImageCount");
  createInfo.minImageCount = (uint32_t)luaL_checkinteger(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, 2, "imageFormat");
  createInfo.imageFormat = (VkFormat)luaL_checkinteger(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, 2, "imageColorSpace");
  createInfo.imageColorSpace = (VkColorSpaceKHR)luaL_checkinteger(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, 2, "imageExtentWidth");
  createInfo.imageExtent.width = (uint32_t)luaL_checkinteger(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, 2, "imageExtentHeight");
  createInfo.imageExtent.height = (uint32_t)luaL_checkinteger(L, -1);
  lua_pop(L, 1);

  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  lua_getfield(L, 2, "queueFamilyIndices");
  if (lua_istable(L, -1)) {
      uint32_t count = (uint32_t)lua_objlen(L, -1);
      uint32_t *indices = malloc(count * sizeof(uint32_t));
      for (uint32_t i = 0; i < count; i++) {
          lua_rawgeti(L, -1, i + 1);
          indices[i] = (uint32_t)luaL_checkinteger(L, -1);
          lua_pop(L, 1);
      }
      createInfo.queueFamilyIndexCount = count;
      createInfo.pQueueFamilyIndices = indices;

      if (count > 1) {
          createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      } else {
          createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      }
      lua_pop(L, 1);
  } else {
      lua_pop(L, 1);
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      createInfo.queueFamilyIndexCount = 0;
      createInfo.pQueueFamilyIndices = NULL;
  }

  createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  lua_getfield(L, 2, "presentMode");
  createInfo.presentMode = (VkPresentModeKHR)luaL_checkinteger(L, -1);
  lua_pop(L, 1);

  createInfo.clipped = VK_TRUE;

  VkSwapchainKHR swapchain;
  VkResult result = vkCreateSwapchainKHR(dptr->device, &createInfo, NULL, &swapchain);

  if (createInfo.pQueueFamilyIndices) {
      free((void *)createInfo.pQueueFamilyIndices);
  }

  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkCreateSwapchainKHR failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  VulkanSwapchain *swptr = (VulkanSwapchain *)lua_newuserdata(L, sizeof(VulkanSwapchain));
  swptr->swapchain = swapchain;
  swptr->device = dptr->device;
  luaL_getmetatable(L, "VulkanSwapchain");
  lua_setmetatable(L, -2);
  return 1;
}

static int l_vk_GetSwapchainImagesKHR(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanSwapchain *swptr = (VulkanSwapchain *)luaL_checkudata(L, 2, "VulkanSwapchain");

  uint32_t imageCount;
  VkResult result = vkGetSwapchainImagesKHR(dptr->device, swptr->swapchain, &imageCount, NULL);
  if (result != VK_SUCCESS || imageCount == 0) {
      lua_pushnil(L);
      lua_pushstring(L, "Failed to get swapchain image count");
      return 2;
  }

  VkImage *images = malloc(imageCount * sizeof(VkImage));
  result = vkGetSwapchainImagesKHR(dptr->device, swptr->swapchain, &imageCount, images);
  if (result != VK_SUCCESS) {
      free(images);
      lua_pushnil(L);
      lua_pushstring(L, "Failed to retrieve swapchain images");
      return 2;
  }

  lua_newtable(L);
  for (uint32_t i = 0; i < imageCount; i++) {
      VulkanImage *imgptr = (VulkanImage *)lua_newuserdata(L, sizeof(VulkanImage));
      imgptr->image = images[i];
      luaL_getmetatable(L, "VulkanImage");
      lua_setmetatable(L, -2);
      lua_rawseti(L, -2, i + 1);
  }
  free(images);
  return 1;
}

static int l_vk_CreateImageView(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  luaL_checktype(L, 2, LUA_TTABLE);

  VkImageViewCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
  lua_getfield(L, 2, "image");
  VulkanImage *imgptr = (VulkanImage *)luaL_checkudata(L, -1, "VulkanImage");
  createInfo.image = imgptr->image;
  lua_pop(L, 1);

  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  lua_getfield(L, 2, "format");
  createInfo.format = (VkFormat)luaL_checkinteger(L, -1);
  lua_pop(L, 1);

  createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  createInfo.subresourceRange.baseMipLevel = 0;
  createInfo.subresourceRange.levelCount = 1;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount = 1;

  VkImageView imageView;
  VkResult result = vkCreateImageView(dptr->device, &createInfo, NULL, &imageView);
  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkCreateImageView failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  VulkanImageView *viewptr = (VulkanImageView *)lua_newuserdata(L, sizeof(VulkanImageView));
  viewptr->imageView = imageView;
  viewptr->device = dptr->device;
  luaL_getmetatable(L, "VulkanImageView");
  lua_setmetatable(L, -2);
  return 1;
}

static int l_vk_CreateRenderPass(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  luaL_checktype(L, 2, LUA_TTABLE);

  VkAttachmentDescription colorAttachment = {0};
  lua_getfield(L, 2, "format");
  colorAttachment.format = (VkFormat)luaL_checkinteger(L, -1);
  lua_pop(L, 1);
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  };

  VkSubpassDescription subpass = {0};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &colorAttachment,
      .subpassCount = 1,
      .pSubpasses = &subpass
  };

  VkRenderPass renderPass;
  VkResult result = vkCreateRenderPass(dptr->device, &createInfo, NULL, &renderPass);
  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkCreateRenderPass failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  VulkanRenderPass *rpptr = (VulkanRenderPass *)lua_newuserdata(L, sizeof(VulkanRenderPass));
  rpptr->renderPass = renderPass;
  rpptr->device = dptr->device;
  luaL_getmetatable(L, "VulkanRenderPass");
  lua_setmetatable(L, -2);
  return 1;
}

static int l_vk_CreateFramebuffer(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  luaL_checktype(L, 2, LUA_TTABLE);

  VkFramebufferCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
  lua_getfield(L, 2, "renderPass");
  VulkanRenderPass *rpptr = (VulkanRenderPass *)luaL_checkudata(L, -1, "VulkanRenderPass");
  createInfo.renderPass = rpptr->renderPass;
  lua_pop(L, 1);

  uint32_t attachmentCount = 0;
  VkImageView *attachments = NULL;
  lua_getfield(L, 2, "attachments");
  if (lua_istable(L, -1)) {
      attachmentCount = (uint32_t)lua_objlen(L, -1);
      attachments = malloc(attachmentCount * sizeof(VkImageView));
      for (uint32_t i = 0; i < attachmentCount; i++) {
          lua_rawgeti(L, -1, i + 1);
          VulkanImageView *viewptr = (VulkanImageView *)luaL_checkudata(L, -1, "VulkanImageView");
          attachments[i] = viewptr->imageView;
          lua_pop(L, 1);
      }
  }
  lua_pop(L, 1);
  createInfo.attachmentCount = attachmentCount;
  createInfo.pAttachments = attachments;

  lua_getfield(L, 2, "width");
  createInfo.width = (uint32_t)luaL_checkinteger(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, 2, "height");
  createInfo.height = (uint32_t)luaL_checkinteger(L, -1);
  lua_pop(L, 1);

  createInfo.layers = 1;

  VkFramebuffer framebuffer;
  VkResult result = vkCreateFramebuffer(dptr->device, &createInfo, NULL, &framebuffer);
  free(attachments);

  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkCreateFramebuffer failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  VulkanFramebuffer *fbptr = (VulkanFramebuffer *)lua_newuserdata(L, sizeof(VulkanFramebuffer));
  fbptr->framebuffer = framebuffer;
  fbptr->device = dptr->device;
  luaL_getmetatable(L, "VulkanFramebuffer");
  lua_setmetatable(L, -2);
  return 1;
}


static int l_vk_CreateShaderModule(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  size_t codeSize;
  const char *code = luaL_checklstring(L, 2, &codeSize);

  VkShaderModuleCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = codeSize,
      .pCode = (const uint32_t *)code
  };

  VkShaderModule shaderModule;
  VkResult result = vkCreateShaderModule(dptr->device, &createInfo, NULL, &shaderModule);
  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkCreateShaderModule failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  VulkanShaderModule *smptr = (VulkanShaderModule *)lua_newuserdata(L, sizeof(VulkanShaderModule));
  smptr->shaderModule = shaderModule;
  smptr->device = dptr->device;
  luaL_getmetatable(L, "VulkanShaderModule");
  lua_setmetatable(L, -2);
  return 1;
}

static int l_vk_CreatePipelineLayout(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");

  VkPipelineLayoutCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pSetLayouts = NULL
  };

  VkPipelineLayout pipelineLayout;
  VkResult result = vkCreatePipelineLayout(dptr->device, &createInfo, NULL, &pipelineLayout);
  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkCreatePipelineLayout failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  VulkanPipelineLayout *plptr = (VulkanPipelineLayout *)lua_newuserdata(L, sizeof(VulkanPipelineLayout));
  plptr->pipelineLayout = pipelineLayout;
  plptr->device = dptr->device;
  luaL_getmetatable(L, "VulkanPipelineLayout");
  lua_setmetatable(L, -2);
  return 1;
}

static int l_vk_CreateGraphicsPipelines(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  luaL_checktype(L, 2, LUA_TTABLE);

  VkPipelineShaderStageCreateInfo shaderStages[2];
  memset(shaderStages, 0, sizeof(shaderStages));
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  lua_getfield(L, 2, "vertexShader");
  VulkanShaderModule *vertShader = (VulkanShaderModule *)luaL_checkudata(L, -1, "VulkanShaderModule");
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].module = vertShader->shaderModule;
  shaderStages[0].pName = "main";
  lua_pop(L, 1);

  lua_getfield(L, 2, "fragmentShader");
  VulkanShaderModule *fragShader = (VulkanShaderModule *)luaL_checkudata(L, -1, "VulkanShaderModule");
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].module = fragShader->shaderModule;
  shaderStages[1].pName = "main";
  lua_pop(L, 1);

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 0,
      .pVertexBindingDescriptions = NULL,
      .vertexAttributeDescriptionCount = 0,
      .pVertexAttributeDescriptions = NULL
  };

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE
  };

  VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = 800.0f,
      .height = 600.0f,
      .minDepth = 0.0f,
      .maxDepth = 1.0f
  };

  VkRect2D scissor = {
      .offset = {0, 0},
      .extent = {800, 600}
  };

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

  lua_getfield(L, 2, "pipelineLayout");
  VulkanPipelineLayout *plptr = (VulkanPipelineLayout *)luaL_checkudata(L, -1, "VulkanPipelineLayout");
  lua_pop(L, 1);

  lua_getfield(L, 2, "renderPass");
  VulkanRenderPass *rpptr = (VulkanRenderPass *)luaL_checkudata(L, -1, "VulkanRenderPass");
  lua_pop(L, 1);

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
      .layout = plptr->pipelineLayout,
      .renderPass = rpptr->renderPass,
      .subpass = 0
  };

  VkPipeline graphicsPipeline;
  VkResult result = vkCreateGraphicsPipelines(dptr->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline);
  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkCreateGraphicsPipelines failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  VulkanPipeline *pptr = (VulkanPipeline *)lua_newuserdata(L, sizeof(VulkanPipeline));
  pptr->pipeline = graphicsPipeline;
  pptr->device = dptr->device;
  luaL_getmetatable(L, "VulkanPipeline");
  lua_setmetatable(L, -2);
  return 1;
}

static int l_vk_CreateSemaphore(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");

  VkSemaphoreCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
  };

  VkSemaphore semaphore;
  VkResult result = vkCreateSemaphore(dptr->device, &createInfo, NULL, &semaphore);
  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkCreateSemaphore failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  VulkanSemaphore *sptr = (VulkanSemaphore *)lua_newuserdata(L, sizeof(VulkanSemaphore));
  sptr->semaphore = semaphore;
  sptr->device = dptr->device;
  luaL_getmetatable(L, "VulkanSemaphore");
  lua_setmetatable(L, -2);
  return 1;
}

static int l_vk_CreateFence(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  int signaled = lua_toboolean(L, 2);

  VkFenceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0
  };

  VkFence fence;
  VkResult result = vkCreateFence(dptr->device, &createInfo, NULL, &fence);
  if (result != VK_SUCCESS) {  // Fixed typo: removed duplicate "result"
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkCreateFence failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  VulkanFence *fptr = (VulkanFence *)lua_newuserdata(L, sizeof(VulkanFence));
  fptr->fence = fence;
  fptr->device = dptr->device;
  luaL_getmetatable(L, "VulkanFence");
  lua_setmetatable(L, -2);
  return 1;
}

static int l_vk_AcquireNextImageKHR(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanSwapchain *swptr = (VulkanSwapchain *)luaL_checkudata(L, 2, "VulkanSwapchain");
  uint64_t timeout = luaL_optinteger(L, 3, UINT64_MAX);

  VulkanSemaphore *semaphore = NULL;
  if (lua_type(L, 4) == LUA_TUSERDATA) {
      semaphore = (VulkanSemaphore *)luaL_checkudata(L, 4, "VulkanSemaphore");
  }

  VulkanFence *fence = NULL;
  if (lua_type(L, 5) == LUA_TUSERDATA) {
      fence = (VulkanFence *)luaL_checkudata(L, 5, "VulkanFence");
  }

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(dptr->device, swptr->swapchain, timeout,
      semaphore ? semaphore->semaphore : VK_NULL_HANDLE,
      fence ? fence->fence : VK_NULL_HANDLE,
      &imageIndex);

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkAcquireNextImageKHR failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  lua_pushinteger(L, imageIndex);
  return 1;
}


static int l_vk_QueueSubmit(lua_State *L) {
  VulkanQueue *qptr = (VulkanQueue *)luaL_checkudata(L, 1, "VulkanQueue");
  luaL_checktype(L, 2, LUA_TTABLE);
  VulkanFence *fence = NULL;
  if (lua_type(L, 3) == LUA_TUSERDATA) {
      fence = (VulkanFence *)luaL_checkudata(L, 3, "VulkanFence");
  }

  uint32_t submitCount = (uint32_t)lua_objlen(L, 2);
  VkSubmitInfo *submitInfos = malloc(submitCount * sizeof(VkSubmitInfo));
  for (uint32_t i = 0; i < submitCount; i++) {
      lua_rawgeti(L, 2, i + 1);
      luaL_checktype(L, -1, LUA_TTABLE);

      submitInfos[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfos[i].pNext = NULL;

      uint32_t waitSemaphoreCount = 0;
      VkSemaphore *waitSemaphores = NULL;
      lua_getfield(L, -1, "waitSemaphores");
      if (lua_istable(L, -1)) {
          waitSemaphoreCount = (uint32_t)lua_objlen(L, -1);
          waitSemaphores = malloc(waitSemaphoreCount * sizeof(VkSemaphore));
          for (uint32_t j = 0; j < waitSemaphoreCount; j++) {
              lua_rawgeti(L, -1, j + 1);
              VulkanSemaphore *sptr = (VulkanSemaphore *)luaL_checkudata(L, -1, "VulkanSemaphore");
              waitSemaphores[j] = sptr->semaphore;
              lua_pop(L, 1);
          }
      }
      lua_pop(L, 1);
      submitInfos[i].waitSemaphoreCount = waitSemaphoreCount;
      submitInfos[i].pWaitSemaphores = waitSemaphores;

      VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
      submitInfos[i].pWaitDstStageMask = waitStages;

      uint32_t commandBufferCount = 0;
      VkCommandBuffer *commandBuffers = NULL;
      lua_getfield(L, -1, "commandBuffers");
      if (lua_istable(L, -1)) {
          commandBufferCount = (uint32_t)lua_objlen(L, -1);
          commandBuffers = malloc(commandBufferCount * sizeof(VkCommandBuffer));
          for (uint32_t j = 0; j < commandBufferCount; j++) {
              lua_rawgeti(L, -1, j + 1);
              commandBuffers[j] = (VkCommandBuffer)lua_touserdata(L, -1); // Assuming command buffers as light userdata
              lua_pop(L, 1);
          }
      }
      lua_pop(L, 1);
      submitInfos[i].commandBufferCount = commandBufferCount;
      submitInfos[i].pCommandBuffers = commandBuffers;

      uint32_t signalSemaphoreCount = 0;
      VkSemaphore *signalSemaphores = NULL;
      lua_getfield(L, -1, "signalSemaphores");
      if (lua_istable(L, -1)) {
          signalSemaphoreCount = (uint32_t)lua_objlen(L, -1);
          signalSemaphores = malloc(signalSemaphoreCount * sizeof(VkSemaphore));
          for (uint32_t j = 0; j < signalSemaphoreCount; j++) {
              lua_rawgeti(L, -1, j + 1);
              VulkanSemaphore *sptr = (VulkanSemaphore *)luaL_checkudata(L, -1, "VulkanSemaphore");
              signalSemaphores[j] = sptr->semaphore;
              lua_pop(L, 1);
          }
      }
      lua_pop(L, 1);
      submitInfos[i].signalSemaphoreCount = signalSemaphoreCount;
      submitInfos[i].pSignalSemaphores = signalSemaphores;

      lua_pop(L, 1); // Pop the submit info table
  }

  VkResult result = vkQueueSubmit(qptr->queue, submitCount, submitInfos, fence ? fence->fence : VK_NULL_HANDLE);

  for (uint32_t i = 0; i < submitCount; i++) {
      free((void *)submitInfos[i].pWaitSemaphores);
      free((void *)submitInfos[i].pCommandBuffers);
      free((void *)submitInfos[i].pSignalSemaphores);
  }
  free(submitInfos);

  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkQueueSubmit failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  lua_pushboolean(L, true);
  return 1;
}


static int l_vk_QueuePresentKHR(lua_State *L) {
  VulkanQueue *qptr = (VulkanQueue *)luaL_checkudata(L, 1, "VulkanQueue");
  luaL_checktype(L, 2, LUA_TTABLE);

  VkPresentInfoKHR presentInfo = { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };

  uint32_t waitSemaphoreCount = 0;
  VkSemaphore *waitSemaphores = NULL;
  lua_getfield(L, 2, "waitSemaphores");
  if (lua_istable(L, -1)) {
      waitSemaphoreCount = (uint32_t)lua_objlen(L, -1);
      waitSemaphores = malloc(waitSemaphoreCount * sizeof(VkSemaphore));
      for (uint32_t i = 0; i < waitSemaphoreCount; i++) {
          lua_rawgeti(L, -1, i + 1);
          VulkanSemaphore *sptr = (VulkanSemaphore *)luaL_checkudata(L, -1, "VulkanSemaphore");
          waitSemaphores[i] = sptr->semaphore;
          lua_pop(L, 1);
      }
  }
  lua_pop(L, 1);
  presentInfo.waitSemaphoreCount = waitSemaphoreCount;
  presentInfo.pWaitSemaphores = waitSemaphores;

  uint32_t swapchainCount = 0;
  VkSwapchainKHR *swapchains = NULL;
  uint32_t *imageIndices = NULL;
  lua_getfield(L, 2, "swapchains");
  if (lua_istable(L, -1)) {
      swapchainCount = (uint32_t)lua_objlen(L, -1);
      swapchains = malloc(swapchainCount * sizeof(VkSwapchainKHR));
      imageIndices = malloc(swapchainCount * sizeof(uint32_t));
      for (uint32_t i = 0; i < swapchainCount; i++) {
          lua_rawgeti(L, -1, i + 1);
          luaL_checktype(L, -1, LUA_TTABLE);
          lua_getfield(L, -1, "swapchain");
          VulkanSwapchain *swptr = (VulkanSwapchain *)luaL_checkudata(L, -1, "VulkanSwapchain");
          swapchains[i] = swptr->swapchain;
          lua_pop(L, 1);
          lua_getfield(L, -1, "imageIndex");
          imageIndices[i] = (uint32_t)luaL_checkinteger(L, -1);
          lua_pop(L, 1);
          lua_pop(L, 1);
      }
  }
  lua_pop(L, 1);
  presentInfo.swapchainCount = swapchainCount;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = imageIndices;

  VkResult result = vkQueuePresentKHR(qptr->queue, &presentInfo);

  free(waitSemaphores);
  free(swapchains);
  free(imageIndices);

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkQueuePresentKHR failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  lua_pushboolean(L, true);
  return 1;
}

static int l_vk_CreateCommandPool(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  uint32_t queueFamilyIndex = (uint32_t)luaL_checkinteger(L, 2);

  VkCommandPoolCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = queueFamilyIndex,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
  };

  VkCommandPool commandPool;
  VkResult result = vkCreateCommandPool(dptr->device, &createInfo, NULL, &commandPool);
  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkCreateCommandPool failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  VulkanCommandPool *cpptr = (VulkanCommandPool *)lua_newuserdata(L, sizeof(VulkanCommandPool));
  cpptr->commandPool = commandPool;
  cpptr->device = dptr->device;
  luaL_getmetatable(L, "VulkanCommandPool");
  lua_setmetatable(L, -2);
  return 1;
}

static int l_vk_AllocateCommandBuffers(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanCommandPool *cpptr = (VulkanCommandPool *)luaL_checkudata(L, 2, "VulkanCommandPool");
  uint32_t count = (uint32_t)luaL_checkinteger(L, 3);

  VkCommandBufferAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = cpptr->commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = count
  };

  VkCommandBuffer *commandBuffers = malloc(count * sizeof(VkCommandBuffer));
  VkResult result = vkAllocateCommandBuffers(dptr->device, &allocInfo, commandBuffers);
  if (result != VK_SUCCESS) {
      free(commandBuffers);
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkAllocateCommandBuffers failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  lua_newtable(L);
  for (uint32_t i = 0; i < count; i++) {
      lua_pushlightuserdata(L, commandBuffers[i]);
      lua_rawseti(L, -2, i + 1);
  }
  free(commandBuffers); // Lua now owns references
  return 1;
}

static int l_vk_BeginCommandBuffer(lua_State *L) {
  VkCommandBuffer cmdBuffer = (VkCommandBuffer)lua_touserdata(L, 1);

  VkCommandBufferBeginInfo beginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  };

  VkResult result = vkBeginCommandBuffer(cmdBuffer, &beginInfo);
  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkBeginCommandBuffer failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  lua_pushboolean(L, true);
  return 1;
}

static int l_vk_CmdBeginRenderPass(lua_State *L) {
  VkCommandBuffer cmdBuffer = (VkCommandBuffer)lua_touserdata(L, 1);
  VulkanRenderPass *rpptr = (VulkanRenderPass *)luaL_checkudata(L, 2, "VulkanRenderPass");
  VulkanFramebuffer *fbptr = (VulkanFramebuffer *)luaL_checkudata(L, 3, "VulkanFramebuffer");

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  VkRenderPassBeginInfo renderPassInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = rpptr->renderPass,
      .framebuffer = fbptr->framebuffer,
      .renderArea = {{0, 0}, {800, 600}},
      .clearValueCount = 1,
      .pClearValues = &clearColor
  };

  vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  return 0;
}

static int l_vk_CmdBindPipeline(lua_State *L) {
  VkCommandBuffer cmdBuffer = (VkCommandBuffer)lua_touserdata(L, 1);
  VulkanPipeline *pptr = (VulkanPipeline *)luaL_checkudata(L, 2, "VulkanPipeline");

  vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pptr->pipeline);
  return 0;
}

static int l_vk_CmdDraw(lua_State *L) {
  VkCommandBuffer cmdBuffer = (VkCommandBuffer)lua_touserdata(L, 1);
  uint32_t vertexCount = (uint32_t)luaL_checkinteger(L, 2);
  uint32_t instanceCount = (uint32_t)luaL_checkinteger(L, 3);
  uint32_t firstVertex = (uint32_t)luaL_checkinteger(L, 4);
  uint32_t firstInstance = (uint32_t)luaL_checkinteger(L, 5);

  vkCmdDraw(cmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
  return 0;
}

static int l_vk_CmdEndRenderPass(lua_State *L) {
  VkCommandBuffer cmdBuffer = (VkCommandBuffer)lua_touserdata(L, 1);
  vkCmdEndRenderPass(cmdBuffer);
  return 0;
}

static int l_vk_EndCommandBuffer(lua_State *L) {
  VkCommandBuffer cmdBuffer = (VkCommandBuffer)lua_touserdata(L, 1);

  VkResult result = vkEndCommandBuffer(cmdBuffer);
  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkEndCommandBuffer failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  lua_pushboolean(L, true);
  return 1;
}

static int l_vk_WaitForFences(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanFence *fptr = (VulkanFence *)luaL_checkudata(L, 2, "VulkanFence");

  VkResult result = vkWaitForFences(dptr->device, 1, &fptr->fence, VK_TRUE, UINT64_MAX);
  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkWaitForFences failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  lua_pushboolean(L, true);
  return 1;
}

static int l_vk_ResetFences(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  VulkanFence *fptr = (VulkanFence *)luaL_checkudata(L, 2, "VulkanFence");

  VkResult result = vkResetFences(dptr->device, 1, &fptr->fence);
  if (result != VK_SUCCESS) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "vkResetFences failed with result %d", result);
      lua_pushnil(L);
      lua_pushstring(L, errMsg);
      return 2;
  }

  lua_pushboolean(L, true);
  return 1;
}

static int l_vk_commandpool_gc(lua_State *L) {
  VulkanCommandPool *cpptr = (VulkanCommandPool *)luaL_checkudata(L, 1, "VulkanCommandPool");
  if (cpptr->commandPool) {
      vkDestroyCommandPool(cpptr->device, cpptr->commandPool, NULL);
      cpptr->commandPool = VK_NULL_HANDLE;
  }
  return 0;
}


static int l_vk_swapchain_gc(lua_State *L) {
  VulkanSwapchain *swptr = (VulkanSwapchain *)luaL_checkudata(L, 1, "VulkanSwapchain");
  if (swptr->swapchain) {
      vkDestroySwapchainKHR(swptr->device, swptr->swapchain, NULL);
      swptr->swapchain = VK_NULL_HANDLE;
  }
  return 0;
}

static int l_vk_imageview_gc(lua_State *L) {
  VulkanImageView *viewptr = (VulkanImageView *)luaL_checkudata(L, 1, "VulkanImageView");
  if (viewptr->imageView) {
      vkDestroyImageView(viewptr->device, viewptr->imageView, NULL);
      viewptr->imageView = VK_NULL_HANDLE;
  }
  return 0;
}

static int l_vk_renderpass_gc(lua_State *L) {
  VulkanRenderPass *rpptr = (VulkanRenderPass *)luaL_checkudata(L, 1, "VulkanRenderPass");
  if (rpptr->renderPass) {
      vkDestroyRenderPass(rpptr->device, rpptr->renderPass, NULL);
      rpptr->renderPass = VK_NULL_HANDLE;
  }
  return 0;
}

static int l_vk_framebuffer_gc(lua_State *L) {
  VulkanFramebuffer *fbptr = (VulkanFramebuffer *)luaL_checkudata(L, 1, "VulkanFramebuffer");
  if (fbptr->framebuffer) {
      vkDestroyFramebuffer(fbptr->device, fbptr->framebuffer, NULL);
      fbptr->framebuffer = VK_NULL_HANDLE;
  }
  return 0;
}

static int l_vk_shadermodule_gc(lua_State *L) {
  VulkanShaderModule *smptr = (VulkanShaderModule *)luaL_checkudata(L, 1, "VulkanShaderModule");
  if (smptr->shaderModule) {
      vkDestroyShaderModule(smptr->device, smptr->shaderModule, NULL);
      smptr->shaderModule = VK_NULL_HANDLE;
  }
  return 0;
}

static int l_vk_pipelinelayout_gc(lua_State *L) {
  VulkanPipelineLayout *plptr = (VulkanPipelineLayout *)luaL_checkudata(L, 1, "VulkanPipelineLayout");
  if (plptr->pipelineLayout) {
      vkDestroyPipelineLayout(plptr->device, plptr->pipelineLayout, NULL);
      plptr->pipelineLayout = VK_NULL_HANDLE;
  }
  return 0;
}

static int l_vk_pipeline_gc(lua_State *L) {
  VulkanPipeline *pptr = (VulkanPipeline *)luaL_checkudata(L, 1, "VulkanPipeline");
  if (pptr->pipeline) {
      vkDestroyPipeline(pptr->device, pptr->pipeline, NULL);
      pptr->pipeline = VK_NULL_HANDLE;
  }
  return 0;
}

static int l_vk_semaphore_gc(lua_State *L) {
  VulkanSemaphore *sptr = (VulkanSemaphore *)luaL_checkudata(L, 1, "VulkanSemaphore");
  if (sptr->semaphore) {
      vkDestroySemaphore(sptr->device, sptr->semaphore, NULL);
      sptr->semaphore = VK_NULL_HANDLE;
  }
  return 0;
}

static int l_vk_fence_gc(lua_State *L) {
  VulkanFence *fptr = (VulkanFence *)luaL_checkudata(L, 1, "VulkanFence");
  if (fptr->fence) {
      vkDestroyFence(fptr->device, fptr->fence, NULL);
      fptr->fence = VK_NULL_HANDLE;
  }
  return 0;
}

static int l_vk_instance_gc(lua_State *L) {
  VulkanInstance *iptr = (VulkanInstance *)luaL_checkudata(L, 1, "VulkanInstance");
  if (iptr->instance) {
      vkDestroyInstance(iptr->instance, NULL);
      iptr->instance = VK_NULL_HANDLE;
  }
  return 0;
}

static int l_vk_device_gc(lua_State *L) {
  VulkanDevice *dptr = (VulkanDevice *)luaL_checkudata(L, 1, "VulkanDevice");
  if (dptr->device) {
      vkDestroyDevice(dptr->device, NULL);
      dptr->device = VK_NULL_HANDLE;
  }
  return 0;
}


static int l_vk_EnumerateInstanceLayerProperties(lua_State *L) {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, NULL);
  VkLayerProperties *layers = malloc(layerCount * sizeof(VkLayerProperties));
  vkEnumerateInstanceLayerProperties(&layerCount, layers);

  lua_newtable(L);
  for (uint32_t i = 0; i < layerCount; i++) {
      lua_newtable(L);
      lua_pushstring(L, layers[i].layerName);
      lua_setfield(L, -2, "layerName");
      lua_pushinteger(L, layers[i].specVersion);
      lua_setfield(L, -2, "specVersion");
      lua_rawseti(L, -2, i + 1);
  }
  free(layers);
  return 1;
}

static const luaL_Reg instance_mt[] = {
    {"__gc", l_vk_instance_gc},
    {NULL, NULL}
};

static const luaL_Reg device_mt[] = {
    {"__gc", l_vk_device_gc},
    {NULL, NULL}
};

static const luaL_Reg queue_mt[] = {
  {NULL, NULL} // No cleanup needed; queues are tied to device lifetime
};

static const luaL_Reg swapchain_mt[] = {
  {"__gc", l_vk_swapchain_gc},
  {NULL, NULL}
};

static const luaL_Reg image_mt[] = {
  {NULL, NULL} // No cleanup needed; images are tied to swapchain
};

static const luaL_Reg imageview_mt[] = {
  {"__gc", l_vk_imageview_gc},
  {NULL, NULL}
};

static const luaL_Reg renderpass_mt[] = {
  {"__gc", l_vk_renderpass_gc},
  {NULL, NULL}
};

static const luaL_Reg framebuffer_mt[] = {
  {"__gc", l_vk_framebuffer_gc},
  {NULL, NULL}
};

static const luaL_Reg shadermodule_mt[] = {
  {"__gc", l_vk_shadermodule_gc},
  {NULL, NULL}
};

static const luaL_Reg pipelinelayout_mt[] = {
  {"__gc", l_vk_pipelinelayout_gc},
  {NULL, NULL}
};

static const luaL_Reg pipeline_mt[] = {
  {"__gc", l_vk_pipeline_gc},
  {NULL, NULL}
};

static const luaL_Reg semaphore_mt[] = {
  {"__gc", l_vk_semaphore_gc},
  {NULL, NULL}
};

static const luaL_Reg fence_mt[] = {
  {"__gc", l_vk_fence_gc},
  {NULL, NULL}
};


static const luaL_Reg vulkan_funcs[] = {
  {"vk_EnumerateInstanceLayerProperties", l_vk_EnumerateInstanceLayerProperties},
  {"create_instance", l_vk_CreateInstance},
  {"vk_CreateInstance", l_vk_CreateInstance},
  {"vk_EnumeratePhysicalDevices", l_vk_EnumeratePhysicalDevices},
  {"vk_GetPhysicalDeviceProperties", l_vk_GetPhysicalDeviceProperties},
  {"vk_GetPhysicalDeviceQueueFamilyProperties", l_vk_GetPhysicalDeviceQueueFamilyProperties},
  {"vk_GetPhysicalDeviceSurfaceSupportKHR", l_vk_GetPhysicalDeviceSurfaceSupportKHR},
  {"vk_CreateDevice", l_vk_CreateDevice},
  {"vk_GetDeviceQueue", l_vk_GetDeviceQueue},
  {"vk_GetPhysicalDeviceSurfaceCapabilitiesKHR", l_vk_GetPhysicalDeviceSurfaceCapabilitiesKHR},
  {"vk_GetPhysicalDeviceSurfaceFormatsKHR", l_vk_GetPhysicalDeviceSurfaceFormatsKHR},
  {"vk_GetPhysicalDeviceSurfacePresentModesKHR", l_vk_GetPhysicalDeviceSurfacePresentModesKHR},
  {"vk_CreateSwapchainKHR", l_vk_CreateSwapchainKHR},
  {"vk_GetSwapchainImagesKHR", l_vk_GetSwapchainImagesKHR},
  {"vk_CreateImageView", l_vk_CreateImageView},
  {"vk_CreateRenderPass", l_vk_CreateRenderPass},
  {"vk_CreateFramebuffer", l_vk_CreateFramebuffer},
  {"vk_CreateShaderModule", l_vk_CreateShaderModule},
  {"vk_CreatePipelineLayout", l_vk_CreatePipelineLayout},
  {"vk_CreateGraphicsPipelines", l_vk_CreateGraphicsPipelines},
  {"vk_CreateSemaphore", l_vk_CreateSemaphore},
  {"vk_CreateFence", l_vk_CreateFence},
  {"vk_AcquireNextImageKHR", l_vk_AcquireNextImageKHR},
  {"vk_QueueSubmit", l_vk_QueueSubmit},
  {"vk_QueuePresentKHR", l_vk_QueuePresentKHR},
  {"vk_CreateCommandPool", l_vk_CreateCommandPool},
  {"vk_AllocateCommandBuffers", l_vk_AllocateCommandBuffers},
  {"vk_BeginCommandBuffer", l_vk_BeginCommandBuffer},
  {"vk_CmdBeginRenderPass", l_vk_CmdBeginRenderPass},
  {"vk_CmdBindPipeline", l_vk_CmdBindPipeline},
  {"vk_CmdDraw", l_vk_CmdDraw},
  {"vk_CmdEndRenderPass", l_vk_CmdEndRenderPass},
  {"vk_EndCommandBuffer", l_vk_EndCommandBuffer},
  {"vk_WaitForFences", l_vk_WaitForFences},
  {"vk_ResetFences", l_vk_ResetFences},
  {NULL, NULL}
};

int luaopen_vulkan(lua_State *L) {
    // Register metatables
    luaL_newmetatable(L, "VulkanInstance");
    luaL_setfuncs(L, instance_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanPhysicalDevice");
    lua_pop(L, 1); // No methods for physical devices yet

    luaL_newmetatable(L, "VulkanDevice");
    luaL_setfuncs(L, device_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanQueue");
    luaL_setfuncs(L, queue_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanSwapchain");
    luaL_setfuncs(L, swapchain_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanImage");
    luaL_setfuncs(L, image_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanImageView");
    luaL_setfuncs(L, imageview_mt, 0);
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

    luaL_newmetatable(L, "VulkanPipelineLayout");
    luaL_setfuncs(L, pipelinelayout_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanPipeline");
    luaL_setfuncs(L, pipeline_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanSemaphore");
    luaL_setfuncs(L, semaphore_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanFence");
    luaL_setfuncs(L, fence_mt, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "VulkanCommandPool");
    luaL_setfuncs(L, (const luaL_Reg[]){{"__gc", l_vk_commandpool_gc}, {NULL, NULL}}, 0);
    lua_pop(L, 1);

    // Create the module table
    luaL_newlib(L, vulkan_funcs);

    // Add Vulkan constants and helpers
    lua_pushinteger(L, VK_API_VERSION_1_0);
    lua_setfield(L, -2, "VK_API_VERSION_1_0");
    lua_pushinteger(L, VK_QUEUE_GRAPHICS_BIT);
    lua_setfield(L, -2, "VK_QUEUE_GRAPHICS_BIT");
    lua_pushinteger(L, VK_FORMAT_B8G8R8A8_UNORM);
    lua_setfield(L, -2, "VK_FORMAT_B8G8R8A8_UNORM");
    lua_pushinteger(L, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
    lua_setfield(L, -2, "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR");
    lua_pushinteger(L, VK_PRESENT_MODE_FIFO_KHR);
    lua_setfield(L, -2, "VK_PRESENT_MODE_FIFO_KHR");

    lua_pushcfunction(L, l_vk_make_version);
    lua_setfield(L, -2, "make_version");
    return 1;
}