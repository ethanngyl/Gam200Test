-- TestScript.lua - Simple test for Script Browser feature
-- This script can be attached to any entity to test script functionality

local timer = 0

function OnInit()
    Log("TestScript initialized!")
    Log("Script Browser feature is working correctly!")
end

function OnUpdate(dt)
    timer = timer + dt
    
    -- Print a message every 2 seconds
    if timer >= 2.0 then
        Log("TestScript is running! Timer: " .. string.format("%.1f", timer) .. "s")
        timer = 0
    end
end

function OnDestroy()
    Log("TestScript destroyed")
end


