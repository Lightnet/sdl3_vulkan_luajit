#ifndef SDL3_LUAJIT_H
#define SDL3_LUAJIT_H

#include "lua.h"
#include <SDL3/SDL.h>

int luaopen_sdl3(lua_State *L);

#endif