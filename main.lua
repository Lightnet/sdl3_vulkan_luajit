local sdl3 = require("sdl3")

local ok, err = sdl3.SDL_Init(sdl3.INIT_VIDEO)
if not ok then
    print("SDL3 init failed: " .. (err or "No error message"))
    return
end

local window, err = sdl3.SDL_CreateWindow("LuaJIT + SDL3", 800, 600, 0)
if not window then
    print("Window creation failed: " .. (err or "No error message"))
    sdl3.SDL_Quit()
    return
end

-- Robust argument handling
if arg and #arg > 0 then
    print("Arguments:")
    for i, v in ipairs(arg) do
        print(i, v)
    end
else
    print("No arguments provided")
end

print("Hello from main.lua!")

-- Event loop
local done = false
while not done do
    local event_type = sdl3.SDL_PollEvent()
    while event_type do
        if event_type == sdl3.EVENT_QUIT then
            done = true
        end
        event_type = sdl3.SDL_PollEvent()
    end
    sdl3.SDL_Delay(16)
end

sdl3.SDL_Quit()