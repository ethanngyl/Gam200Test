--[[
===============================================================================
File:        ApplyAnimationsToAllPlayers.lua
Purpose:     Workaround to apply animations to all 3 players manually
Description: This script manually sets up the Warrior animation sprite sheet
             for all 3 players using basic sprite animation configuration.
===============================================================================
]]--

function ApplyAnimationsToAllPlayers()
    print("[ApplyAnimationsToAllPlayers] ========================================")
    print("[ApplyAnimationsToAllPlayers] MANUAL ANIMATION SETUP")
    print("[ApplyAnimationsToAllPlayers] ========================================")

    local players = GetAllPlayers()
    if not players then
        print("[ApplyAnimationsToAllPlayers] ERROR: GetAllPlayers() returned nil!")
        return false
    end

    print("[ApplyAnimationsToAllPlayers] Found " .. #players .. " players")

    -- Apply Warrior idle animation to each player
    for i = 1, #players do
        local pid = players[i]
        if pid and pid ~= 0 then
            print("[ApplyAnimationsToAllPlayers] Setting up Player " .. i .. " (Entity " .. pid .. ")...")

            -- Use SetSpriteAnimationSheet to manually apply Warrior sprite
            local success = SetSpriteAnimationSheet(
                pid,                                                    -- entity ID
                "assets/Warrior/FrontView/WarriorTopDownView.png",     -- texture path
                1,                                                      -- rows
                12,                                                     -- columns
                12,                                                     -- frameCount
                0.55,                                                   -- frameTime
                true                                                    -- loop
            )

            if success then
                print("[ApplyAnimationsToAllPlayers]   ✓ Animation sheet applied successfully")

                -- Set animation to playing
                SetAnimationPlaying(pid, true)
                print("[ApplyAnimationsToAllPlayers]   ✓ Animation set to playing")
            else
                print("[ApplyAnimationsToAllPlayers]   ✗ Failed to apply animation sheet")
            end
        end
    end

    print("[ApplyAnimationsToAllPlayers] Animation setup complete")
    print("[ApplyAnimationsToAllPlayers] ========================================")
    return true
end
