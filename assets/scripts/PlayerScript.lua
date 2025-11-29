--[[
===============================================================================
 File:           PlayerScript.lua
 Author:         Converted from PlayerManager.cpp
 Type:           Entity Component Script
 Description:    Player controller component for grid-based movement

 Usage:
   1. Add ScriptComponent to player entity in Entity Inspector
   2. Set script path to "assets/scripts/PlayerScript.lua"
   3. Player will handle arrow key movement automatically

 Features:
 - Arrow key grid movement (UP/DOWN/LEFT/RIGHT)
 - AP consumption per move
 - Turn-based validation (only moves on player turn)
 - Tile interaction (chests, goals)
 - Animation switching based on direction
 - Visual feedback (tile pulse, border outline)
===============================================================================
--]]

-- Entity-specific state (persists between frames)
local entity = nil
local cooldown = 0.0
local gridMovementEnabled = true

-- Animation configuration
local ANIMATIONS = {
    UP = "Idle_back",
    DOWN = "Idle_front",
    LEFT = "Idle_sideview",
    RIGHT = "Idle_sideview"
}

--[[
    Called once when entity is created
    @param entityID The ID of this entity
--]]
function OnInit(entityID)
    entity = entityID
    cooldown = 0.0
    gridMovementEnabled = true

    Log("PlayerScript initialized on entity: " .. tostring(entityID))
end

--[[
    Called every frame
    @param dt Delta time in seconds
--]]
function OnUpdate(dt)
    -- Update cooldown timer
    if cooldown > 0.0 then
        cooldown = cooldown - dt
        if cooldown < 0.0 then
            cooldown = 0.0
        end
    end

    -- Handle player input
    HandleMovement(dt)
end

--[[
    Handle arrow key movement
--]]
function HandleMovement(dt)
    -- Check cooldown
    if cooldown > 0.0 then
        return
    end

    -- Check if it's player's turn
    local turn = GetCurrentTurn()
    if turn ~= "Player" then
        Log("DEBUG: Not player's turn, turn = " .. tostring(turn))
        return
    end

    -- Check player AP
    local ap, maxAP = GetPlayerAP()
    if ap <= 0 then
        Log("DEBUG: Player out of AP: " .. tostring(ap))
        return
    end

    Log("DEBUG: Player turn check passed! AP = " .. tostring(ap) .. "/" .. tostring(maxAP))

    -- Detect arrow key input
    local stepX, stepY = 0, 0
    local animName = nil
    local flip = false

    if IsKeyDown("Up") then
        stepY = 1
        animName = ANIMATIONS.UP
        Log("DEBUG: Up key pressed")
    elseif IsKeyDown("Down") then
        stepY = -1
        animName = ANIMATIONS.DOWN
        Log("DEBUG: Down key pressed")
    elseif IsKeyDown("Left") then
        stepX = -1
        animName = ANIMATIONS.LEFT
        flip = true
        Log("DEBUG: Left key pressed")
    elseif IsKeyDown("Right") then
        stepX = 1
        animName = ANIMATIONS.RIGHT
        flip = false
        Log("DEBUG: Right key pressed")
    else
        return  -- No input
    end

    -- Get current grid position
    local curX, curY = GetPlayerGridPosition()
    if not curX or not curY then
        Log("ERROR: Could not get player position")
        return
    end

    -- Calculate next tile
    local nextX = curX + stepX
    local nextY = curY + stepY

    -- Validate movement
    if not IsValidGridPosition(nextX, nextY) then
        return
    end

    if not IsWalkableTile(nextX, nextY) then
        return
    end

    -- Check tile interactions
    if not CheckTileInteraction(nextX, nextY) then
        return
    end

    -- Visual feedback
    ShowTileBorder(nextX, nextY, 0.22, 200)
    PulseTile(nextX, nextY, 1.15, 150)

    -- Move player
    MovePlayerToTile(nextX, nextY)

    -- Update animation
    if animName then
        LoadPlayerAnimation(animName)
        SetPlayerFlipX(flip)
    end

    -- Consume AP
    ConsumePlayerAP(1)

    -- Set cooldown
    cooldown = 0.2
end

--[[
    Check and handle tile interactions
    @param tileX Grid X coordinate
    @param tileY Grid Y coordinate
    @return true if movement should proceed
--]]
function CheckTileInteraction(tileX, tileY)
    -- Check for chest
    if HasChestAtTile(tileX, tileY) then
        CollectChest(tileX, tileY)
        PlaySound("item_obtained")
        return true
    end

    -- Check for goal
    if HasGoalAtTile(tileX, tileY) then
        local collected, required = GetChestProgress()
        if collected >= required then
            return true  -- Can proceed
        else
            Log("Need " .. (required - collected) .. " more chests!")
            return false  -- Block movement
        end
    end

    return true
end

--[[
    Called when entity is destroyed
--]]
function OnDestroy()
    Log("PlayerScript destroyed on entity: " .. tostring(entity))
end
