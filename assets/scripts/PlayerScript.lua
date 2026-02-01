-- ============================================================================
-- PlayerScript.lua
-- Grid-based player movement and animation component script
-- ============================================================================
-- This script handles player movement on a grid using arrow keys or WASD
-- Features:
-- - Grid-based movement (one tile at a time)
-- - Input handling for arrow keys and WASD
-- - Action Point (AP) consumption
-- - Turn-based movement with cooldown
-- - Visual feedback (tile borders and pulses)
-- - Automatic animation state management (Idle/Walk/Attack/Injured/Death)
-- ============================================================================

-- ============================================================================
-- ANIMATION STATE ENUMS
-- ============================================================================

local AnimGroup = {
    Idle = 0,
    Walk = 1,
    Attack = 2,
    Injured = 3,
    Death = 4
}

local AnimDirection = {
    Front = 0,
    Back = 1,
    Side = 2,
    None = 3
}

-- ============================================================================
-- SCRIPT STATE
-- ============================================================================

local entityID = 0  -- Will be set by the engine when attached
local moveCooldown = 0.0
local moveCooldownTime = 0.2  -- Seconds between moves (prevents rapid input)
local apCostPerMove = 1  -- AP cost for each movement

-- Animation state
local currentAnimGroup = AnimGroup.Idle
local currentAnimDirection = AnimDirection.Front
local isFlippedX = false

-- ============================================================================
-- KEY PRESSED HELPER (simulates IsKeyPressed using IsKeyDown)
-- Tracks key state to detect the frame when a key is first pressed
-- ============================================================================
local previousKeyStates = {}

local function IsKeyPressedOnce(keyName)
    local currentDown = IsKeyDown(keyName)
    local wasDown = previousKeyStates[keyName] or false
    previousKeyStates[keyName] = currentDown
    -- Return true only on the frame the key transitions from up to down
    return currentDown and not wasDown
end

-- ============================================================================
-- LIFECYCLE: OnInit
-- ============================================================================

function OnInit(id)
    entityID = id

    -- Initialize animation state
    currentAnimGroup = AnimGroup.Idle
    currentAnimDirection = AnimDirection.Front
    isFlippedX = false

    SetAnimationGroup(entityID, currentAnimGroup)
    SetAnimationDirection(entityID, currentAnimDirection)
    SetAnimationFlipX(entityID, isFlippedX)

    Log("[PlayerScript] Initialized for entity " .. entityID .. " with animation control")
end

-- ============================================================================
-- LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    -- ========================================================================
    -- PARTY SYSTEM: INPUT ROUTING
    -- ========================================================================

    -- CRITICAL: Only process input if this is the active character
    -- Prevents all 3 party members from responding to input simultaneously
    if not IsActiveCharacter(entityID) then
        -- Not this character's turn - do nothing
        return
    end

    -- ========================================================================
    -- ANIMATION STATE MANAGEMENT
    -- ========================================================================

    -- Debug animation triggers disabled (J/K/L keys not mapped in engine)
    -- To re-enable, add J/K/L key mappings to LevelLoader_API.cpp Lua_IsKeyDown()
    --[[
    if IsKeyPressedOnce("K") then  -- Attack
        currentAnimGroup = AnimGroup.Attack
        SetAnimationGroup(entityID, currentAnimGroup)
        SetAnimationLoop(entityID, false)
        Log("[PlayerScript] Attack animation triggered")
    end

    if IsKeyPressedOnce("J") then  -- Injured
        currentAnimGroup = AnimGroup.Injured
        SetAnimationGroup(entityID, currentAnimGroup)
        SetAnimationLoop(entityID, false)
        Log("[PlayerScript] Injured animation triggered")
    end

    if IsKeyPressedOnce("L") then  -- Death
        currentAnimGroup = AnimGroup.Death
        SetAnimationGroup(entityID, currentAnimGroup)
        SetAnimationLoop(entityID, false)
        Log("[PlayerScript] Death animation triggered")
        -- Death animation blocks all movement
        return
    end
    --]]

    -- Block movement during Death animation
    local currentAnim = GetAnimationGroup(entityID)
    if currentAnim == AnimGroup.Death then
        return
    end

    -- ========================================================================
    -- MOVEMENT INPUT
    -- ========================================================================

    -- Update cooldown timer
    if moveCooldown > 0 then
        moveCooldown = moveCooldown - dt

        -- Return to Idle if not moving
        if currentAnimGroup == AnimGroup.Walk then
            currentAnimGroup = AnimGroup.Idle
            SetAnimationGroup(entityID, currentAnimGroup)
        end

        return
    end

    -- Get current grid position
    local currentX, currentY = GetPlayerGridPosition()
    if currentX == nil or currentY == nil then
        return  -- Player position not available
    end

    -- Check for movement input
    local targetX, targetY = currentX, currentY
    local moveAttempted = false
    local moveDirX, moveDirY = 0, 0

    -- WASD input only (arrow keys disabled)
    if IsKeyDown("W") then
        targetY = currentY + 1
        moveDirY = 1
        moveAttempted = true
    elseif IsKeyDown("S") then
        targetY = currentY - 1
        moveDirY = -1
        moveAttempted = true
    elseif IsKeyDown("A") then
        targetX = currentX - 1
        moveDirX = -1
        moveAttempted = true
    elseif IsKeyDown("D") then
        targetX = currentX + 1
        moveDirX = 1
        moveAttempted = true
    end

    -- If no movement input, return to Idle
    if not moveAttempted then
        if currentAnimGroup == AnimGroup.Walk then
            currentAnimGroup = AnimGroup.Idle
            SetAnimationGroup(entityID, currentAnimGroup)
        end
        return
    end

    -- ========================================================================
    -- MOVEMENT VALIDATION
    -- ========================================================================

    -- Check if the target position is valid and walkable
    if not IsValidGridPosition(targetX, targetY) then
        Log("[PlayerScript] Invalid grid position: (" .. targetX .. ", " .. targetY .. ")")
        return
    end

    if not IsWalkableTile(targetX, targetY) then
        Log("[PlayerScript] Tile not walkable: (" .. targetX .. ", " .. targetY .. ")")
        -- Show visual feedback for blocked tile
        PulseTile(targetX, targetY, 0.3, 1.0, 0.3, 0.3)  -- Red pulse
        return
    end

    -- ========================================================================
    -- AP CHECK
    -- ========================================================================

    -- Use entity-based AP API (supports party system)
    local currentAP, maxAP = GetEntityAP(entityID)
    if currentAP < apCostPerMove then
        Log("[PlayerScript] Not enough AP to move (current: " .. currentAP .. ", need: " .. apCostPerMove .. ")")
        -- Show visual feedback for insufficient AP
        PulseTile(targetX, targetY, 0.3, 1.0, 1.0, 0.3)  -- Yellow pulse

        -- Check if this character's turn should end
        if currentAP == 0 then
            Log("[PlayerScript] Character out of AP - ending turn")
            -- Optional: Auto-advance to next character when AP depleted
            -- EndCharacterTurn()
        end

        return
    end

    -- ========================================================================
    -- EXECUTE MOVEMENT
    -- ========================================================================

    -- Move the player
    local success = MovePlayerToTile(targetX, targetY)

    if success then
        -- Consume AP (use entity-based API for party system)
        ConsumeEntityAP(entityID, apCostPerMove)

        -- Visual feedback
        ShowTileBorder(targetX, targetY, 0.5)  -- Show border for 0.5 seconds
        PulseTile(targetX, targetY, 0.3, 0.3, 1.0, 0.3)  -- Green pulse

        -- ====================================================================
        -- UPDATE ANIMATION STATE BASED ON MOVEMENT
        -- ====================================================================

        -- Determine animation direction and flip state
        local newDirection = currentAnimDirection
        local newFlipX = isFlippedX

        if moveDirY > 0 then
            -- Moving up
            newDirection = AnimDirection.Back
        elseif moveDirY < 0 then
            -- Moving down
            newDirection = AnimDirection.Front
        elseif moveDirX > 0 then
            -- Moving right
            newDirection = AnimDirection.Side
            newFlipX = false
        elseif moveDirX < 0 then
            -- Moving left
            newDirection = AnimDirection.Side
            newFlipX = true
        end

        -- Update direction if changed
        if newDirection ~= currentAnimDirection then
            currentAnimDirection = newDirection
            SetAnimationDirection(entityID, currentAnimDirection)
        end

        -- Update flip state if changed
        if newFlipX ~= isFlippedX then
            isFlippedX = newFlipX
            SetAnimationFlipX(entityID, isFlippedX)
        end

        -- Switch to Walk animation (unless in special state)
        if currentAnimGroup ~= AnimGroup.Attack and
           currentAnimGroup ~= AnimGroup.Injured and
           currentAnimGroup ~= AnimGroup.Death then
            currentAnimGroup = AnimGroup.Walk
            SetAnimationGroup(entityID, currentAnimGroup)
            SetAnimationPlaying(entityID, true)
        end

        -- Check for chest collection
        if HasChestAtTile(targetX, targetY) then
            CollectChest(targetX, targetY)
            Log("[PlayerScript] Collected chest at (" .. targetX .. ", " .. targetY .. ")")
        end

        -- Check for goal completion
        if HasGoalAtTile(targetX, targetY) then
            Log("[PlayerScript] Reached goal! Level complete!")
            -- Goal completion is handled by C++ TurnSystem
        end

        -- Set movement cooldown
        moveCooldown = moveCooldownTime

        Log("[PlayerScript] Moved to (" .. targetX .. ", " .. targetY .. ") - AP remaining: " .. (currentAP - apCostPerMove))
    else
        Log("[PlayerScript] Failed to move to (" .. targetX .. ", " .. targetY .. ")")
    end
end

-- ============================================================================
-- LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("[PlayerScript] Destroyed for entity " .. entityID)
end
