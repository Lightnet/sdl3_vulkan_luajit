#include "sdl3_luajit.h"
#include "lauxlib.h"
#include "lualib.h"

typedef struct {
    SDL_Window *window;
} SDL3Window;

static int l_sdl3_SDL_Init(lua_State *L) {
    Uint32 flags = (Uint32)luaL_checkinteger(L, 1);
    bool success = SDL_Init(flags);
    if (!success) {
        const char *err = SDL_GetError();
        lua_pushboolean(L, false);
        lua_pushstring(L, err && *err ? err : "Unknown SDL_Init error");
        return 2;
    }
    lua_pushboolean(L, true);
    return 1;
}

static int l_sdl3_SDL_CreateWindow(lua_State *L) {
    const char *title = luaL_checkstring(L, 1);
    int w = (int)luaL_checkinteger(L, 2);
    int h = (int)luaL_checkinteger(L, 3);
    Uint32 flags = (Uint32)luaL_checkinteger(L, 4);

    SDL_Window *window = SDL_CreateWindow(title, w, h, flags | SDL_WINDOW_VULKAN);  // Add SDL_WINDOW_VULKAN
    if (!window) {
        const char *err = SDL_GetError();
        lua_pushnil(L);
        lua_pushstring(L, err && *err ? err : "Unknown SDL_CreateWindow error");
        return 2;
    }

    SDL3Window *wptr = (SDL3Window *)lua_newuserdata(L, sizeof(SDL3Window));
    wptr->window = window;
    luaL_getmetatable(L, "SDL3Window");
    lua_setmetatable(L, -2);
    return 1;
}

static int l_sdl3_SDL_Delay(lua_State *L) {
    Uint32 ms = (Uint32)luaL_checkinteger(L, 1);
    SDL_Delay(ms);
    return 0;
}

static int l_sdl3_SDL_Quit(lua_State *L) {
    SDL_Quit();
    return 0;
}

static int l_sdl3_SDL_PollEvent(lua_State *L) {
  SDL_Event *event = (SDL_Event *)lua_newuserdata(L, sizeof(SDL_Event));
  if (SDL_PollEvent(event)) {
      luaL_getmetatable(L, "SDL_Event");
      if (lua_isnil(L, -1)) {
          lua_pop(L, 1);
          luaL_newmetatable(L, "SDL_Event");
          lua_pushvalue(L, -1);
          lua_setfield(L, -2, "__index");
          // No __gc needed since this is transient userdata per poll
      }
      lua_setmetatable(L, -2);
      return 1;
  }
  lua_pushnil(L);
  return 1;
}

static int l_sdl3_SDL_GetEventType(lua_State *L) {
  SDL_Event *event = (SDL_Event *)luaL_checkudata(L, 1, "SDL_Event");
  lua_pushinteger(L, event->type);
  return 1;
}

static int l_window_gc(lua_State *L) {
    SDL3Window *wptr = (SDL3Window *)luaL_checkudata(L, 1, "SDL3Window");
    if (wptr->window) {
        SDL_DestroyWindow(wptr->window);
        wptr->window = NULL;
    }
    return 0;
}

static int l_sdl3_SDL_GetKeyFromEvent(lua_State *L) {
  SDL_Event *event = (SDL_Event *)luaL_checkudata(L, 1, "SDL_Event");
  if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
      lua_pushinteger(L, event->key.key);
      return 1;
  }
  lua_pushnil(L);
  lua_pushstring(L, "Event is not a key event");
  return 2;
}

static int l_SDL_GetTicks(lua_State *L) {
  Uint64 ticks = SDL_GetTicks();
  lua_pushinteger(L, ticks);
  return 1;
}

static int l_sdl3_SDL_DestroyWindow(lua_State *L) {
  SDL3Window *wptr = (SDL3Window *)luaL_checkudata(L, 1, "SDL3Window");
  printf("l_sdl3_SDL_DestroyWindow: window=%p\n", (void*)wptr->window);
  if (wptr->window) {
      SDL_DestroyWindow(wptr->window);
      wptr->window = NULL;
      printf("l_sdl3_SDL_DestroyWindow: Window destroyed\n");
  }
  return 0;
}

static const luaL_Reg sdl3_funcs[] = {
    {"SDL_GetTicks", l_SDL_GetTicks},
    {"SDL_Init", l_sdl3_SDL_Init},
    {"SDL_CreateWindow", l_sdl3_SDL_CreateWindow},
    {"SDL_Delay", l_sdl3_SDL_Delay},
    {"SDL_Quit", l_sdl3_SDL_Quit},
    {"SDL_PollEvent", l_sdl3_SDL_PollEvent},
    {"SDL_GetEventType", l_sdl3_SDL_GetEventType},
    {"SDL_GetKeyFromEvent", l_sdl3_SDL_GetKeyFromEvent},
    {"SDL_DestroyWindow", l_sdl3_SDL_DestroyWindow},
    {NULL, NULL}
};

static const luaL_Reg window_mt[] = {
    {"__gc", l_window_gc},
    {NULL, NULL}
};

int luaopen_sdl3(lua_State *L) {
    luaL_newmetatable(L, "SDL3Window");
    luaL_setfuncs(L, window_mt, 0);
    lua_pop(L, 1);

    luaL_newlib(L, sdl3_funcs);

    lua_pushinteger(L, SDL_INIT_VIDEO); lua_setfield(L, -2, "SDL_INIT_VIDEO");
    lua_pushinteger(L, SDL_WINDOW_VULKAN); lua_setfield(L, -2, "SDL_WINDOW_VULKAN");
    lua_pushinteger(L, SDL_EVENT_QUIT); lua_setfield(L, -2, "SDL_EVENT_QUIT");
    lua_pushinteger(L, SDL_EVENT_KEY_DOWN); lua_setfield(L, -2, "SDL_EVENT_KEY_DOWN");
    lua_pushinteger(L, SDLK_LEFT); lua_setfield(L, -2, "SDLK_LEFT");
    lua_pushinteger(L, SDLK_RIGHT); lua_setfield(L, -2, "SDLK_RIGHT");
    lua_pushinteger(L, SDLK_UP); lua_setfield(L, -2, "SDLK_UP");
    lua_pushinteger(L, SDLK_DOWN); lua_setfield(L, -2, "SDLK_DOWN");
    return 1;
}