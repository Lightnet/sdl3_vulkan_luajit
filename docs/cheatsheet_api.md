
# Vulkan + SDL3 LuaJIT Cheatsheet

# Notes:
 * this grok beta 3 generate.
 * there might be some mistakes due to large text leaking to forget.

Overview

This cheatsheet covers the Vulkan and SDL APIs exposed in vulkan_luajit.c for rendering a basic triangle using LuaJIT and SDL3. It assumes a double-buffered swapchain and a simple graphics pipeline.

---

Step-by-Step Setup and Rendering

1. Initialize SDL

- Function: SDL.SDL_Init(flags)
    
    - Args: flags (int) - Bitmask of subsystems (e.g., SDL.SDL_INIT_VIDEO = 0x20)
        
    - Returns: nil on success, errors via Lua error
        
    - Purpose: Initializes SDL subsystems.
        
    - Example: SDL.SDL_Init(SDL.SDL_INIT_VIDEO)
        
- Function: SDL.SDL_CreateWindow(title, w, h, flags)
    
    - Args: title (string), w, h (int), flags (int, e.g., SDL.SDL_WINDOW_VULKAN = 0x10000000)
        
    - Returns: window (lightuserdata)
        
    - Purpose: Creates a window for Vulkan rendering.
        
    - Example: window = SDL.SDL_CreateWindow("Vulkan Window", 0, 0, 800, 600, SDL.SDL_WINDOW_VULKAN)
        
- Constants:
    
    - SDL.SDL_INIT_VIDEO = 0x20
        
    - SDL.SDL_WINDOW_VULKAN = 0x10000000
        

---

2. Create Vulkan Instance

- Function: vulkan.create_instance(extensions, layers)
    
    - Args: extensions (table of strings), layers (table of strings)
        
    - Returns: instance (VulkanInstance userdata)
        
    - Purpose: Creates a Vulkan instance with required extensions and optional layers.
        
    - Example:
        
        lua
        
        ```lua
        local ext = { "VK_KHR_surface", "VK_KHR_win32_surface" }
        local layers = { "VK_LAYER_KHRONOS_validation" }
        instance = vulkan.create_instance(ext, layers)
        ```
        
- Function: SDL.SDL_Vulkan_GetInstanceExtensions()
    
    - Args: None
        
    - Returns: ext_count (int), ext_names (table of strings)
        
    - Purpose: Retrieves Vulkan extensions required by SDL.
        
    - Example: ext_count, ext_names = SDL.SDL_Vulkan_GetInstanceExtensions()
        

---

3. Create Surface

- Function: SDL.SDL_Vulkan_CreateSurface(window, instance)
    
    - Args: window (lightuserdata), instance (VulkanInstance userdata)
        
    - Returns: surface (VulkanSurface userdata)
        
    - Purpose: Creates a Vulkan surface for the SDL window.
        
    - Example: surface = SDL.SDL_Vulkan_CreateSurface(window, instance)
        

---

4. Create Physical Device and Logical Device

- Function: vulkan.vk_CreateDevice(physicalDevice, queueFamilies)
    
    - Args: physicalDevice (VkPhysicalDevice lightuserdata), queueFamilies (table of {family=int, count=int})
        
    - Returns: device (VulkanDevice userdata)
        
    - Purpose: Creates a logical device with specified queue families.
        
    - Example:
        
        lua
        
        ```lua
        device = vulkan.vk_CreateDevice(physicalDevice, {{family=0, count=1}})
        ```
        
- Function: vulkan.vk_GetDeviceQueue(device, familyIndex, queueIndex)
    
    - Args: device (VulkanDevice userdata), familyIndex (int), queueIndex (int)
        
    - Returns: queue (VulkanQueue userdata)
        
    - Purpose: Retrieves a queue from the device.
        
    - Example: graphicsQueue = vulkan.vk_GetDeviceQueue(device, 0, 0)
        

---

5. Create Swapchain

- Function: vulkan.vk_CreateSwapchainKHR(device, surface, width, height, imageCount)
    
    - Args: device (VulkanDevice), surface (VulkanSurface), width, height (int), imageCount (int)
        
    - Returns: swapchain (VulkanSwapchain userdata)
        
    - Example: swapchain = vulkan.vk_CreateSwapchainKHR(device, surface, 800, 600, 2)
        
- Function: vulkan.vk_GetSwapchainImagesKHR(device, swapchain)
    
    - Args: device (VulkanDevice), swapchain (VulkanSwapchain)
        
    - Returns: images (table of VulkanImage userdata)
        
    - Example: swapchainImages = vulkan.vk_GetSwapchainImagesKHR(device, swapchain)
        

---

6. Create Image Views

- Function: vulkan.vk_CreateImageView(device, image, format)
    
    - Args: device (VulkanDevice), image (VulkanImage), format (int, e.g., VK_FORMAT_B8G8R8A8_UNORM = 44)
        
    - Returns: imageView (VulkanImageView userdata)
        
    - Example:
        
        lua
        
        ```lua
        imageViews = {}
        for i, img in ipairs(swapchainImages) do
            imageViews[i] = vulkan.vk_CreateImageView(device, img, 44)
        end
        ```
        
- Constants:
    
    - VK_FORMAT_B8G8R8A8_UNORM = 44
        

---

7. Create Render Pass

- Function: vulkan.vk_CreateRenderPass(device, format)
    
    - Args: device (VulkanDevice), format (int)
        
    - Returns: renderPass (VulkanRenderPass userdata)
        
    - Example: renderPass = vulkan.vk_CreateRenderPass(device, 44)
        

---

8. Create Framebuffers

- Function: vulkan.vk_CreateFramebuffer(device, renderPass, imageView, width, height)
    
    - Args: device (VulkanDevice), renderPass (VulkanRenderPass), imageView (VulkanImageView), width, height (int)
        
    - Returns: framebuffer (VulkanFramebuffer userdata)
        
    - Example:
        
        lua
        
        ```lua
        framebuffers = {}
        for i, view in ipairs(imageViews) do
            framebuffers[i] = vulkan.vk_CreateFramebuffer(device, renderPass, view, 800, 600)
        end
        ```
        

---

9. Create Shaders and Pipeline

- Function: vulkan.vk_CreateShaderModule(device, code)
    
    - Args: device (VulkanDevice), code (string, SPIR-V binary)
        
    - Returns: shaderModule (VulkanShaderModule userdata)
        
    - Example:
        
        lua
        
        ```lua
        vertShader = vulkan.vk_CreateShaderModule(device, io.open("triangle.vert.spv", "rb"):read("*a"))
        fragShader = vulkan.vk_CreateShaderModule(device, io.open("triangle.frag.spv", "rb"):read("*a"))
        ```
        
- Function: vulkan.vk_CreatePipelineLayout(device)
    
    - Args: device (VulkanDevice)
        
    - Returns: pipelineLayout (VulkanPipelineLayout userdata)
        
    - Example: pipelineLayout = vulkan.vk_CreatePipelineLayout(device)
        
- Function: vulkan.vk_CreateGraphicsPipelines(device, renderPass, pipelineLayout, vertShader, fragShader)
    
    - Args: device (VulkanDevice), renderPass (VulkanRenderPass), pipelineLayout (VulkanPipelineLayout), vertShader, fragShader (VulkanShaderModule)
        
    - Returns: pipeline (VulkanPipeline userdata)
        
    - Example: pipeline = vulkan.vk_CreateGraphicsPipelines(device, renderPass, pipelineLayout, vertShader, fragShader)
        

---

10. Create Command Buffers and Sync Objects

- Function: vulkan.vk_CreateCommandPool(device, queueFamily)
    
    - Args: device (VulkanDevice), queueFamily (int)
        
    - Returns: commandPool (VulkanCommandPool userdata)
        
    - Example: commandPool = vulkan.vk_CreateCommandPool(device, 0)
        
- Function: vulkan.vk_AllocateCommandBuffers(device, commandPool, count)
    
    - Args: device (VulkanDevice), commandPool (VulkanCommandPool), count (int)
        
    - Returns: commandBuffers (table of VulkanCommandBuffer userdata)
        
    - Example: commandBuffers = vulkan.vk_AllocateCommandBuffers(device, commandPool, 2)
        
- Function: vulkan.vk_CreateSemaphore(device)
    
    - Args: device (VulkanDevice)
        
    - Returns: semaphore (VulkanSemaphore userdata)
        
    - Example: semaphore = vulkan.vk_CreateSemaphore(device)
        
- Function: vulkan.vk_CreateFence(device, signaled)
    
    - Args: device (VulkanDevice), signaled (boolean)
        
    - Returns: fence (VulkanFence userdata)
        
    - Example: fence = vulkan.vk_CreateFence(device, true)
        

---

11. Render Loop

- Function: vulkan.vk_WaitForFences(device, fence)
    
    - Args: device (VulkanDevice), fence (VulkanFence)
        
    - Example: vulkan.vk_WaitForFences(device, fence)
        
- Function: vulkan.vk_ResetFences(device, fence)
    
    - Args: device (VulkanDevice), fence (VulkanFence)
        
    - Example: vulkan.vk_ResetFences(device, fence)
        
- Function: vulkan.vk_AcquireNextImageKHR(device, swapchain, timeout, semaphore, fence)
    
    - Args: device (VulkanDevice), swapchain (VulkanSwapchain), timeout (int), semaphore (VulkanSemaphore), fence (VulkanFence or nil)
        
    - Returns: imageIndex (int)
        
    - Example: imageIndex = vulkan.vk_AcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore, nil)
        
- Function: vulkan.vk_BeginCommandBuffer(cmdBuffer)
    
    - Args: cmdBuffer (VulkanCommandBuffer)
        
    - Example: vulkan.vk_BeginCommandBuffer(cmdBuffer)
        
- Function: vulkan.vk_CmdBeginRenderPass(cmdBuffer, renderPass, framebuffer)
    
    - Args: cmdBuffer (VulkanCommandBuffer), renderPass (VulkanRenderPass), framebuffer (VulkanFramebuffer)
        
    - Example: vulkan.vk_CmdBeginRenderPass(cmdBuffer, renderPass, framebuffer)
        
- Function: vulkan.vk_CmdBindPipeline(cmdBuffer, pipeline)
    
    - Args: cmdBuffer (VulkanCommandBuffer), pipeline (VulkanPipeline)
        
    - Example: vulkan.vk_CmdBindPipeline(cmdBuffer, pipeline)
        
- Function: vulkan.vk_CmdDraw(cmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance)
    
    - Args: cmdBuffer (VulkanCommandBuffer), vertexCount, instanceCount, firstVertex, firstInstance (int)
        
    - Example: vulkan.vk_CmdDraw(cmdBuffer, 3, 1, 0, 0)
        
- Function: vulkan.vk_CmdEndRenderPass(cmdBuffer)
    
    - Args: cmdBuffer (VulkanCommandBuffer)
        
    - Example: vulkan.vk_CmdEndRenderPass(cmdBuffer)
        
- Function: vulkan.vk_EndCommandBuffer(cmdBuffer)
    
    - Args: cmdBuffer (VulkanCommandBuffer)
        
    - Example: vulkan.vk_EndCommandBuffer(cmdBuffer)
        
- Function: vulkan.vk_QueueSubmit(queue, submits, fence)
    
    - Args: queue (VulkanQueue), submits (table of {waitSemaphores, commandBuffers, signalSemaphores}), fence (VulkanFence or nil)
        
    - Example:
        
        lua
        
        ```lua
        vulkan.vk_QueueSubmit(queue, {{waitSemaphores={sem1}, commandBuffers={cmd}, signalSemaphores={sem2}}}, fence)
        ```
        
- Function: vulkan.vk_QueuePresentKHR(queue, presentInfo)
    
    - Args: queue (VulkanQueue), presentInfo (table with waitSemaphores, swapchains)
        
    - Example:
        
        lua
        
        ```lua
        vulkan.vk_QueuePresentKHR(queue, {waitSemaphores={sem}, swapchains={{swapchain=swapchain, imageIndex=imageIndex}}})
        ```
        
- Constants:
    
    - UINT64_MAX = 0xFFFFFFFFFFFFFFFF
        

---

12. Cleanup

- Function: vulkan.vk_QueueWaitIdle(queue)
    
    - Args: queue (VulkanQueue)
        
    - Example: vulkan.vk_QueueWaitIdle(queue)
        
- Destroy Functions:
    
    - vulkan.vk_DestroyFence(device, fence)
        
    - vulkan.vk_DestroySemaphore(device, semaphore)
        
    - vulkan.vk_DestroyCommandPool(device, commandPool)
        
    - vulkan.vk_DestroyPipeline(device, pipeline)
        
    - vulkan.vk_DestroyPipelineLayout(device, pipelineLayout)
        
    - vulkan.vk_DestroyShaderModule(device, shaderModule)
        
    - vulkan.vk_DestroyFramebuffer(device, framebuffer)
        
    - vulkan.vk_DestroyRenderPass(device, renderPass)
        
    - vulkan.vk_DestroyImageView(device, imageView)
        
    - vulkan.vk_DestroySwapchainKHR(device, swapchain)
        
    - vulkan.vk_DestroyDevice(device)
        
    - vulkan.vk_DestroySurfaceKHR(instance, surface)
        
    - vulkan.vk_DestroyInstance(instance)
        
    - Example:
        
        lua
        
        ```lua
        vulkan.vk_DestroyFence(device, fence)
        ```
        
- Function: SDL.SDL_DestroyWindow(window)
    
    - Args: window (lightuserdata)
        
    - Example: SDL.SDL_DestroyWindow(window)
        
- Function: SDL.SDL_Quit()
    
    - Args: None
        
    - Example: SDL.SDL_Quit()
        

---

Pros and Cons

Pros

- Flexibility: Vulkan’s explicit control allows fine-tuned performance optimization.
    
- Cross-Platform: SDL + Vulkan works on Windows, Linux, etc., with minor tweaks.
    
- LuaJIT: Fast scripting with C interop via LuaJIT’s FFI or C bindings.
    
- Validation: Vulkan layers catch errors early (e.g., resource leaks).
    

Cons

- Complexity: Vulkan’s verbosity requires careful resource management (e.g., sync objects, cleanup order).
    
- Boilerplate: Setup (swapchain, pipeline, etc.) is lengthy compared to OpenGL.
    
- Debugging: Errors like missing returns or sync issues need manual tracing.
    
- Shader Dependency: Rendering depends on correctly compiled SPIR-V shaders.
    

---

Full Example

lua

```lua
SDL.SDL_Init(SDL.SDL_INIT_VIDEO)
window = SDL.SDL_CreateWindow("Vulkan", 800, 600, SDL.SDL_WINDOW_VULKAN)
instance = vulkan.create_instance({"VK_KHR_surface", "VK_KHR_win32_surface"}, {"VK_LAYER_KHRONOS_validation"})
surface = SDL.SDL_Vulkan_CreateSurface(window, instance)
device = vulkan.vk_CreateDevice(physicalDevice, {{family=0, count=1}})
graphicsQueue = vulkan.vk_GetDeviceQueue(device, 0, 0)
swapchain = vulkan.vk_CreateSwapchainKHR(device, surface, 800, 600, 2)
swapchainImages = vulkan.vk_GetSwapchainImagesKHR(device, swapchain)
imageViews = {}
for i, img in ipairs(swapchainImages) do imageViews[i] = vulkan.vk_CreateImageView(device, img, 44) end
renderPass = vulkan.vk_CreateRenderPass(device, 44)
framebuffers = {}
for i, view in ipairs(imageViews) do framebuffers[i] = vulkan.vk_CreateFramebuffer(device, renderPass, view, 800, 600) end
vertShader = vulkan.vk_CreateShaderModule(device, io.open("triangle.vert.spv", "rb"):read("*a"))
fragShader = vulkan.vk_CreateShaderModule(device, io.open("triangle.frag.spv", "rb"):read("*a"))
pipelineLayout = vulkan.vk_CreatePipelineLayout(device)
pipeline = vulkan.vk_CreateGraphicsPipelines(device, renderPass, pipelineLayout, vertShader, fragShader)
commandPool = vulkan.vk_CreateCommandPool(device, 0)
commandBuffers = vulkan.vk_AllocateCommandBuffers(device, commandPool, 2)
fences = {vulkan.vk_CreateFence(device, true), vulkan.vk_CreateFence(device, true)}
semaphores = {vulkan.vk_CreateSemaphore(device), vulkan.vk_CreateSemaphore(device)}

local currentFrame = 1
while true do
    vulkan.vk_WaitForFences(device, fences[currentFrame])
    vulkan.vk_ResetFences(device, fences[currentFrame])
    local imageIndex = vulkan.vk_AcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphores[currentFrame], nil)
    local cmd = commandBuffers[currentFrame]
    vulkan.vk_BeginCommandBuffer(cmd)
    vulkan.vk_CmdBeginRenderPass(cmd, renderPass, framebuffers[imageIndex + 1])
    vulkan.vk_CmdBindPipeline(cmd, pipeline)
    vulkan.vk_CmdDraw(cmd, 3, 1, 0, 0)
    vulkan.vk_CmdEndRenderPass(cmd)
    vulkan.vk_EndCommandBuffer(cmd)
    vulkan.vk_QueueSubmit(graphicsQueue, {{waitSemaphores={semaphores[currentFrame]}, commandBuffers={cmd}, signalSemaphores={semaphores[currentFrame]}}}, fences[currentFrame])
    vulkan.vk_QueuePresentKHR(graphicsQueue, {waitSemaphores={semaphores[currentFrame]}, swapchains={{swapchain=swapchain, imageIndex=imageIndex}}})
    currentFrame = (currentFrame % 2) + 1
end

-- Cleanup
vulkan.vk_QueueWaitIdle(graphicsQueue)
for i = 1, 2 do
    vulkan.vk_DestroyFence(device, fences[i])
    vulkan.vk_DestroySemaphore(device, semaphores[i])
end
vulkan.vk_DestroyCommandPool(device, commandPool)
vulkan.vk_DestroyPipeline(device, pipeline)
vulkan.vk_DestroyPipelineLayout(device, pipelineLayout)
vulkan.vk_DestroyShaderModule(device, vertShader)
vulkan.vk_DestroyShaderModule(device, fragShader)
for i = 1, #framebuffers do vulkan.vk_DestroyFramebuffer(device, framebuffers[i]) end
vulkan.vk_DestroyRenderPass(device, renderPass)
for i = 1, #imageViews do vulkan.vk_DestroyImageView(device, imageViews[i]) end
vulkan.vk_DestroySwapchainKHR(device, swapchain)
vulkan.vk_DestroyDevice(device)
vulkan.vk_DestroySurfaceKHR(instance, surface)
vulkan.vk_DestroyInstance(instance)
SDL.SDL_DestroyWindow(window)
SDL.SDL_Quit()
```

---

This cheatsheet should serve as a quick reference for setting up and using your Vulkan + SDL3 renderer. Let me know if you’d like to expand it or focus on specific areas!