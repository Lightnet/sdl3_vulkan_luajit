local SDL = require("SDL")
local vulkan = require("vulkan")

-- Initialize SDL
print("SDL_Init")
local success, err = SDL.SDL_Init(SDL.SDL_INIT_VIDEO)
if not success then error("Failed to initialize SDL: " .. err) end

print("SDL_CreateWindow")
local window = SDL.SDL_CreateWindow("Vulkan Triangle", 800, 600, SDL.SDL_WINDOW_VULKAN)
if not window then error("Failed to create window: " .. SDL.SDL_GetError()) end

print("SDL_Vulkan_GetInstanceExtensions")
local count, extensions = SDL.SDL_Vulkan_GetInstanceExtensions()
if not count then error("Failed to get Vulkan instance extensions: " .. extensions) end
print("Number of extensions: " .. count)
for i, ext in ipairs(extensions) do print("Required extension " .. i .. ": " .. ext) end

-- Initialize Vulkan
print("vulkan.create_instance")
local instance = vulkan.create_instance({
    application_info = {
        application_name = "Vulkan Triangle",
        application_version = vulkan.make_version(1, 0, 0),
        engine_name = "LuaJIT Vulkan",
        engine_version = vulkan.make_version(1, 0, 0),
        api_version = vulkan.VK_API_VERSION_1_0
    },
    enabled_layer_names = {"VK_LAYER_KHRONOS_validation"},
    enabled_extension_names = extensions
})
if not instance then error("Failed to create Vulkan instance") end

print("SDL_Vulkan_CreateSurface")
local surface = SDL.SDL_Vulkan_CreateSurface(window, instance)
if not surface then error("Failed to create Vulkan surface: " .. SDL.SDL_GetError()) end

local physicalDevices = vulkan.vk_EnumeratePhysicalDevices(instance)
if not physicalDevices then error("No physical devices found") end

local physicalDevice = physicalDevices[1]
local props = vulkan.vk_GetPhysicalDeviceProperties(physicalDevice)
print("Using device: " .. props.deviceName)

local queueFamilies = vulkan.vk_GetPhysicalDeviceQueueFamilyProperties(physicalDevice)
print("Queue families available: " .. #queueFamilies)
for i, family in ipairs(queueFamilies) do
    print(string.format("Queue Family %d: %d queues, flags: 0x%x", i, family.queueCount, family.queueFlags))
    local supportsPresent = vulkan.vk_GetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i - 1, surface)
    print("  Supports presenting: " .. tostring(supportsPresent))
end

print("vulkan.vk_CreateDevice")
local device, graphicsFamily, presentFamily = vulkan.vk_CreateDevice(physicalDevice, surface, {
    enabled_extension_names = { "VK_KHR_swapchain" }
})
if not device then error("Failed to create Vulkan device: " .. graphicsFamily) end
print("Graphics queue family: " .. graphicsFamily)
print("Present queue family: " .. presentFamily)

print("vulkan.vk_GetDeviceQueue (graphics)")
local graphicsQueue = vulkan.vk_GetDeviceQueue(device, graphicsFamily, 0)
if not graphicsQueue then error("Failed to get graphics queue") end

print("vulkan.vk_GetDeviceQueue (present)")
local presentQueue = vulkan.vk_GetDeviceQueue(device, presentFamily, 0)
if not presentQueue then error("Failed to get present queue") end

print("vulkan.vk_GetPhysicalDeviceSurfaceCapabilitiesKHR")
local caps = vulkan.vk_GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface)
if not caps then error("Failed to get surface capabilities") end
print("Min image count: " .. caps.minImageCount .. ", Max image count: " .. caps.maxImageCount)
print("Current extent: " .. caps.currentWidth .. "x" .. caps.currentHeight)

print("vulkan.vk_GetPhysicalDeviceSurfaceFormatsKHR")
local formats = vulkan.vk_GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface)
if not formats then error("Failed to get surface formats") end
for i, fmt in ipairs(formats) do
    print(string.format("Format %d: format=%d, colorSpace=%d", i, fmt.format, fmt.colorSpace))
end

print("vulkan.vk_GetPhysicalDeviceSurfacePresentModesKHR")
local presentModes = vulkan.vk_GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface)
if not presentModes then error("Failed to get present modes") end
for i, mode in ipairs(presentModes) do print("Present mode " .. i .. ": " .. mode) end

print("vulkan.vk_CreateSwapchainKHR")
local swapchain = vulkan.vk_CreateSwapchainKHR(device, {
    surface = surface,
    minImageCount = caps.minImageCount,
    imageFormat = vulkan.VK_FORMAT_B8G8R8A8_UNORM,
    imageColorSpace = vulkan.VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    imageExtentWidth = caps.currentWidth,
    imageExtentHeight = caps.currentHeight,
    queueFamilyIndices = { graphicsFamily },
    presentMode = vulkan.VK_PRESENT_MODE_FIFO_KHR
})
if not swapchain then error("Failed to create swapchain") end
print("Swapchain created successfully")

print("vulkan.vk_GetSwapchainImagesKHR")
local swapchainImages = vulkan.vk_GetSwapchainImagesKHR(device, swapchain)
if not swapchainImages then error("Failed to get swapchain images") end
print("Number of swapchain images: " .. #swapchainImages)

print("vulkan.vk_CreateImageView")
local imageViews = {}
for i, image in ipairs(swapchainImages) do
    local view = vulkan.vk_CreateImageView(device, {
        image = image,
        format = vulkan.VK_FORMAT_B8G8R8A8_UNORM
    })
    if not view then error("Failed to create image view " .. i) end
    imageViews[i] = view
end
print("Created " .. #imageViews .. " image views")

print("vulkan.vk_CreateRenderPass")
local renderPass = vulkan.vk_CreateRenderPass(device, {
    format = vulkan.VK_FORMAT_B8G8R8A8_UNORM,
    initialLayout = vulkan.VK_IMAGE_LAYOUT_UNDEFINED,
    finalLayout = vulkan.VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
})
if not renderPass then error("Failed to create render pass") end
print("Render pass created successfully")

print("vulkan.vk_CreateFramebuffer")
local framebuffers = {}
for i, view in ipairs(imageViews) do
    local fb = vulkan.vk_CreateFramebuffer(device, {
        renderPass = renderPass,
        attachments = { view },
        width = caps.currentWidth,
        height = caps.currentHeight
    })
    if not fb then error("Failed to create framebuffer " .. i) end
    framebuffers[i] = fb
end
print("Created " .. #framebuffers .. " framebuffers")

-- Load shaders from files
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

print("vulkan.vk_CreateShaderModule (vertex)")
local vertShaderModule = assert(vulkan.vk_CreateShaderModule(device, vertShaderCode))
print("vulkan.vk_CreateShaderModule (fragment)")
local fragShaderModule = assert(vulkan.vk_CreateShaderModule(device, fragShaderCode))

print("vulkan.vk_CreatePipelineLayout")
local pipelineLayout = assert(vulkan.vk_CreatePipelineLayout(device))

print("vulkan.vk_CreateGraphicsPipelines")
local pipeline = assert(vulkan.vk_CreateGraphicsPipelines(device, {
    vertexShader = vertShaderModule,
    fragmentShader = fragShaderModule,
    pipelineLayout = pipelineLayout,
    renderPass = renderPass
}))

-- Create synchronization objects (one per swapchain image)
local imageAvailableSemaphores = {}
local renderFinishedSemaphores = {}
local inFlightFences = {}
for i = 1, #swapchainImages do
    imageAvailableSemaphores[i] = assert(vulkan.vk_CreateSemaphore(device))
    renderFinishedSemaphores[i] = assert(vulkan.vk_CreateSemaphore(device))
    inFlightFences[i] = assert(vulkan.vk_CreateFence(device, true)) -- Signaled initially
end

-- Create command pool and buffers (one per swapchain image)
local commandPool = assert(vulkan.vk_CreateCommandPool(device, graphicsFamily))
local commandBuffers = assert(vulkan.vk_AllocateCommandBuffers(device, commandPool, #swapchainImages))

-- Render function
local currentFrame = 1
local function render()
    local fence = inFlightFences[currentFrame]
    
    -- Wait for the previous frame using this fence to complete
    vulkan.vk_WaitForFences(device, fence)
    vulkan.vk_ResetFences(device, fence)

    -- Acquire the next image
    local imageIndex = vulkan.vk_AcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], nil)
    if not imageIndex then
        print("Failed to acquire next image")
        return
    end

    -- Use the command buffer corresponding to the current frame
    local cmdBuffer = commandBuffers[currentFrame] -- Use currentFrame, not imageIndex
    
    -- Reset and begin recording
    vulkan.vk_ResetCommandBuffer(cmdBuffer)
    vulkan.vk_BeginCommandBuffer(cmdBuffer)

    -- Render pass
    vulkan.vk_CmdBeginRenderPass(cmdBuffer, renderPass, framebuffers[imageIndex + 1])
    vulkan.vk_CmdBindPipeline(cmdBuffer, pipeline)
    vulkan.vk_CmdDraw(cmdBuffer, 3, 1, 0, 0)
    vulkan.vk_CmdEndRenderPass(cmdBuffer)

    vulkan.vk_EndCommandBuffer(cmdBuffer)

    -- Submit the command buffer
    local submitInfo = {
        waitSemaphores = { imageAvailableSemaphores[currentFrame] },
        commandBuffers = { cmdBuffer },
        signalSemaphores = { renderFinishedSemaphores[currentFrame] }
    }
    vulkan.vk_QueueSubmit(graphicsQueue, { submitInfo }, fence)

    -- Present the image
    local presentInfo = {
        waitSemaphores = { renderFinishedSemaphores[currentFrame] },
        swapchains = { { swapchain = swapchain, imageIndex = imageIndex } }
    }
    vulkan.vk_QueuePresentKHR(presentQueue, presentInfo)

    -- Move to the next frame
    currentFrame = (currentFrame % #swapchainImages) + 1
end

-- Render loop
local running = true
while running do
    local event = SDL.SDL_PollEvent()
    while event do
        local event_type = SDL.SDL_GetEventType(event)
        if event_type == SDL.SDL_EVENT_QUIT then
            running = false
        end
        event = SDL.SDL_PollEvent()
    end
    render()
end

-- Cleanup function (unchanged from your original)
local function cleanup()
    print("Starting cleanup...")
    vulkan.vk_QueueWaitIdle(graphicsQueue)
    vulkan.vk_QueueWaitIdle(presentQueue)

    for i = 1, #inFlightFences do
        vulkan.vk_DestroyFence(device, inFlightFences[i])
        vulkan.vk_DestroySemaphore(device, renderFinishedSemaphores[i])
        vulkan.vk_DestroySemaphore(device, imageAvailableSemaphores[i])
    end
    vulkan.vk_DestroyCommandPool(device, commandPool)
    vulkan.vk_DestroyPipeline(device, pipeline)
    vulkan.vk_DestroyPipelineLayout(device, pipelineLayout)
    vulkan.vk_DestroyShaderModule(device, fragShaderModule)
    vulkan.vk_DestroyShaderModule(device, vertShaderModule)
    for i = 1, #framebuffers do
        vulkan.vk_DestroyFramebuffer(device, framebuffers[i])
    end
    vulkan.vk_DestroyRenderPass(device, renderPass)
    for i = 1, #imageViews do
        vulkan.vk_DestroyImageView(device, imageViews[i])
    end
    vulkan.vk_DestroySwapchainKHR(device, swapchain)
    vulkan.vk_DestroyDevice(device)
    vulkan.vk_DestroyInstance(instance)
    SDL.SDL_DestroyWindow(window)
    SDL.SDL_Quit()
    print("Done.")
end

cleanup()