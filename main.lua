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
local swapchain, width, height = vulkan.VK_CreateSwapchainKHR(device, surface, window, physicalDevice)
local extent = {width = width, height = height}
local swapchainImages = vulkan.VK_GetSwapchainImagesKHR(device, swapchain)
print("Swapchain image type: " .. type(swapchainImages[1]))
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
print("Number of swapchain images: " .. #swapchainImages)
print("Number of command buffers: " .. #commandBuffers)
for i, cb in ipairs(commandBuffers) do
    print("Command buffer " .. i .. ": " .. tostring(cb))
end

print("Descriptor pool flags: " .. tostring(vulkan.VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT))
local descriptorPool = vulkan.VK_CreateDescriptorPool(device, {
    flags = vulkan.VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
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
local firstFrame = true

print("SDL.SDL_EVENT_QUIT: " .. SDL.SDL_EVENT_QUIT)
print("SDL module:", SDL)
for k, v in pairs(SDL) do
    print("SDL." .. k, v)
end
print("END SDL module:")

local success, err = pcall(function()
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
                    position.x = position.x - 0.05
                    vulkan.VK_UpdateBuffer(uniformBuffer, position)
                elseif key == SDL.SDLK_RIGHT then
                    position.x = position.x + 0.05
                    vulkan.VK_UpdateBuffer(uniformBuffer, position)
                elseif key == SDL.SDLK_UP then
                    position.y = position.y - 0.05
                    vulkan.VK_UpdateBuffer(uniformBuffer, position)
                elseif key == SDL.SDLK_DOWN then
                    position.y = position.y + 0.05
                    vulkan.VK_UpdateBuffer(uniformBuffer, position)
                end
            end
            event = SDL.SDL_PollEvent()
        end

        vulkan.VK_WaitForFences(device, fence, 1000000000)
        vulkan.VK_ResetFences(device, fence)
        local imageIndex, recreateSwapchain = vulkan.VK_AcquireNextImageKHR(device, swapchain, imageAvailableSemaphore, 1000000000)
        if recreateSwapchain then
            print("Swapchain recreation needed")
            swapchain, extent = vulkan.VK_CreateSwapchainKHR(device, surface, window, physicalDevice)
            swapchainImages = vulkan.VK_GetSwapchainImagesKHR(device, swapchain)
            for i, fb in ipairs(framebuffers) do
                vulkan.VK_DestroyFramebuffer(device, fb)
            end
            framebuffers = {}
            for i, image in ipairs(swapchainImages) do
                framebuffers[i] = vulkan.VK_CreateFramebuffer(device, renderPass, image, extent)
            end
            commandBuffers = vulkan.VK_AllocateCommandBuffers(device, commandPool, #swapchainImages)
            imageIndex = 0
        end

        if imageIndex and imageIndex >= 0 and imageIndex < #swapchainImages then
            local cmd = commandBuffers[imageIndex + 1]
            if cmd then
                assert(vulkan.VK_BeginCommandBuffer(cmd))
                vulkan.VK_CmdPipelineBarrier(cmd, 
                    vulkan.VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    vulkan.VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    0, {}, {}, {
                        {
                            srcAccessMask = 0,
                            dstAccessMask = vulkan.VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                            oldLayout = firstFrame and vulkan.VK_IMAGE_LAYOUT_UNDEFINED or vulkan.VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                            newLayout = vulkan.VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            image = swapchainImages[imageIndex + 1]
                        }
                    })
                vulkan.VK_CmdBeginRenderPass(cmd, renderPass, framebuffers[imageIndex + 1], extent)
                vulkan.VK_CmdBindPipeline(cmd, pipeline)
                vulkan.VK_CmdBindDescriptorSet(cmd, pipeline, descriptorSet, 0)
                vulkan.VK_CmdDraw(cmd, 3, 1, 0, 0)
                vulkan.VK_CmdEndRenderPass(cmd)
                vulkan.VK_CmdPipelineBarrier(cmd, 
                    vulkan.VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    vulkan.VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    0, {}, {}, {
                        {
                            srcAccessMask = vulkan.VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                            dstAccessMask = 0,
                            oldLayout = vulkan.VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            newLayout = vulkan.VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                            image = swapchainImages[imageIndex + 1]
                        }
                    })
                assert(vulkan.VK_EndCommandBuffer(cmd))
                assert(vulkan.VK_QueueSubmit(queue, cmd, imageAvailableSemaphore, renderFinishedSemaphore, fence))
                assert(vulkan.VK_QueuePresentKHR(queue, swapchain, imageIndex, renderFinishedSemaphore))
                firstFrame = false
            else
                print("Error: Command buffer is nil for imageIndex " .. imageIndex)
            end
        else
            print("Error: Invalid or nil imageIndex: " .. tostring(imageIndex))
        end

        frameCount = frameCount + 1
        local currentTime = SDL.SDL_GetTicks()
        if currentTime - startTime >= 1000 then
            print("FPS: " .. frameCount)
            frameCount = 0
            startTime = currentTime
        end
    end
end)
if not success then
    print("Main loop error: " .. tostring(err))
end

local function cleanup()
    print("Starting cleanup...")
    local function safe_call(func, desc, ...)
        local args = {...}
        local success, err = pcall(func, unpack(args))
        if not success then
            print("Cleanup error in " .. desc .. ": " .. tostring(err))
        end
    end

    safe_call(vulkan.VK_QueueWaitIdle, "queue wait idle", queue)
    safe_call(vulkan.VK_DestroyFence, "destroy fence", device, fence)
    safe_call(vulkan.VK_DestroySemaphore, "destroy render finished semaphore", device, renderFinishedSemaphore)
    safe_call(vulkan.VK_DestroySemaphore, "destroy image available semaphore", device, imageAvailableSemaphore)
    safe_call(vulkan.VK_DestroyBuffer, "destroy uniform buffer", device, uniformBuffer)
    safe_call(vulkan.VK_FreeCommandBuffers, "free command buffers", device, commandPool, commandBuffers)
    safe_call(vulkan.VK_DestroyCommandPool, "destroy command pool", device, commandPool)
    safe_call(vulkan.VK_FreeDescriptorSets, "free descriptor set", device, descriptorPool, descriptorSet)
    safe_call(vulkan.VK_DestroyDescriptorPool, "destroy descriptor pool", device, descriptorPool)
    for i, fb in ipairs(framebuffers) do
        safe_call(vulkan.VK_DestroyFramebuffer, "destroy framebuffer " .. i, device, fb)
    end
    safe_call(vulkan.VK_DestroyRenderPass, "destroy render pass", device, renderPass)
    safe_call(vulkan.VK_DestroySwapchainKHR, "destroy swapchain", device, swapchain)
    safe_call(vulkan.VK_DestroyPipeline, "destroy pipeline", device, pipeline)
    safe_call(vulkan.VK_DestroyShaderModule, "destroy frag shader", device, fragShader)
    safe_call(vulkan.VK_DestroyShaderModule, "destroy vert shader", device, vertShader)
    safe_call(vulkan.VK_DestroyDescriptorSetLayout, "destroy descriptor set layout", device, descriptorSetLayout)
    safe_call(vulkan.VK_DestroyDevice, "destroy device", device)
    safe_call(vulkan.VK_DestroySurfaceKHR, "destroy surface", surface, instance)
    safe_call(vulkan.VK_DestroyInstance, "destroy instance", instance)
    safe_call(SDL.SDL_DestroyWindow, "destroy window", window)
    safe_call(SDL.SDL_Quit, "quit SDL")
end

cleanup()
print("Done.")