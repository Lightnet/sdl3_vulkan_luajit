#ifndef SDL3_LUAJIT_H
#define SDL3_LUAJIT_H

#include "lua.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h> // For SDL_Vulkan_CreateSurface
//#include <vulkan/vulkan.h>

int luaopen_SDL(lua_State *L);

#endif