cmake_minimum_required(VERSION 3.20)
project(LuaJIT_SDL3_Vulkan C)

include(FetchContent)

# Directory for caching build artifacts
set(CACHE_DIR "${CMAKE_BINARY_DIR}/cache")
file(MAKE_DIRECTORY ${CACHE_DIR})
message(STATUS "Cache directory: ${CACHE_DIR}")

# --- LuaJIT Configuration ---
set(LUAJIT_DLL "${CACHE_DIR}/lua51.dll")
set(LUAJIT_LIB "${CACHE_DIR}/lua51.lib")
set(LUAJIT_SRC_DIR "${CMAKE_BINARY_DIR}/_deps/luajit-src")
set(LUAJIT_BUILD_DIR "${LUAJIT_SRC_DIR}/src")

# Check if LuaJIT DLL exists
set(REBUILD_LUAJIT FALSE)
if(NOT EXISTS "${LUAJIT_DLL}")
    set(REBUILD_LUAJIT TRUE)
    message(STATUS "LuaJIT DLL missing: ${LUAJIT_DLL}, fetching and building...")
else()
    message(STATUS "LuaJIT already built at ${LUAJIT_DLL}")
endif()

if(REBUILD_LUAJIT)
    FetchContent_Declare(
        luajit
        GIT_REPOSITORY https://github.com/openresty/luajit2.git
        GIT_TAG v2.1-20250117
    )
    FetchContent_MakeAvailable(luajit)

    add_custom_command(
        OUTPUT "${LUAJIT_DLL}" "${LUAJIT_LIB}"
        COMMAND ${CMAKE_COMMAND} -E chdir "${LUAJIT_BUILD_DIR}" msvcbuild.bat
        COMMAND ${CMAKE_COMMAND} -E copy "${LUAJIT_BUILD_DIR}/lua51.dll" "${LUAJIT_DLL}"
        COMMAND ${CMAKE_COMMAND} -E copy "${LUAJIT_BUILD_DIR}/lua51.lib" "${LUAJIT_LIB}"
        COMMENT "Building LuaJIT"
        WORKING_DIRECTORY "${LUAJIT_BUILD_DIR}"
    )
    add_custom_target(BuildLuaJIT DEPENDS "${LUAJIT_DLL}" "${LUAJIT_LIB}")
endif()

message(STATUS "LuaJIT include and build directory: ${LUAJIT_BUILD_DIR}")

add_library(luajit_lib SHARED IMPORTED)
set_target_properties(luajit_lib 
    PROPERTIES 
    IMPORTED_LOCATION "${LUAJIT_DLL}"
    IMPORTED_IMPLIB "${LUAJIT_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${LUAJIT_BUILD_DIR}"
)
if(REBUILD_LUAJIT)
    add_dependencies(luajit_lib BuildLuaJIT)
endif()

# --- SDL3 Configuration ---
set(SDL_DLL "${CACHE_DIR}/SDL3.dll")
set(SDL_LIB "${CACHE_DIR}/SDL3.lib")
set(SDL_SRC_DIR "${CMAKE_BINARY_DIR}/_deps/sdl3-src")

# Check if SDL3 DLL exists
set(REBUILD_SDL FALSE)
if(NOT EXISTS "${SDL_DLL}")
    set(REBUILD_SDL TRUE)
    message(STATUS "SDL3 DLL missing: ${SDL_DLL}, fetching and building...")
else()
    message(STATUS "SDL3 already built at ${SDL_DLL}")
endif()

if(REBUILD_SDL)
    set(SDL_VULKAN ON CACHE BOOL "Enable Vulkan support" FORCE)
    set(SDL_SHARED ON CACHE BOOL "Build SDL as a shared library" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "Build SDL as a static library" FORCE)

    FetchContent_Declare(
        sdl3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.8
    )
    FetchContent_MakeAvailable(sdl3)

    add_custom_command(
        OUTPUT "${SDL_DLL}" "${SDL_LIB}"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:SDL3::SDL3>" "${SDL_DLL}"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_LINKER_FILE:SDL3::SDL3>" "${SDL_LIB}"
        DEPENDS SDL3::SDL3
        COMMENT "Copying SDL3 build artifacts to cache"
    )
    add_custom_target(BuildSDL3 DEPENDS "${SDL_DLL}" "${SDL_LIB}")
endif()

# --- Vulkan-Headers ---
set(VULKAN_HEADERS_DIR "${CMAKE_BINARY_DIR}/_deps/vulkan-headers-src")
if(NOT EXISTS "${VULKAN_HEADERS_DIR}")
    message(STATUS "Vulkan headers directory missing, fetching...")
endif()
FetchContent_Declare(
    vulkan_headers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG vulkan-sdk-1.4.304.1
)
FetchContent_MakeAvailable(vulkan_headers)
if(NOT EXISTS "${VULKAN_HEADERS_DIR}/include/vulkan/vulkan.h")
    message(WARNING "Vulkan headers not fetched correctly into ${VULKAN_HEADERS_DIR}")
endif()

# Find Vulkan SDK
find_package(Vulkan REQUIRED)

# --- Executable ---
add_executable(hello_world 
    src/main.c
    src/sdl_luajit.c 
    src/vulkan_luajit.c
)
target_include_directories(hello_world PRIVATE 
    "${SDL_SRC_DIR}/include"  # SDL3 headers (SDL.h)
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${LUAJIT_BUILD_DIR}"     # LuaJIT headers (lua.h)
    "${VULKAN_HEADERS_DIR}/include"  # Vulkan headers
)
target_link_libraries(hello_world PRIVATE 
    luajit_lib 
    "${SDL_LIB}"
    Vulkan::Vulkan
)
if(REBUILD_SDL)
    add_dependencies(hello_world BuildSDL3)
endif()

# Copy DLLs and main.lua to output directory
add_custom_command(TARGET hello_world POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LUAJIT_DLL}" "$<TARGET_FILE_DIR:hello_world>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL_DLL}" "$<TARGET_FILE_DIR:hello_world>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/main.lua" "$<TARGET_FILE_DIR:hello_world>"
    COMMENT "Copying DLLs and main.lua to output directory"
)

set_target_properties(hello_world 
    PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

# --- Shader Compilation (Commented for Later) ---
set(GLSLANG_VALIDATOR "C:/VulkanSDK/1.4.304.1/Bin/glslangValidator.exe")
set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_BIN_DIR ${CMAKE_BINARY_DIR})
if(NOT EXISTS ${SHADER_SRC_DIR})
    file(MAKE_DIRECTORY ${SHADER_SRC_DIR})
    message(STATUS "Created shader source directory: ${SHADER_SRC_DIR}")
endif()
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
add_custom_target(Shaders ALL DEPENDS ${SHADER_BIN_DIR}/triangle.vert.spv ${SHADER_BIN_DIR}/triangle.frag.spv)
add_dependencies(hello_world Shaders)