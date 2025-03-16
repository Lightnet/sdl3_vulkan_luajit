#include "sdl_luajit.h"
#include "vulkan_luajit.h" // Add this to access VulkanInstance and VulkanSurface
#include "lauxlib.h"
#include "lualib.h"

typedef struct {
    SDL_Window *window;
} SDLWindow;


static int l_sdl_SDL_Init(lua_State *L) {
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

static int l_sdl_SDL_CreateWindow(lua_State *L) {
  const char *title = luaL_checkstring(L, 1);
  int w = (int)luaL_checkinteger(L, 2);
  int h = (int)luaL_checkinteger(L, 3);
  Uint32 flags = (Uint32)luaL_checkinteger(L, 4);

  SDL_Window *window = SDL_CreateWindow(title, w, h, flags | SDL_WINDOW_VULKAN); // Restore original behavior
  if (!window) {
      const char *err = SDL_GetError();
      lua_pushnil(L);
      lua_pushstring(L, err && *err ? err : "Unknown SDL_CreateWindow error");
      return 2;
  }

  SDLWindow *wptr = (SDLWindow *)lua_newuserdata(L, sizeof(SDLWindow));
  wptr->window = window;
  luaL_getmetatable(L, "SDLWindow");
  lua_setmetatable(L, -2);
  return 1;
}

static int l_sdl_SDL_Delay(lua_State *L) {
    Uint32 ms = (Uint32)luaL_checkinteger(L, 1);
    SDL_Delay(ms);
    return 0;
}

static int l_sdl_SDL_Quit(lua_State *L) {
    SDL_Quit();
    return 0;
}

static int l_sdl_SDL_PollEvent(lua_State *L) {
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

static int l_sdl_SDL_GetEventType(lua_State *L) {
  SDL_Event *event = (SDL_Event *)luaL_checkudata(L, 1, "SDL_Event");
  lua_pushinteger(L, event->type);
  return 1;
}

static int l_window_gc(lua_State *L) {
    SDLWindow *wptr = (SDLWindow *)luaL_checkudata(L, 1, "SDLWindow");
    if (wptr->window) {
        SDL_DestroyWindow(wptr->window);
        wptr->window = NULL;
    }
    return 0;
}

static int l_sdl_SDL_GetKeyFromEvent(lua_State *L) {
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

static int l_sdl_SDL_DestroyWindow(lua_State *L) {
  SDLWindow *wptr = (SDLWindow *)luaL_checkudata(L, 1, "SDLWindow");
  printf("l_sdl_SDL_DestroyWindow: window=%p\n", (void*)wptr->window);
  if (wptr->window) {
      SDL_DestroyWindow(wptr->window);
      wptr->window = NULL;
      printf("l_sdl_SDL_DestroyWindow: Window destroyed\n");
  }
  return 0;
}


static int l_sdl_SDL_Vulkan_GetInstanceExtensions(lua_State *L) {
  Uint32 extensionCount = 0;
  const char *const *extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
  if (!extensionNames || extensionCount == 0) {
      const char *err = SDL_GetError();
      lua_pushnil(L);
      lua_pushstring(L, err && *err ? err : "No Vulkan extensions available");
      return 2;
  }

  lua_pushinteger(L, extensionCount);
  lua_newtable(L);
  for (Uint32 i = 0; i < extensionCount; i++) {
      lua_pushstring(L, extensionNames[i]);
      lua_rawseti(L, -2, i + 1);
  }

  return 2;
}



static int l_sdl_SDL_Vulkan_CreateSurface(lua_State *L) {
  SDLWindow *wptr = (SDLWindow *)luaL_checkudata(L, 1, "SDLWindow"); // Fixed typo
  VulkanInstance *iptr = (VulkanInstance *)luaL_checkudata(L, 2, "VulkanInstance");

  if (!wptr->window) {
      lua_pushnil(L);
      lua_pushstring(L, "Invalid SDL window");
      return 2;
  }
  if (!iptr->instance) {
      lua_pushnil(L);
      lua_pushstring(L, "Invalid Vulkan instance");
      return 2;
  }

  VkSurfaceKHR surface;
  if (!SDL_Vulkan_CreateSurface(wptr->window, iptr->instance, NULL, &surface)) {
      const char *err = SDL_GetError();
      lua_pushnil(L);
      lua_pushstring(L, err && *err ? err : "Unknown SDL_Vulkan_CreateSurface error");
      return 2;
  }

  VulkanSurface *sptr = (VulkanSurface *)lua_newuserdata(L, sizeof(VulkanSurface));
  sptr->surface = surface;
  sptr->instance = iptr->instance; // Store instance for cleanup
  luaL_getmetatable(L, "VulkanSurface");
  lua_setmetatable(L, -2);
  return 1;
}

static int l_vk_surface_gc(lua_State *L) {
  VulkanSurface *sptr = (VulkanSurface *)luaL_checkudata(L, 1, "VulkanSurface");
  if (sptr->surface && sptr->instance) {
      vkDestroySurfaceKHR(sptr->instance, sptr->surface, NULL);
      sptr->surface = VK_NULL_HANDLE;
  }
  return 0;
}

static const luaL_Reg sdl_funcs[] = {
  {"SDL_GetTicks", l_SDL_GetTicks},
  {"SDL_Init", l_sdl_SDL_Init},
  {"SDL_CreateWindow", l_sdl_SDL_CreateWindow},
  {"SDL_Delay", l_sdl_SDL_Delay},
  {"SDL_Quit", l_sdl_SDL_Quit},
  {"SDL_PollEvent", l_sdl_SDL_PollEvent},
  {"SDL_GetEventType", l_sdl_SDL_GetEventType},
  {"SDL_GetKeyFromEvent", l_sdl_SDL_GetKeyFromEvent},
  {"SDL_DestroyWindow", l_sdl_SDL_DestroyWindow},
  {"SDL_Vulkan_GetInstanceExtensions", l_sdl_SDL_Vulkan_GetInstanceExtensions},
  {"SDL_Vulkan_CreateSurface", l_sdl_SDL_Vulkan_CreateSurface},
  {NULL, NULL}
};

static const luaL_Reg window_mt[] = {
  {"__gc", l_window_gc},
  {NULL, NULL}
};

static const luaL_Reg surface_mt[] = {
  {"__gc", l_vk_surface_gc},
  {NULL, NULL}
};

int luaopen_SDL(lua_State *L) {
  luaL_newmetatable(L, "SDLWindow");
  luaL_setfuncs(L, window_mt, 0);
  lua_pop(L, 1);

  luaL_newmetatable(L, "VulkanSurface"); // Add this
  luaL_setfuncs(L, surface_mt, 0);
  lua_pop(L, 1);

  luaL_newlib(L, sdl_funcs);

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