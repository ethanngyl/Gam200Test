print("=== SCRIPT LOADED ===")

function OnInit()
    -- 'self' is available inside functions
    Log("Hello from Lua script! My entity ID is: " .. self)
    Log("OnInit: Starting initialization...")
    
    -- Try to add a Transform component
    local success = AddTransform(self, 0.0, 0.0)
    
    if success then
        Log("SUCCESS: Transform component added!")
    else
        Log("FAILED: Could not add Transform component")
    end
    
    Log("OnInit: Complete!")
end

function OnUpdate(dt)
    -- This runs every frame
    -- We'll keep it simple and just check if we can read position
    
    local x, y = GetPosition(self)
    if x and y then
        -- Only log every 60 frames (~1 second at 60 FPS) to avoid spam
        if math.random(1, 60) == 1 then
            Log(string.format("Entity at position: (%.2f, %.2f)", x, y))
        end
    end
end

function OnDestroy()
    Log("OnDestroy: Script is being cleaned up for entity " .. self)
end