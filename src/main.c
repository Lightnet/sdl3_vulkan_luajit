#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "sdl3_luajit.h"

int main(int argc, char *argv[]) {
    const char *script_path = "main.lua";

    if (argc > 1) {
        script_path = argv[1];
        const char *ext = strrchr(script_path, '.');
        if (!ext || strcmp(ext, ".lua") != 0) {
            fprintf(stderr, "Error: Script '%s' must have a .lua extension\n", script_path);
            return 1;
        }
    }

    lua_State *L = luaL_newstate();
    if (!L) {
        fprintf(stderr, "Failed to create LuaJIT state\n");
        return 1;
    }

    luaL_openlibs(L);

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_pushcfunction(L, luaopen_sdl3);
    lua_setfield(L, -2, "sdl3");
    lua_pop(L, 2);

    // Load the script
    if (luaL_loadfile(L, script_path) != LUA_OK) {
        fprintf(stderr, "Error loading script '%s': %s\n", script_path, lua_tostring(L, -1));
        lua_close(L);
        return 1;
    }

    // Push command-line arguments as variadic args to the script
    int nargs = 0;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            lua_pushstring(L, argv[i]);
            nargs++;
        }
    }

    // Run the script with arguments
    if (lua_pcall(L, nargs, 0, 0) != LUA_OK) {
        fprintf(stderr, "Error running script '%s': %s\n", script_path, lua_tostring(L, -1));
        lua_close(L);
        return 1;
    }

    lua_close(L);
    return 0;
}