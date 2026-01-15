-- ============================================================================
-- PlayerScript.lua
-- Grid-based player movement component script
-- ============================================================================
-- This script handles player movement on a grid using arrow keys or WASD
-- Features:
-- - Grid-based movement (one tile at a time)
-- - Input handling for arrow keys and WASD
-- - Action Point (AP) consumption
-- - Turn-based movement with cooldown
-- - Visual feedback (tile borders and pulses)
-- ============================================================================

-- ============================================================================
-- SCRIPT STATE
-- ============================================================================

local entityID = 0  -- Will be set by the engine when attached
local moveCooldown = 0.0
local moveCooldownTime = 0.2  -- Seconds between moves (prevents rapid input)
local apCostPerMove = 1  -- AP cost for each movement

-- ============================================================================
-- LIFECYCLE: OnInit
-- ============================================================================

function OnInit(id)
    entityID = id
    Log("[PlayerScript] Initialized for entity " .. entityID)
end

-- ============================================================================
-- LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    -- Update cooldown timer
    if moveCooldown > 0 then
        moveCooldown = moveCooldown - dt
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

    -- WASD input only (arrow keys disabled)
    if IsKeyDown("W") then
        targetY = currentY + 1
        moveAttempted = true
    elseif IsKeyDown("S") then
        targetY = currentY - 1
        moveAttempted = true
    elseif IsKeyDown("A") then
        targetX = currentX - 1
        moveAttempted = true
    elseif IsKeyDown("D") then
        targetX = currentX + 1
        moveAttempted = true
    end

    -- If no movement input, return early
    if not moveAttempted then
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

    local currentAP, maxAP = GetPlayerAP()
    if currentAP < apCostPerMove then
        Log("[PlayerScript] Not enough AP to move (current: " .. currentAP .. ", need: " .. apCostPerMove .. ")")
        -- Show visual feedback for insufficient AP
        PulseTile(targetX, targetY, 0.3, 1.0, 1.0, 0.3)  -- Yellow pulse
        return
    end

    -- ========================================================================
    -- EXECUTE MOVEMENT
    -- ========================================================================

    -- Move the player
    local success = MovePlayerToTile(targetX, targetY)

    if success then
        -- Consume AP
        ConsumePlayerAP(apCostPerMove)

        -- Visual feedback
        ShowTileBorder(targetX, targetY, 0.5)  -- Show border for 0.5 seconds
        PulseTile(targetX, targetY, 0.3, 0.3, 1.0, 0.3)  -- Green pulse

        -- Determine player facing direction and flip sprite
        if targetX > currentX then
            SetPlayerFlipX(false)  -- Moving right - face right
        elseif targetX < currentX then
            SetPlayerFlipX(true)   -- Moving left - face left
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
