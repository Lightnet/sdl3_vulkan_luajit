local SDL = require("SDL")
local vulkan = require("vulkan")

-- Initialize SDL
if not SDL.SDL_Init(SDL.SDL_INIT_VIDEO) then
    error("Failed to initialize SDL: " .. SDL.SDL_GetError())
end

local window = SDL.SDL_CreateWindow("Vulkan Triangle", 800, 600, SDL.SDL_WINDOW_VULKAN)
if not window then
    error("Failed to create window: " .. SDL.SDL_GetError())
end

-- Vulkan setup
local instance = vulkan.VK_CreateInstanceHelper(window, "Vulkan App", "LuaJIT Engine")
local surface = vulkan.SDL_Vulkan_CreateSurface(window, instance)
local physicalDevices = vulkan.VK_EnumeratePhysicalDevices(instance)
local physicalDevice = physicalDevices[1]
local queueFamilyProperties = vulkan.VK_GetPhysicalDeviceQueueFamilyProperties(physicalDevice)
local queueFamilyIndex
for i, family in ipairs(queueFamilyProperties) do
    if bit.band(family.queueFlags, vulkan.VK_QUEUE_GRAPHICS_BIT) ~= 0 then
        queueFamilyIndex = i - 1
        break
    end
end

local device = vulkan.VK_CreateDevice(physicalDevice, queueFamilyIndex)
local queue = vulkan.VK_GetDeviceQueue(device, queueFamilyIndex, 0)
local swapchain, extent = vulkan.VK_CreateSwapchainKHR(device, surface, window, physicalDevice)
local swapchainImages = vulkan.VK_GetSwapchainImagesKHR(device, swapchain)
local renderPass = vulkan.VK_CreateRenderPass(device)
local framebuffers = {}
for i, image in ipairs(swapchainImages) do
    framebuffers[i] = vulkan.VK_CreateFramebuffer(device, renderPass, image, extent)
end

local vertShader = assert(vulkan.VK_CreateShaderModule(device, "triangle.vert.spv"))
local fragShader = assert(vulkan.VK_CreateShaderModule(device, "triangle.frag.spv"))
local pipeline, descriptorSetLayout = vulkan.VK_CreateGraphicsPipelines(device, vertShader, fragShader, renderPass, extent)
local commandPool = vulkan.VK_CreateCommandPool(device, queueFamilyIndex)
local commandBuffers = vulkan.VK_AllocateCommandBuffers(device, commandPool, #swapchainImages)

-- Descriptor pool and set
local descriptorPool = vulkan.VK_CreateDescriptorPool(device, {
    maxSets = 1,
    poolSizes = {{type = vulkan.VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorCount = 1}}
})
local uniformBuffer = assert(vulkan.VK_CreateBuffer(device, 8))
local position = {x = 0.0, y = 0.0}
vulkan.VK_UpdateBuffer(uniformBuffer, position)
local descriptorSet = vulkan.VK_AllocateDescriptorSet(device, descriptorPool, descriptorSetLayout)
vulkan.VK_UpdateDescriptorSet(device, descriptorSet, uniformBuffer, 0, 8)

local imageAvailableSemaphore = vulkan.VK_CreateSemaphore(device)
local renderFinishedSemaphore = vulkan.VK_CreateSemaphore(device)
local fence = vulkan.VK_CreateFence(device)

-- Main loop
local running = true
local startTime = SDL.SDL_GetTicks()
local frameCount = 0

print("SDL.SDL_EVENT_QUIT: " .. SDL.SDL_EVENT_QUIT)

while running do
    local event = SDL.SDL_PollEvent()
    while event do
        local event_type = SDL.SDL_GetEventType(event)
        if event_type == SDL.SDL_EVENT_QUIT then
            running = false
            break
        elseif event_type == SDL.SDL_EVENT_KEY_DOWN then
            local key = SDL.SDL_GetKeyFromEvent(event)
            if key == SDL.SDLK_LEFT then
                print("LEFT")
                position.x = position.x - 0.05
                vulkan.VK_UpdateBuffer(uniformBuffer, position)
            elseif key == SDL.SDLK_RIGHT then
                print("RIGHT")
                position.x = position.x + 0.05
                vulkan.VK_UpdateBuffer(uniformBuffer, position)
            elseif key == SDL.SDLK_UP then
                print("UP")
                position.y = position.y - 0.05
                vulkan.VK_UpdateBuffer(uniformBuffer, position)
            elseif key == SDL.SDLK_DOWN then
                print("DOWN")
                position.y = position.y + 0.05
                vulkan.VK_UpdateBuffer(uniformBuffer, position)
            end
        end
        event = SDL.SDL_PollEvent()
    end

    local imageIndex = vulkan.VK_AcquireNextImageKHR(device, swapchain, imageAvailableSemaphore)
    local cmd = commandBuffers[imageIndex]

    assert(vulkan.VK_BeginCommandBuffer(cmd))
    vulkan.VK_CmdBeginRenderPass(cmd, renderPass, framebuffers[imageIndex], extent)
    vulkan.VK_CmdBindPipeline(cmd, pipeline)
    vulkan.VK_CmdBindDescriptorSet(cmd, pipeline, descriptorSet, 0)
    vulkan.VK_CmdDraw(cmd, 3, 1, 0, 0)
    vulkan.VK_CmdEndRenderPass(cmd)
    assert(vulkan.VK_EndCommandBuffer(cmd))

    vulkan.VK_ResetFences(device, fence)
    assert(vulkan.VK_QueueSubmit(queue, cmd, imageAvailableSemaphore, renderFinishedSemaphore, fence))
    assert(vulkan.VK_WaitForFences(device, fence, 1000000000))
    assert(vulkan.VK_QueuePresentKHR(queue, swapchain, imageIndex, renderFinishedSemaphore))

    frameCount = frameCount + 1
    local currentTime = SDL.SDL_GetTicks()
    if currentTime - startTime >= 1000 then
        print("FPS: " .. frameCount)
        frameCount = 0
        startTime = currentTime
    end
end

local function cleanup()
    vulkan.VK_QueueWaitIdle(queue)
    vulkan.VK_DestroyFence(device, fence)
    vulkan.VK_DestroySemaphore(device, renderFinishedSemaphore)
    vulkan.VK_DestroySemaphore(device, imageAvailableSemaphore)
    vulkan.VK_DestroyBuffer(device, uniformBuffer)
    vulkan.VK_DestroyDescriptorPool(device, descriptorPool)
    vulkan.VK_DestroyCommandPool(device, commandPool)
    for _, fb in ipairs(framebuffers) do
        vulkan.VK_DestroyFramebuffer(device, fb)
    end
    vulkan.VK_DestroyRenderPass(device, renderPass)
    vulkan.VK_DestroySwapchainKHR(device, swapchain)
    vulkan.VK_DestroyPipeline(device, pipeline)
    vulkan.VK_DestroyShaderModule(device, fragShader)
    vulkan.VK_DestroyShaderModule(device, vertShader)
    vulkan.VK_DestroyDescriptorSetLayout(device, descriptorSetLayout)
    vulkan.VK_DestroyDevice(device)
    vulkan.VK_DestroySurfaceKHR(instance, surface)
    vulkan.VK_DestroyInstance(instance)
    SDL.SDL_DestroyWindow(window)
    SDL.SDL_Quit()
end

pcall(cleanup)
print("Done.")