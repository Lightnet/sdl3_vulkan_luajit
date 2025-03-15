# SDL3 Vulkan LuaJIT

# License: MIT

# Information:
A lightweight prototype integrating SDL3, Vulkan, and LuaJIT to create a simple graphics application scripted in Lua. This project draws a colorful triangle on screen as a proof-of-concept, inspired by the simplicity of Raylib's Lua bindings, but built from scratch with Vulkan for rendering and SDL3 for windowing.

## Notes:
 * Needs work on readme doc.
 * Most be rework from Grok Beta 3. It was not detail but quick mark up.

---

# Project Structure

```text
sdl3_vulkan_luajit/
├── CMakeLists.txt           # CMake configuration file
├── build/                   # Generated build directory (not in repo)
│   ├── hello_world.exe      # Compiled executable
│   ├── main.lua             # Copied Lua script
│   ├── triangle.vert.spv    # Compiled vertex shader
│   └── triangle.frag.spv    # Compiled fragment shader
├── shaders/                 # Source shader files
│   ├── triangle.frag        # Fragment shader (GLSL)
│   └── triangle.vert        # Vertex shader (GLSL)
├── include/                 # Header files
│   ├── sdl3_luajit.h        # SDL3 LuaJIT wrapper header
│   └── vulkan_luajit.h      # Vulkan LuaJIT wrapper header
├── src/                     # Source files
│   ├── main.c               # Entry point (loads LuaJIT)
│   ├── sdl3_luajit.c        # SDL3 LuaJIT wrapper
│   ├── vulkan_luajit.c      # Vulkan LuaJIT wrapper
│   └── test.c               # Optional: Double-checks loading
└── main.lua                 # Main Lua script
```

---

# Features

- SDL3: Handles window creation and event management.
- Vulkan: Renders a hard-coded RGB triangle using SPIR-V shaders.
- LuaJIT: Scripts the application logic, interfacing with SDL3 and Vulkan via custom C wrappers.
- CMake: Automates building and shader compilation.
    

The current demo creates a window and renders a triangle with red, green, and blue vertices on a black background.

---

# Requirements

## Software

- Visual Studio 2022: Must include the Desktop development with C++ workload (provides NMake).
- CMake: Version 3.10 or higher.
- Vulkan SDK: Version 1.4.304.1 (download from [LunarG](https://vulkan.lunarg.com/sdk/home)).
    
## Libraries

- SDL3: Windowing and input (built from source or prebuilt).
- Vulkan-Headers: Vulkan API headers (included with Vulkan SDK).
- LuaJIT 2: Fast Lua interpreter (built from source).
    

---

# Build Guide

This project uses CMake and NMake (from VS2022) for building. Follow these steps to set up and run the demo.

## Prerequisites

1. Install Visual Studio 2022:
    
    - Select the Desktop development with C++ workload during installation to include NMake.
    - Open the Developer PowerShell for VS 2022 for commands (accessible via Start menu or vcvarsall.bat).
        
2. Install CMake:
    
    - Download from [cmake.org](https://cmake.org/download/) and add to your PATH.
        
3. Install Vulkan SDK 1.4.304.1:
    
    - Download from [LunarG](https://vulkan.lunarg.com/sdk/home) and install to C:\VulkanSDK\1.4.304.1 (default path assumed).
  
## Build Steps
 * Use build.bat if on windows other is refs.
 * Not work on other OS.

1. Clone the Repository:
    
    cmd
    ```text
    git clone https://github.com/yourusername/sdl3_vulkan_luajit.git
    cd sdl3_vulkan_luajit
    ```

2. Build the Project:
    If using the windows. Current project folder there is build.bat.

    cmd
    ```text
    ./build.bat
    ```
    
    - This compiles the shaders (triangle.vert and triangle.frag) to SPIR-V and builds hello_world.exe.
        
4. Run the Demo:
    
    cmd
    ```text
    ./run.bat
    ```
    
    - You should see a window with a colored triangle!
---

Expected Output

```text
Vulkan instance extensions:
  1: VK_KHR_surface
  2: VK_KHR_win32_surface
Found 1 physical devices
Queue families for device 1:
  1: flags=15, count=16
  2: flags=12, count=2
  3: flags=14, count=8
  4: flags=44, count=1
  5: flags=76, count=1
Selected graphics queue family: index=0
Logical device created
Swapchain created with extent: 800x600
Graphics queue retrieved
Swapchain images retrieved: 2
Render pass created
Framebuffers created: 2
Vertex shader created
Fragment shader created
Graphics pipeline created
Command pool created
Command buffers allocated: 2
Command buffers recorded
Image available semaphore created
Render finished semaphore created
No arguments provided
Hello from main.lua! Vulkan API: 4210688
//... logs render checks...
```

A window opens showing a triangle with red, green, and blue vertices on a black background. Close the window to exit.

---

## How It Works

- main.c: Initializes LuaJIT and loads main.lua.
- sdl3_luajit.c: Wraps SDL3 functions for Lua (windowing, events).
- vulkan_luajit.c: Wraps Vulkan functions for Lua (instance, device, swapchain, pipeline, rendering).
- main.lua: Orchestrates setup and rendering, drawing a triangle using Vulkan.
- shaders/: GLSL vertex and fragment shaders compiled to SPIR-V with glslangValidator.
    
The triangle’s vertex data is hard-coded in triangle.vert, with colors passed to triangle.frag.

---

## Troubleshooting

- No Triangle: Ensure triangle.vert.spv and triangle.frag.spv are in build/. Rebuild if missing.
- CMake Errors: Verify paths to SDL3, LuaJIT, and Vulkan SDK in the cmake command.
- Crash: Check for Vulkan validation layer errors (enable with Vulkan SDK’s VK_LAYER_KHRONOS_validation).
    
---

## Inspiration & References

- [MoonVulkan](https://github.com/stetre/moonvulkan): Lua bindings for Vulkan.
- [LuaJIT GFX Sandbox](https://github.com/pixeljetstream/luajit_gfx_sandbox): Graphics experiments with LuaJIT.
- Built after exploring Raylib’s Lua bindings, aiming for a Vulkan-based alternative.

---

## Future Plans

- Optimize rendering with fences instead of vkQueueWaitIdle.
- Add vertex buffers for dynamic geometry.
- Support window resizing with swapchain recreation.
    