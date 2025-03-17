
```lua
-- Render loop
local running = true
while running do
    -- Handle SDL events
    local event = SDL.SDL_PollEvent()
    print("event")
    print(event)
    while event do
        print("event")
        local event_type = SDL.SDL_GetEventType(event)
        if event_type == SDL.SDL_EVENT_QUIT then
            running = false
        end
        event = SDL.SDL_PollEvent()
    end
end
```