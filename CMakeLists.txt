cmake_minimum_required(VERSION 3.20)
project(LuaJIT_SDL3_Vulkan C)

include(FetchContent)

# Fetch OpenResty's LuaJIT2
FetchContent_Declare(
    luajit
    GIT_REPOSITORY https://github.com/openresty/luajit2.git
    GIT_TAG v2.1-20250117
)
FetchContent_MakeAvailable(luajit)

set(LUAJIT_SRC_DIR ${FETCHCONTENT_BASE_DIR}/luajit-src)
set(LUAJIT_BUILD_DIR ${LUAJIT_SRC_DIR}/src)

add_custom_target(
    BuildLuaJIT
    COMMAND ${CMAKE_COMMAND} -E chdir ${LUAJIT_SRC_DIR}/src msvcbuild.bat
    COMMENT "Building LuaJIT with msvcbuild.bat"
    WORKING_DIRECTORY ${LUAJIT_SRC_DIR}/src
)

add_library(luajit_lib SHARED IMPORTED)
set_target_properties(luajit_lib PROPERTIES
    IMPORTED_LOCATION ${LUAJIT_BUILD_DIR}/lua51.dll
    IMPORTED_IMPLIB ${LUAJIT_BUILD_DIR}/lua51.lib
    INTERFACE_INCLUDE_DIRECTORIES ${LUAJIT_BUILD_DIR}
)
add_dependencies(luajit_lib BuildLuaJIT)

# Fetch SDL3
FetchContent_Declare(
    sdl3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.8
)
FetchContent_MakeAvailable(sdl3)

set(SDL_SHARED ON CACHE BOOL "Build SDL as a shared library" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Build SDL as a static library" FORCE)

# Fetch Vulkan-Headers
FetchContent_Declare(
    vulkan_headers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG vulkan-sdk-1.4.304.1
)
FetchContent_MakeAvailable(vulkan_headers)

# Find Vulkan (requires Vulkan SDK installed)
find_package(Vulkan REQUIRED)

# Add executable
add_executable(hello_world src/main.c src/sdl3_luajit.c src/vulkan_luajit.c)
target_include_directories(hello_world PRIVATE 
    ${sdl3_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LUAJIT_BUILD_DIR}
    ${vulkan_headers_SOURCE_DIR}/include
)
target_link_libraries(hello_world PRIVATE luajit_lib SDL3::SDL3 Vulkan::Vulkan)

# Copy DLLs and main.lua
add_custom_command(TARGET hello_world POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${LUAJIT_BUILD_DIR}/lua51.dll
    $<TARGET_FILE_DIR:hello_world>
    COMMENT "Copying lua51.dll to output directory"
)
add_custom_command(TARGET hello_world POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE_DIR:hello_world>
    COMMENT "Copying SDL3.dll to output directory"
)
add_custom_command(TARGET hello_world POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/main.lua
    $<TARGET_FILE_DIR:hello_world>
    COMMENT "Copying main.lua to output directory"
)

set_target_properties(hello_world PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

# Shader compilation
set(GLSLANG_VALIDATOR "C:/VulkanSDK/1.4.304.1/Bin/glslangValidator.exe")
set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_BIN_DIR ${CMAKE_BINARY_DIR})

add_custom_command(
    OUTPUT ${SHADER_BIN_DIR}/triangle.vert.spv
    COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_SRC_DIR}/triangle.vert -o ${SHADER_BIN_DIR}/triangle.vert.spv
    DEPENDS ${SHADER_SRC_DIR}/triangle.vert
    COMMENT "Compiling triangle.vert to SPIR-V"
)

add_custom_command(
    OUTPUT ${SHADER_BIN_DIR}/triangle.frag.spv
    COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_SRC_DIR}/triangle.frag -o ${SHADER_BIN_DIR}/triangle.frag.spv
    DEPENDS ${SHADER_SRC_DIR}/triangle.frag
    COMMENT "Compiling triangle.frag to SPIR-V"
)

add_custom_target(
    Shaders ALL
    DEPENDS ${SHADER_BIN_DIR}/triangle.vert.spv ${SHADER_BIN_DIR}/triangle.frag.spv
)

add_dependencies(hello_world Shaders)