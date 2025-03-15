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

    SDL_Window *window = SDL_CreateWindow(title, w, h, flags);
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

// New: sdl3.SDL_PollEvent() -> event_type or nil
static int l_sdl3_SDL_PollEvent(lua_State *L) {
    SDL_Event event;
    if (SDL_PollEvent(&event)) {
        lua_pushinteger(L, event.type);  // Return event type (e.g., SDL_EVENT_QUIT)
        return 1;
    }
    lua_pushnil(L);  // No event available
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

static const luaL_Reg sdl3_funcs[] = {
    {"SDL_Init", l_sdl3_SDL_Init},
    {"SDL_CreateWindow", l_sdl3_SDL_CreateWindow},
    {"SDL_Delay", l_sdl3_SDL_Delay},
    {"SDL_Quit", l_sdl3_SDL_Quit},
    {"SDL_PollEvent", l_sdl3_SDL_PollEvent},
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

    // Constants
    lua_pushinteger(L, SDL_INIT_VIDEO);
    lua_setfield(L, -2, "INIT_VIDEO");
    lua_pushinteger(L, SDL_EVENT_QUIT);
    lua_setfield(L, -2, "EVENT_QUIT");
    return 1;
}