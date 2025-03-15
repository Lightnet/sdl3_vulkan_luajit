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
    print("Swapchain images retrieval failed: " .. (err or "No error message"))
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
    print("Creating framebuffer " .. i .. " with image " .. tostring(image))
    local framebuffer, fb_err = vulkan.VK_CreateFramebuffer(device, renderPass, image, extent)
    if not framebuffer then
        print("Framebuffer " .. i .. " creation failed: " .. (fb_err or "No error message"))
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
    print("Framebuffer " .. i .. " created")
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
  print("Recording command buffer " .. i)
  if not cmd then
      print("Command buffer " .. i .. " is nil")
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
  print("Command buffer " .. i .. ": Begin successful")

  if not framebuffers[i] then
      print("Framebuffer " .. i .. " is nil")
      vulkan.VK_EndCommandBuffer(cmd)
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

  print("Command buffer " .. i .. ": Beginning render pass...")
  vulkan.VK_CmdBeginRenderPass(cmd, renderPass, framebuffers[i], extent)
  print("Command buffer " .. i .. ": Render pass begun")

  print("Command buffer " .. i .. ": Binding pipeline...")
  vulkan.VK_CmdBindPipeline(cmd, pipeline)
  print("Command buffer " .. i .. ": Pipeline bound")

  print("Command buffer " .. i .. ": Drawing triangle...")
  vulkan.VK_CmdDraw(cmd, 3, 1, 0, 0)
  print("Command buffer " .. i .. ": Triangle drawn")

  print("Command buffer " .. i .. ": Ending render pass...")
  vulkan.VK_CmdEndRenderPass(cmd)
  print("Command buffer " .. i .. ": Render pass ended")

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
  print("Command buffer " .. i .. " recorded")
end
print("Command buffers recorded")


-- Create two image available semaphores
print("Creating image available semaphores...")
local imageAvailableSemaphores = {}
for i = 1, 2 do
    local semaphore, err = vulkan.VK_CreateSemaphore(device)
    if not semaphore then
        print("Failed to create image available semaphore " .. i .. ": " .. (err or "No error message"))
        -- Cleanup and exit (add your cleanup here if needed)
        return
    end
    table.insert(imageAvailableSemaphores, semaphore)
    print("Image available semaphore " .. i .. " created")
end

print("Creating render finished semaphore...")
local renderFinishedSemaphore, err = vulkan.VK_CreateSemaphore(device)
if not renderFinishedSemaphore then
    print("Failed to create render finished semaphore: " .. (err or "No error message"))
    return
end
print("Render finished semaphore created")



print("Creating fences...")
local fences = {}
for i = 1, 2 do
    local fence, err = vulkan.VK_CreateFence(device)
    if not fence then
        print("Failed to create fence " .. i .. ": " .. (err or "No error message"))
        return
    end
    table.insert(fences, fence)
    print("Fence " .. i .. " created")
end
print("Fences created: " .. #fences)

if arg and #arg > 0 then
    print("Arguments:")
    for i, v in ipairs(arg) do
        print(i, v)
    end
else
    print("No arguments provided")
end

print("Hello from main.lua! Vulkan API: " .. vulkan.API_VERSION_1_4)

print("Entering render loop...")
local done = false
local currentFrame = 1  -- Track which semaphore/fence pair to use (1 or 2)
while not done do
    local event_type = sdl3.SDL_PollEvent()
    while event_type do
        if event_type == sdl3.SDL_EVENT_QUIT then
            done = true
            print("Quit event received")
            break
        end
        event_type = sdl3.SDL_PollEvent()
    end

    if not done then
        -- Wait for the previous frame to finish
        print("Waiting for fence " .. currentFrame .. "...")
        local success, err = vulkan.VK_WaitForFences(device, fences[currentFrame], 1000000000)
        if not success then
            print("Failed to wait for fence " .. currentFrame .. ": " .. (err or "No error message"))
            done = true
            break
        end

        print("Acquiring next image with semaphore " .. currentFrame .. "...")
        local imageIndex, err = vulkan.VK_AcquireNextImageKHR(device, swapchain, imageAvailableSemaphores[currentFrame])
        if not imageIndex then
            print("Failed to acquire next image: " .. (err or "No error message"))
            done = true
            break
        end
        print("Image acquired: " .. imageIndex)

        print("Resetting fence " .. currentFrame .. "...")
        success, err = vulkan.VK_ResetFences(device, fences[currentFrame])
        if not success then
            print("Failed to reset fence " .. currentFrame .. ": " .. (err or "No error message"))
            done = true
            break
        end

        print("Submitting queue for image " .. imageIndex .. "...")
        success, err = vulkan.VK_QueueSubmit(graphicsQueue, commandBuffers[imageIndex], imageAvailableSemaphores[currentFrame], renderFinishedSemaphore, fences[currentFrame])
        if not success then
            print("Queue submit failed: " .. (err or "No error message"))
            done = true
            break
        end

        print("Presenting image " .. imageIndex .. "...")
        success, err = vulkan.VK_QueuePresentKHR(graphicsQueue, swapchain, imageIndex, renderFinishedSemaphore)
        if not success then
            print("Queue present failed: " .. (err or "No error message"))
            done = true
            break
        end

        -- Toggle to the next frame (1 or 2)
        currentFrame = currentFrame % 2 + 1
        sdl3.SDL_Delay(16)  -- ~60 FPS
    end
end
print("Exited render loop")

-- Wait for GPU to finish
print("Waiting for queue to idle...")
local success, err = vulkan.VK_QueueWaitIdle(graphicsQueue)
if not success then
    print("Queue wait idle failed: " .. (err or "No error message"))
end

-- Cleanup
print("Cleaning up...")
for i, fence in ipairs(fences) do
    vulkan.VK_DestroyFence(fence)
end
for i, semaphore in ipairs(imageAvailableSemaphores) do
    vulkan.VK_DestroySemaphore(semaphore)
end
vulkan.VK_DestroySemaphore(renderFinishedSemaphore)
vulkan.VK_DestroyCommandPool(commandPool)
vulkan.VK_DestroyPipeline(pipeline)
vulkan.VK_DestroyShaderModule(fragShader)
vulkan.VK_DestroyShaderModule(vertShader)
for i, fb in ipairs(framebuffers) do
    vulkan.VK_DestroyFramebuffer(fb)
end
vulkan.VK_DestroyRenderPass(renderPass)
vulkan.VK_DestroySwapchainKHR(swapchain)
vulkan.VK_DestroyDevice(device)
vulkan.VK_DestroySurfaceKHR(surface)
vulkan.VK_DestroyInstance(instance)
sdl3.SDL_Quit()
print("Cleanup complete")