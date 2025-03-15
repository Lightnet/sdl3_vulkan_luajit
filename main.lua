local bit = require("bit")
local sdl3 = require("sdl3")
local vulkan = require("vulkan")

local ok, err = sdl3.SDL_Init(sdl3.INIT_VIDEO)
if not ok then
    print("SDL3 init failed: " .. (err or "No error message"))
    return
end

local window, err = sdl3.SDL_CreateWindow("LuaJIT + SDL3 + Vulkan", 800, 600, sdl3.WINDOW_VULKAN)
if not window then
    print("Window creation failed: " .. (err or "No error message"))
    sdl3.SDL_Quit()
    return
end

local extensions, err = vulkan.SDL_Vulkan_GetInstanceExtensions()
if not extensions then
    print("Failed to get Vulkan extensions: " .. (err or "No error message"))
    sdl3.SDL_Quit()
    return
end
print("Vulkan instance extensions:")
for i, ext in ipairs(extensions) do
    print("  " .. i .. ": " .. ext)
end

local instance, err = vulkan.VK_CreateInstanceHelper(window, "LuaJIT App", "LuaJIT Engine")
if not instance then
    print("Vulkan init failed: " .. (err or "No error message"))
    sdl3.SDL_Quit()
    return
end

local surface, err = vulkan.SDL_Vulkan_CreateSurface(window, instance)
if not surface then
    print("Surface creation failed: " .. (err or "No error message"))
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end

local devices, err = vulkan.VK_EnumeratePhysicalDevices(instance)
if not devices then
    print("Failed to enumerate devices: " .. (err or "No error message"))
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Found " .. #devices .. " physical devices")

local queueFamilies, err = vulkan.VK_GetPhysicalDeviceQueueFamilyProperties(devices[1])
if not queueFamilies then
    print("Failed to get queue families: " .. (err or "No error message"))
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Queue families for device 1:")
for i, family in ipairs(queueFamilies) do
    print("  " .. i .. ": flags=" .. family.queueFlags .. ", count=" .. family.queueCount)
end

local graphicsQueueFamily = nil
for i, family in ipairs(queueFamilies) do
    if bit.band(family.queueFlags, vulkan.QUEUE_GRAPHICS_BIT) ~= 0 then
        graphicsQueueFamily = { queueFamilyIndex = i - 1, queueCount = 1 }
        break
    end
end
if not graphicsQueueFamily then
    print("No graphics queue family found")
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Selected graphics queue family: index=" .. graphicsQueueFamily.queueFamilyIndex)

local device, err = vulkan.VK_CreateDevice(devices[1], graphicsQueueFamily)
if not device then
    print("Device creation failed: " .. (err or "No error message"))
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Logical device created")

local swapchain, extent, err = vulkan.VK_CreateSwapchainKHR(device, surface, window, devices[1])
if not swapchain then
    print("Swapchain creation failed: " .. (err or "No error message"))
    vulkan.VK_DestroyDevice(device)
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Swapchain created with extent: " .. extent.width .. "x" .. extent.height)

local graphicsQueue, err = vulkan.VK_GetDeviceQueue(device, graphicsQueueFamily.queueFamilyIndex, 0)
if not graphicsQueue then
    print("Failed to get graphics queue: " .. (err or "No error message"))
    vulkan.VK_DestroySwapchainKHR(swapchain)
    vulkan.VK_DestroyDevice(device)
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Graphics queue retrieved")

local swapchainImages, err = vulkan.VK_GetSwapchainImagesKHR(device, swapchain)
if not swapchainImages then
    print("Failed to get swapchain images: " .. (err or "No error message"))
    vulkan.VK_DestroySwapchainKHR(swapchain)
    vulkan.VK_DestroyDevice(device)
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Swapchain images retrieved: " .. #swapchainImages)

local renderPass, err = vulkan.VK_CreateRenderPass(device)
if not renderPass then
    print("Render pass creation failed: " .. (err or "No error message"))
    vulkan.VK_DestroySwapchainKHR(swapchain)
    vulkan.VK_DestroyDevice(device)
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Render pass created")

local framebuffers = {}
for i, image in ipairs(swapchainImages) do
    local framebuffer, err = vulkan.VK_CreateFramebuffer(device, renderPass, image, extent)
    if not framebuffer then
        print("Framebuffer " .. i .. " creation failed: " .. (err or "No error message"))
        for _, fb in ipairs(framebuffers) do
            vulkan.VK_DestroyFramebuffer(fb)
        end
        vulkan.VK_DestroyRenderPass(renderPass)
        vulkan.VK_DestroySwapchainKHR(swapchain)
        vulkan.VK_DestroyDevice(device)
        vulkan.VK_DestroySurfaceKHR(surface)
        vulkan.VK_DestroyInstance(instance)
        sdl3.SDL_Quit()
        return
    end
    table.insert(framebuffers, framebuffer)
end
print("Framebuffers created: " .. #framebuffers)

local vertShader, err = vulkan.VK_CreateShaderModule(device, "triangle.vert.spv")
if not vertShader then
    print("Vertex shader creation failed: " .. (err or "No error message"))
    for _, fb in ipairs(framebuffers) do
        vulkan.VK_DestroyFramebuffer(fb)
    end
    vulkan.VK_DestroyRenderPass(renderPass)
    vulkan.VK_DestroySwapchainKHR(swapchain)
    vulkan.VK_DestroyDevice(device)
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Vertex shader created")

local fragShader, err = vulkan.VK_CreateShaderModule(device, "triangle.frag.spv")
if not fragShader then
    print("Fragment shader creation failed: " .. (err or "No error message"))
    vulkan.VK_DestroyShaderModule(vertShader)
    for _, fb in ipairs(framebuffers) do
        vulkan.VK_DestroyFramebuffer(fb)
    end
    vulkan.VK_DestroyRenderPass(renderPass)
    vulkan.VK_DestroySwapchainKHR(swapchain)
    vulkan.VK_DestroyDevice(device)
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Fragment shader created")

local pipeline, err = vulkan.VK_CreateGraphicsPipelines(device, vertShader, fragShader, renderPass, extent)
if not pipeline then
    print("Pipeline creation failed: " .. (err or "No error message"))
    vulkan.VK_DestroyShaderModule(fragShader)
    vulkan.VK_DestroyShaderModule(vertShader)
    for _, fb in ipairs(framebuffers) do
        vulkan.VK_DestroyFramebuffer(fb)
    end
    vulkan.VK_DestroyRenderPass(renderPass)
    vulkan.VK_DestroySwapchainKHR(swapchain)
    vulkan.VK_DestroyDevice(device)
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Graphics pipeline created")

local commandPool, err = vulkan.VK_CreateCommandPool(device, graphicsQueueFamily.queueFamilyIndex)
if not commandPool then
    print("Command pool creation failed: " .. (err or "No error message"))
    vulkan.VK_DestroyPipeline(pipeline)
    vulkan.VK_DestroyShaderModule(fragShader)
    vulkan.VK_DestroyShaderModule(vertShader)
    for _, fb in ipairs(framebuffers) do
        vulkan.VK_DestroyFramebuffer(fb)
    end
    vulkan.VK_DestroyRenderPass(renderPass)
    vulkan.VK_DestroySwapchainKHR(swapchain)
    vulkan.VK_DestroyDevice(device)
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Command pool created")

local commandBuffers, err = vulkan.VK_AllocateCommandBuffers(device, commandPool, #framebuffers)
if not commandBuffers then
    print("Command buffer allocation failed: " .. (err or "No error message"))
    vulkan.VK_DestroyCommandPool(commandPool)
    vulkan.VK_DestroyPipeline(pipeline)
    vulkan.VK_DestroyShaderModule(fragShader)
    vulkan.VK_DestroyShaderModule(vertShader)
    for _, fb in ipairs(framebuffers) do
        vulkan.VK_DestroyFramebuffer(fb)
    end
    vulkan.VK_DestroyRenderPass(renderPass)
    vulkan.VK_DestroySwapchainKHR(swapchain)
    vulkan.VK_DestroyDevice(device)
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Command buffers allocated: " .. #commandBuffers)

for i, cmd in ipairs(commandBuffers) do
    local success, err = vulkan.VK_BeginCommandBuffer(cmd)
    if not success then
        print("Failed to begin command buffer " .. i .. ": " .. (err or "No error message"))
        vulkan.VK_DestroyCommandPool(commandPool)
        vulkan.VK_DestroyPipeline(pipeline)
        vulkan.VK_DestroyShaderModule(fragShader)
        vulkan.VK_DestroyShaderModule(vertShader)
        for _, fb in ipairs(framebuffers) do
            vulkan.VK_DestroyFramebuffer(fb)
        end
        vulkan.VK_DestroyRenderPass(renderPass)
        vulkan.VK_DestroySwapchainKHR(swapchain)
        vulkan.VK_DestroyDevice(device)
        vulkan.VK_DestroySurfaceKHR(surface)
        vulkan.VK_DestroyInstance(instance)
        sdl3.SDL_Quit()
        return
    end

    vulkan.VK_CmdBeginRenderPass(cmd, renderPass, framebuffers[i], extent)
    vulkan.VK_CmdBindPipeline(cmd, pipeline)
    vulkan.VK_CmdDraw(cmd, 3, 1, 0, 0)  -- 3 vertices, 1 instance
    vulkan.VK_CmdEndRenderPass(cmd)

    success, err = vulkan.VK_EndCommandBuffer(cmd)
    if not success then
        print("Failed to end command buffer " .. i .. ": " .. (err or "No error message"))
        vulkan.VK_DestroyCommandPool(commandPool)
        vulkan.VK_DestroyPipeline(pipeline)
        vulkan.VK_DestroyShaderModule(fragShader)
        vulkan.VK_DestroyShaderModule(vertShader)
        for _, fb in ipairs(framebuffers) do
            vulkan.VK_DestroyFramebuffer(fb)
        end
        vulkan.VK_DestroyRenderPass(renderPass)
        vulkan.VK_DestroySwapchainKHR(swapchain)
        vulkan.VK_DestroyDevice(device)
        vulkan.VK_DestroySurfaceKHR(surface)
        vulkan.VK_DestroyInstance(instance)
        sdl3.SDL_Quit()
        return
    end
end
print("Command buffers recorded")

local imageAvailableSemaphore, err = vulkan.VK_CreateSemaphore(device)
if not imageAvailableSemaphore then
    print("Image available semaphore creation failed: " .. (err or "No error message"))
    vulkan.VK_DestroyCommandPool(commandPool)
    vulkan.VK_DestroyPipeline(pipeline)
    vulkan.VK_DestroyShaderModule(fragShader)
    vulkan.VK_DestroyShaderModule(vertShader)
    for _, fb in ipairs(framebuffers) do
        vulkan.VK_DestroyFramebuffer(fb)
    end
    vulkan.VK_DestroyRenderPass(renderPass)
    vulkan.VK_DestroySwapchainKHR(swapchain)
    vulkan.VK_DestroyDevice(device)
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Image available semaphore created")

local renderFinishedSemaphore, err = vulkan.VK_CreateSemaphore(device)
if not renderFinishedSemaphore then
    print("Render finished semaphore creation failed: " .. (err or "No error message"))
    vulkan.VK_DestroySemaphore(imageAvailableSemaphore)
    vulkan.VK_DestroyCommandPool(commandPool)
    vulkan.VK_DestroyPipeline(pipeline)
    vulkan.VK_DestroyShaderModule(fragShader)
    vulkan.VK_DestroyShaderModule(vertShader)
    for _, fb in ipairs(framebuffers) do
        vulkan.VK_DestroyFramebuffer(fb)
    end
    vulkan.VK_DestroyRenderPass(renderPass)
    vulkan.VK_DestroySwapchainKHR(swapchain)
    vulkan.VK_DestroyDevice(device)
    vulkan.VK_DestroySurfaceKHR(surface)
    vulkan.VK_DestroyInstance(instance)
    sdl3.SDL_Quit()
    return
end
print("Render finished semaphore created")

if arg and #arg > 0 then
    print("Arguments:")
    for i, v in ipairs(arg) do
        print(i, v)
    end
else
    print("No arguments provided")
end

print("Hello from main.lua! Vulkan API: " .. vulkan.API_VERSION_1_4)

local done = false
while not done do
    local event_type = sdl3.SDL_PollEvent()
    while event_type do
        if event_type == sdl3.EVENT_QUIT then
            done = true
        end
        event_type = sdl3.SDL_PollEvent()
    end

    local imageIndex, err = vulkan.VK_AcquireNextImageKHR(device, swapchain, imageAvailableSemaphore)
    if not imageIndex then
        print("Failed to acquire next image: " .. (err or "No error message"))
        break
    end

    local success, err = vulkan.VK_QueueSubmit(graphicsQueue, commandBuffers[imageIndex], imageAvailableSemaphore, renderFinishedSemaphore)
    if not success then
        print("Queue submit failed: " .. (err or "No error message"))
        break
    end

    success, err = vulkan.VK_QueuePresentKHR(graphicsQueue, swapchain, imageIndex, renderFinishedSemaphore)
    if not success then
        print("Queue present failed: " .. (err or "No error message"))
        break
    end

    vulkan.VK_QueueWaitIdle(graphicsQueue)
    sdl3.SDL_Delay(16)
end

vulkan.VK_DestroySemaphore(renderFinishedSemaphore)
vulkan.VK_DestroySemaphore(imageAvailableSemaphore)
vulkan.VK_DestroyCommandPool(commandPool)
vulkan.VK_DestroyPipeline(pipeline)
vulkan.VK_DestroyShaderModule(fragShader)
vulkan.VK_DestroyShaderModule(vertShader)
for _, framebuffer in ipairs(framebuffers) do
    vulkan.VK_DestroyFramebuffer(framebuffer)
end
vulkan.VK_DestroyRenderPass(renderPass)
vulkan.VK_DestroySwapchainKHR(swapchain)
vulkan.VK_DestroyDevice(device)
vulkan.VK_DestroySurfaceKHR(surface)
vulkan.VK_DestroyInstance(instance)
sdl3.SDL_Quit()