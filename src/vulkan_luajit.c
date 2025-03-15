#include "vulkan_luajit.h"
#include "lauxlib.h"
#include "lualib.h"

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

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = extensionCount,
        .ppEnabledExtensionNames = extensionNames,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL
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
    return 1;
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
        .format = VK_FORMAT_B8G8R8A8_UNORM,
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

    VkRenderPassCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass
    };

    VkRenderPass renderPass;
    VkResult result = vkCreateRenderPass(device_ptr->device, &createInfo, NULL, &renderPass);
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

    luaL_newlib(L, vulkan_funcs);
    lua_pushinteger(L, VK_API_VERSION_1_4);
    lua_setfield(L, -2, "API_VERSION_1_4");
    lua_pushinteger(L, VK_QUEUE_GRAPHICS_BIT);
    lua_setfield(L, -2, "QUEUE_GRAPHICS_BIT");
    lua_pushstring(L, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    lua_setfield(L, -2, "KHR_SWAPCHAIN_EXTENSION_NAME");
    return 1;
}