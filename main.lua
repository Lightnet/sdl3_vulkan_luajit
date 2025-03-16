local SDL = require("SDL")
local vulkan = require("vulkan")

-- Simple vertex shader (SPIR-V binary)
-- local vertShaderCode = string.char(
--     0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x0a, 0x00, 0x08, 0x00,
--     0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
--     0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00,
--     0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
--     0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
--     0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
--     0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
--     0x0d, 0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
--     0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00,
--     0x90, 0x01, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
--     0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00,
--     0x0d, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x50, 0x6f, 0x73, 0x69, 0x74,
--     0x69, 0x6f, 0x6e, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00,
--     0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00,
--     0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
--     0x02, 0x00, 0x00, 0x00, 0x2a, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
--     0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x2a, 0x00, 0x04, 0x00,
--     0x07, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
--     0x20, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
--     0x07, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00,
--     0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
--     0xf8, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
--     0x0d, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00,
--     0x38, 0x00, 0x01, 0x00
-- )

-- -- Simple fragment shader (SPIR-V binary)
-- local fragShaderCode = string.char(
--     0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x0a, 0x00, 0x08, 0x00,
--     0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
--     0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00,
--     0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
--     0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
--     0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x06, 0x00, 0x04, 0x00, 0x00, 0x00,
--     0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
--     0x0d, 0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x00,
--     0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00,
--     0x90, 0x01, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
--     0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00,
--     0x0d, 0x00, 0x00, 0x00, 0x6f, 0x75, 0x74, 0x43, 0x6f, 0x6c, 0x6f, 0x72,
--     0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00,
--     0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00,
--     0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
--     0x02, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
--     0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x2c, 0x00, 0x05, 0x00,
--     0x08, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
--     0x11, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x14, 0x00, 0x00, 0x00,
--     0x03, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00,
--     0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
--     0x03, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00,
--     0x3e, 0x00, 0x03, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00,
--     0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00
-- )

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
    format = vulkan.VK_FORMAT_B8G8R8A8_UNORM
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

-- print(vertShaderCode)

-- Create shader modules
print("vulkan.vk_CreateShaderModule (vertex)")
local vertShaderModule = assert(vulkan.vk_CreateShaderModule(device, vertShaderCode))
print("vulkan.vk_CreateShaderModule (fragment)")
local fragShaderModule = assert(vulkan.vk_CreateShaderModule(device, fragShaderCode))


-- print("vulkan.vk_CreateShaderModule (vertex)")
-- local vertShaderModule = vulkan.vk_CreateShaderModule(device, vertShaderCode)
-- if not vertShaderModule then error("Failed to create vertex shader module") end

-- print("vulkan.vk_CreateShaderModule (fragment)")
-- local fragShaderModule = vulkan.vk_CreateShaderModule(device, fragShaderCode)
-- if not fragShaderModule then error("Failed to create fragment shader module") end


print("vulkan.vk_CreatePipelineLayout")
local pipelineLayout = assert(vulkan.vk_CreatePipelineLayout(device))

print("vulkan.vk_CreateGraphicsPipelines")
local graphicsPipeline = assert(vulkan.vk_CreateGraphicsPipelines(device, {
    vertexShader = vertShaderModule,
    fragmentShader = fragShaderModule,
    pipelineLayout = pipelineLayout,
    renderPass = renderPass
}))

-- Create synchronization objects
local imageAvailableSemaphore = assert(vulkan.vk_CreateSemaphore(device))
local renderFinishedSemaphore = assert(vulkan.vk_CreateSemaphore(device))
local inFlightFence = assert(vulkan.vk_CreateFence(device, true)) -- Signaled initially

-- Create command pool and buffer
local commandPool = assert(vulkan.vk_CreateCommandPool(device, graphicsFamily))
local commandBuffers = assert(vulkan.vk_AllocateCommandBuffers(device, commandPool, 1))
local cmdBuffer = commandBuffers[1]

-- Render loop
local running = true
while running do
    -- Handle SDL events
    local event = SDL.SDL_PollEvent()
    -- print("event")
    -- print(event)
    while event do
        -- print("event")
        local event_type = SDL.SDL_GetEventType(event)
        if event_type == SDL.SDL_EVENT_QUIT then
            running = false
        end
        event = SDL.SDL_PollEvent()
    end
    -- Wait for the previous frame to finish
    assert(vulkan.vk_WaitForFences(device, inFlightFence))
    assert(vulkan.vk_ResetFences(device, inFlightFence))

    -- Acquire the next swapchain image
    local imageIndex = assert(vulkan.vk_AcquireNextImageKHR(device, swapchain, nil, imageAvailableSemaphore, nil))

    -- Reset and record the command buffer for this frame
    assert(vulkan.vk_ResetCommandBuffer(cmdBuffer))
    assert(vulkan.vk_BeginCommandBuffer(cmdBuffer))

    -- Begin render pass
    vulkan.vk_CmdBeginRenderPass(cmdBuffer, renderPass, framebuffers[imageIndex + 1])

    -- Bind pipeline and draw triangle
    vulkan.vk_CmdBindPipeline(cmdBuffer, graphicsPipeline)
    vulkan.vk_CmdDraw(cmdBuffer, 3, 1, 0, 0)

    -- End render pass
    vulkan.vk_CmdEndRenderPass(cmdBuffer)

    -- Transition swapchain image to PRESENT_SRC_KHR
    local barrier = {
      sType = vulkan.VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      oldLayout = vulkan.VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      newLayout = vulkan.VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      srcQueueFamilyIndex = vulkan.VK_QUEUE_FAMILY_IGNORED,
      dstQueueFamilyIndex = vulkan.VK_QUEUE_FAMILY_IGNORED,
      srcAccessMask = vulkan.VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      dstAccessMask = vulkan.VK_ACCESS_MEMORY_READ_BIT,
      image = swapchainImages[imageIndex + 1],
      subresourceRange = {
          aspectMask = vulkan.VK_IMAGE_ASPECT_COLOR_BIT,
          baseMipLevel = 0,
          levelCount = 1,
          baseArrayLayer = 0,
          layerCount = 1
      }
    }
    vulkan.vk_CmdPipelineBarrier(cmdBuffer, vulkan.VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, vulkan.VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, nil, nil, { barrier })

    -- End command buffer recording
    assert(vulkan.vk_EndCommandBuffer(cmdBuffer))

    -- Submit the command buffer
    local submitInfo = {
        {
            waitSemaphores = { imageAvailableSemaphore },
            waitDstStageMask = { "VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT" },
            commandBuffers = { cmdBuffer },
            signalSemaphores = { renderFinishedSemaphore }
        }
    }
    assert(vulkan.vk_QueueSubmit(graphicsQueue, submitInfo, inFlightFence))

    -- Present the image
    local presentInfo = {
        waitSemaphores = { renderFinishedSemaphore },
        swapchains = { { swapchain = swapchain, imageIndex = imageIndex } }
    }
    assert(vulkan.vk_QueuePresentKHR(presentQueue, presentInfo))
end


local function cleanup()
  print("Starting cleanup...")
  vulkan.vk_DestroyFence(device, inFlightFence)
  vulkan.vk_DestroySemaphore(device, renderFinishedSemaphore)
  vulkan.vk_DestroySemaphore(device, imageAvailableSemaphore)
  vulkan.vk_FreeCommandBuffers(device, commandPool, commandBuffers)
  vulkan.vk_DestroyCommandPool(device, commandPool)
  vulkan.vk_DestroyPipeline(device, graphicsPipeline)
  vulkan.vk_DestroyPipelineLayout(device, pipelineLayout)
  vulkan.vk_DestroyShaderModule(device, fragShaderModule)
  vulkan.vk_DestroyShaderModule(device, vertShaderModule)
  for _, fb in ipairs(framebuffers) do vulkan.vk_DestroyFramebuffer(device, fb) end
  vulkan.vk_DestroyRenderPass(device, renderPass)
  for _, view in ipairs(imageViews) do vulkan.vk_DestroyImageView(device, view) end
  vulkan.vk_DestroySwapchainKHR(device, swapchain)
  vulkan.vk_DestroyDevice(device)
  vulkan.vk_DestroySurfaceKHR(instance, surface)
  vulkan.vk_DestroyInstance(instance)
  SDL.SDL_DestroyWindow(window)
  SDL.SDL_Quit()
end

cleanup()
print("Done.")