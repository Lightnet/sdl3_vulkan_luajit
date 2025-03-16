local sdl = require("SDL")
local vk = require("vulkan")

-- SDL setup
print("SDL_Init")
sdl.SDL_Init(sdl.SDL_INIT_VIDEO)
print("SDL_CreateWindow")
local window = assert(sdl.SDL_CreateWindow("Vulkan Triangle", 800, 600, sdl.SDL_WINDOW_VULKAN))

-- Vulkan instance extensions
print("SDL_Vulkan_GetInstanceExtensions")
local extCount = ffi.new("unsigned int[1]")
assert(sdl.SDL_Vulkan_GetInstanceExtensions(window, extCount, nil) == sdl.SDL_TRUE, "Failed to get extension count")
local extensions = ffi.new("const char*[?]", extCount[0])
assert(sdl.SDL_Vulkan_GetInstanceExtensions(window, extCount, extensions) == sdl.SDL_TRUE, "Failed to get extensions")
print("Number of extensions:", extCount[0])
local extTable = {}
for i = 0, extCount[0] - 1 do
    extTable[i + 1] = ffi.string(extensions[i])
    print("Required extension " .. (i + 1) .. ":", extTable[i + 1])
end

-- Create Vulkan instance
print("vulkan.create_instance")
local instance = assert(vk.vk_CreateInstance({
    enabled_extension_names = extTable,
    application_info = {
        application_name = "Vulkan Triangle",
        application_version = vk.make_version(1, 0, 0),
        engine_name = "LuaJIT Vulkan",
        engine_version = vk.make_version(1, 0, 0),
        api_version = vk.VK_API_VERSION_1_0
    }
}))

-- Create surface
print("SDL_Vulkan_CreateSurface")
local surfacePtr = ffi.new("VkSurfaceKHR[1]")
assert(sdl.SDL_Vulkan_CreateSurface(window, instance, surfacePtr) == sdl.SDL_TRUE, "Failed to create Vulkan surface")
local surface = surfacePtr[0]

-- Enumerate physical devices
local physicalDevices = assert(vk.vk_EnumeratePhysicalDevices(instance))
local physicalDevice = physicalDevices[1]
local props = vk.vk_GetPhysicalDeviceProperties(physicalDevice)
print("Using device:", props.deviceName)

-- Queue family setup
local queueFamilies = vk.vk_GetPhysicalDeviceQueueFamilyProperties(physicalDevice)
print("Queue families available:", #queueFamilies)
for i, family in ipairs(queueFamilies) do
    local presentSupport = vk.vk_GetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i - 1, surface)
    print(string.format("Queue Family %d: %d queues, flags: 0x%x", i, family.queueCount, family.queueFlags))
    print("  Supports presenting:", presentSupport)
end

-- Create device
local device, graphicsFamily, presentFamily = assert(vk.vk_CreateDevice(physicalDevice, surface, {
    enabled_extension_names = { "VK_KHR_swapchain" }
}))
print("vulkan.vk_CreateDevice")
print("Graphics queue family:", graphicsFamily)
print("Present queue family:", presentFamily)

-- Get queues
print("vulkan.vk_GetDeviceQueue (graphics)")
local graphicsQueue = vk.vk_GetDeviceQueue(device, graphicsFamily, 0)
print("vulkan.vk_GetDeviceQueue (present)")
local presentQueue = vk.vk_GetDeviceQueue(device, presentFamily, 0)

-- Surface capabilities
print("vulkan.vk_GetPhysicalDeviceSurfaceCapabilitiesKHR")
local caps = vk.vk_GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface)
print("Min image count:", caps.minImageCount, "Max image count:", caps.maxImageCount)
print("Current extent:", caps.currentWidth .. "x" .. caps.currentHeight)

-- Surface formats
print("vulkan.vk_GetPhysicalDeviceSurfaceFormatsKHR")
local formats = vk.vk_GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface)
for i, fmt in ipairs(formats) do
    print(string.format("Format %d: format=%d, colorSpace=%d", i, fmt.format, fmt.colorSpace))
end

-- Present modes
print("vulkan.vk_GetPhysicalDeviceSurfacePresentModesKHR")
local presentModes = vk.vk_GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface)
for i, mode in ipairs(presentModes) do
    print("Present mode " .. i .. ":", mode)
end

-- Create swapchain
print("vulkan.vk_CreateSwapchainKHR")
local swapchain = assert(vk.vk_CreateSwapchainKHR(device, {
    surface = surface,
    minImageCount = 2,
    imageFormat = vk.VK_FORMAT_B8G8R8A8_UNORM,
    imageColorSpace = vk.VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    imageExtentWidth = 800,
    imageExtentHeight = 600,
    queueFamilyIndices = { graphicsFamily },
    presentMode = vk.VK_PRESENT_MODE_FIFO_KHR
}))
print("Swapchain created successfully")

-- Get swapchain images
print("vulkan.vk_GetSwapchainImagesKHR")
local swapchainImages = assert(vk.vk_GetSwapchainImagesKHR(device, swapchain))
print("Number of swapchain images:", #swapchainImages)

-- Create image views
print("vulkan.vk_CreateImageView")
local imageViews = {}
for i, image in ipairs(swapchainImages) do
    imageViews[i] = assert(vk.vk_CreateImageView(device, {
        image = image,
        format = vk.VK_FORMAT_B8G8R8A8_UNORM
    }))
end
print("Created " .. #imageViews .. " image views")

-- Create render pass
print("vulkan.vk_CreateRenderPass")
local renderPass = assert(vk.vk_CreateRenderPass(device, {
    format = vk.VK_FORMAT_B8G8R8A8_UNORM
}))
print("Render pass created successfully")

-- Create framebuffers
print("vulkan.vk_CreateFramebuffer")
local framebuffers = {}
for i, view in ipairs(imageViews) do
    framebuffers[i] = assert(vk.vk_CreateFramebuffer(device, {
        renderPass = renderPass,
        attachments = { view },
        width = 800,
        height = 600
    }))
end
print("Created " .. #framebuffers .. " framebuffers")

-- Load shaders
print("Loading shaders")
local function readFile(path)
    local file = io.open(path, "rb")
    assert(file, "Failed to open " .. path)
    local data = file:read("*all")
    file:close()
    return data
end
local vertShaderCode = readFile("triangle.vert.spv")
local fragShaderCode = readFile("triangle.frag.spv")

-- Create shader modules
print("vulkan.vk_CreateShaderModule (vertex)")
local vertShaderModule = assert(vk.vk_CreateShaderModule(device, vertShaderCode))
print("vulkan.vk_CreateShaderModule (fragment)")
local fragShaderModule = assert(vk.vk_CreateShaderModule(device, fragShaderCode))

-- Create pipeline layout
print("vulkan.vk_CreatePipelineLayout")
local pipelineLayout = assert(vk.vk_CreatePipelineLayout(device))

-- Create graphics pipeline
print("vulkan.vk_CreateGraphicsPipelines")
local pipeline = assert(vk.vk_CreateGraphicsPipelines(device, {
    vertexShader = vertShaderModule,
    fragmentShader = fragShaderModule,
    pipelineLayout = pipelineLayout,
    renderPass = renderPass
}))

-- Create command pool and buffers (simplified, needs proper implementation)
print("Creating command pool and buffers")
-- These need to be exposed via vulkan_luajit.c:
-- vkCreateCommandPool, vkAllocateCommandBuffers, vkBeginCommandBuffer, vkCmdBeginRenderPass, etc.
-- For now, assume they're added (we'll update vulkan_luajit.c next)

-- Semaphores and fences
print("vulkan.vk_CreateSemaphore")
local imageAvailableSemaphore = assert(vk.vk_CreateSemaphore(device))
local renderFinishedSemaphore = assert(vk.vk_CreateSemaphore(device))
print("vulkan.vk_CreateFence")
local inFlightFence = assert(vk.vk_CreateFence(device, true))

-- Render loop
print("Entering render loop")
local running = true
while running do
    local event = ffi.new("SDL_Event")
    while sdl.SDL_PollEvent(event) ~= 0 do
        if event.type == sdl.SDL_QUIT then
            running = false
        end
    end

    -- Acquire image, submit command buffer, present (needs command buffer setup)
    local imageIndex = assert(vk.vk_AcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphore, nil))
    assert(vk.vk_QueueSubmit(graphicsQueue, {{
        waitSemaphores = { imageAvailableSemaphore },
        signalSemaphores = { renderFinishedSemaphore },
        commandBuffers = { --[[ command buffer here ]] }
    }}, inFlightFence))
    assert(vk.vk_QueuePresentKHR(presentQueue, {
        waitSemaphores = { renderFinishedSemaphore },
        swapchains = { { swapchain = swapchain, imageIndex = imageIndex } }
    }))
end

-- Cleanup
sdl.SDL_DestroyWindow(window)
sdl.SDL_Quit()