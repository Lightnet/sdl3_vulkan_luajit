# SDL3 Vulkan LuaJIT

# Required:
 * VS2022 (It need NMake so need to install c/c++ that has it.)
 * CMake
 * vulkan-sdk-1.4.304.1 (current build)

# Libs:
  * SDL3
  * Vulkan-Headers
  * LuaJIT2

# Information:
 Prototyping build for wrapper SDL3, Vulkan for Lua script.

 This was inspired by RayLib Lua build after testing some features and build a prototype.

 
```
sdl3_vulkan_luajit/
├── CMakeLists.txt
├── build/  (generated)
├── include/
│   └── sdl3_luajit.h
├── src/
│   ├── main.c
│   └── sdl3_luajit.c
└── main.lua
```

# Build guide:
 Required VS2022 with c/c++ to build this application for lua script.

 Reason is simple NMake use VS2022. As why not VS2022 just testing. As it does not use IDE but pure make file build.

 Next part is make sure have VS2022 developer power shell to run as NMake need to build. One reason is LuaJIT need to be build in NMake with VS2022 dev power shell mode.

# Refs:
 * https://github.com/stetre/moonvulkan
 * https://github.com/pixeljetstream/luajit_gfx_sandbox

