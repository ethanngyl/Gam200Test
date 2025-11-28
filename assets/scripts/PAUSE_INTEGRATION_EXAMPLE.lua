--[[
===============================================================================
 EXAMPLE: How to integrate PauseMenu.lua into Level3.lua
===============================================================================

This example shows the minimal changes needed to add pause functionality
to your Level3.lua file.
--]]

-- ============================================================================
-- AT THE TOP OF Level3.lua (after the header comments):
-- ============================================================================

-- Load the PauseMenu module
local PauseMenu = require("PauseMenu")

-- ... rest of your existing variables ...


-- ============================================================================
-- IN OnInit() FUNCTION:
-- ============================================================================

function OnInit()
    Log("========================================")
    Log("LEVEL 3: Tactical Grid Level")
    Log("========================================")

    -- Initialize the pause menu
    PauseMenu.Init()

    -- ... rest of your existing OnInit code ...
end


-- ============================================================================
-- IN OnUpdate(dt) FUNCTION (ADD BEFORE YOUR EXISTING CODE):
-- ============================================================================

function OnUpdate(dt)
    -- Handle pause menu input FIRST (before game logic)
    PauseMenu.Update(dt)

    -- If paused, skip game logic
    -- (Note: LevelLoader already handles this in C++, but you can add
    --  additional checks here if needed for Lua-specific logic)

    -- ... rest of your existing OnUpdate code ...
end


-- ============================================================================
-- IN OnDraw() FUNCTION (ADD AT THE END):
-- ============================================================================

function OnDraw()
    -- ... all your existing OnDraw code ...

    -- Draw pause menu LAST (on top of everything)
    PauseMenu.Draw()
end


-- ============================================================================
-- THAT'S IT! The pause menu will now work with:
-- - P or Escape to toggle pause
-- - Arrow keys or WASD to navigate menu
-- - Enter/Space or 1-3 to select options
-- - Audio automatically mutes when paused
-- ============================================================================
