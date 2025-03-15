#ifndef VULKAN_LUAJIT_H
#define VULKAN_LUAJIT_H

#include "lua.h"
#include <SDL3/SDL.h>        // For SDL_Window in SDL_Vulkan_CreateSurface
#include <SDL3/SDL_vulkan.h> // For SDL_Vulkan_CreateSurface
#include <vulkan/vulkan.h>

int luaopen_vulkan(lua_State *L);

#endif