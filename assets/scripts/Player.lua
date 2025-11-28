--[[
===============================================================================
 File:           Player.lua
 Author:         Converted from PlayerManager.cpp
 Description:    Lua-based player controller for grid movement and actions

 Features:
 - Arrow key grid movement with AP consumption
 - Tile pulse and border outline visual feedback
 - Animation switching based on direction
 - Turn-based movement validation
 - Chest and goal interaction
===============================================================================
--]]

local Player = {}

-- State variables
Player.playerEntity = nil
Player.arrowMoveCooldown = 0.0
Player.gridMovementEnabled = false

-- Animation names for each direction
local ANIMATIONS = {
    UP = "Idle_back",
    DOWN = "Idle_front",
    LEFT = "Idle_sideview",
    RIGHT = "Idle_sideview"
}

--[[
    Initialize the player controller
    @param playerEntityID The entity ID of the player
--]]
function Player.Init(playerEntityID)
    Player.playerEntity = playerEntityID
    Player.arrowMoveCooldown = 0.0
    Player.gridMovementEnabled = true
    Log("Player controller initialized (Entity ID: " .. tostring(playerEntityID) .. ")")
end

--[[
    Update player controller each frame
    @param dt Delta time in seconds
--]]
function Player.Update(dt)
    -- Update cooldown timer
    if Player.arrowMoveCooldown > 0.0 then
        Player.arrowMoveCooldown = Player.arrowMoveCooldown - dt
        if Player.arrowMoveCooldown < 0.0 then
            Player.arrowMoveCooldown = 0.0
        end
    end

    -- Handle arrow key movement
    Player.HandleArrowKeyMovement()
end

--[[
    Handle arrow key movement with grid validation
--]]
function Player.HandleArrowKeyMovement()
    -- Check cooldown to prevent double AP consumption
    if Player.arrowMoveCooldown > 0.0 then
        return
    end

    -- Check if grid movement is enabled
    if not Player.gridMovementEnabled then
        return
    end

    -- Check if it's the player's turn
    local currentTurn = GetCurrentTurn()
    if currentTurn ~= "Player" then
        return
    end

    -- Get player AP
    local ap = GetPlayerAP()
    if ap <= 0 then
        return
    end

    -- Check for arrow key input
    local stepX, stepY = 0, 0
    local animationName = nil
    local flipAnimation = false

    if IsKeyDown("Up") then
        stepY = 1
        animationName = ANIMATIONS.UP
    elseif IsKeyDown("Down") then
        stepY = -1
        animationName = ANIMATIONS.DOWN
    elseif IsKeyDown("Left") then
        stepX = -1
        animationName = ANIMATIONS.LEFT
        flipAnimation = true
    elseif IsKeyDown("Right") then
        stepX = 1
        animationName = ANIMATIONS.RIGHT
        flipAnimation = false
    else
        return -- No movement input
    end

    -- Get current player grid position
    local curX, curY = GetPlayerGridPosition()
    if curX == nil or curY == nil then
        Log("ERROR: Could not get player grid position")
        return
    end

    -- Calculate next tile
    local nextX = curX + stepX
    local nextY = curY + stepY

    -- Validate next tile
    if not IsValidGridPosition(nextX, nextY) then
        return
    end

    if not IsWalkableTile(nextX, nextY) then
        return
    end

    -- Check tile interactions (chests, goals, etc.)
    if not HandleTileInteraction(nextX, nextY) then
        return
    end

    -- Visual feedback
    ShowTileBorder(nextX, nextY, 0.22, 200)
    PulseTile(nextX, nextY, 1.15, 150)

    -- Move player to next tile
    MovePlayerToTile(nextX, nextY)

    -- Switch animation
    if animationName then
        LoadPlayerAnimation(animationName)
        SetPlayerFlipX(flipAnimation)
    end

    -- Consume AP
    ConsumePlayerAP(1)

    -- Set cooldown to prevent double consumption
    Player.arrowMoveCooldown = 0.2
end

--[[
    Handle tile interactions (chests, goals, etc.)
    @param tileX Grid X coordinate
    @param tileY Grid Y coordinate
    @return true if movement should proceed, false if blocked
--]]
function HandleTileInteraction(tileX, tileY)
    -- Check if tile has a chest
    if HasChestAtTile(tileX, tileY) then
        CollectChest(tileX, tileY)
        PlaySound("item_obtained")
        return true
    end

    -- Check if tile has goal
    if HasGoalAtTile(tileX, tileY) then
        local chestsCollected, chestsRequired = GetChestProgress()
        if chestsCollected >= chestsRequired then
            -- Player can proceed to goal
            return true
        else
            -- Block movement - need more chests
            Log("Need " .. (chestsRequired - chestsCollected) .. " more chests!")
            return false
        end
    end

    return true
end

--[[
    Enable or disable grid movement
    @param enabled true to enable, false to disable
--]]
function Player.SetGridMovementEnabled(enabled)
    Player.gridMovementEnabled = enabled
    Log("Grid movement " .. (enabled and "ENABLED" or "DISABLED"))
end

--[[
    Reset player state (called on level reset)
--]]
function Player.Reset()
    Player.arrowMoveCooldown = 0.0
    Player.gridMovementEnabled = true
    Log("Player state reset")
end

return Player
