--[[
===============================================================================
 File:           AnimationController.lua
 Author:         
 Date:           2026-01-16
 ------------------------------------------------------------------------------
 Animation Controller Script

 Purpose:
    Reusable animation control logic extracted from MovementSystem.cpp.
    Handles animation state transitions, direction changes, and frame management
    based on entity movement and input.

 Features:
    - Automatic Idle/Walk transitions based on movement
    - Direction-based animation selection (Front/Back/Side)
    - Horizontal sprite flipping for left/right facing
    - Attack/Injured/Death animation triggers
    - Movement blocking during Death animation

 Usage:
    Attach this script to any entity with SpriteAnimation component:

    AddScriptComponentToEntity(entityID, "assets/scripts/AnimationController.lua")

 Required Components:
    - SpriteAnimation: Animation state and frame data
    - Movement: Movement direction for animation transitions (optional)
    - Transform: Position and scale (optional for blocking)

 Configuration:
    Animations must be loaded via LoadAnimationConfig() before use.
    The script uses AnimGroup and AnimDirection enums to select animations.

 Animation States:
    - Idle: Default state when not moving
    - Walk: Active when entity is moving
    - Attack: Triggered by KEY_K (one-shot)
    - Injured: Triggered by KEY_J (one-shot)
    - Death: Triggered by KEY_L (one-shot, blocks movement)
    
 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
]]--

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
-- ENTITY STATE
-- ============================================================================

local entityID = 0
local characterType = "Warrior"  -- Default character type (Warrior, Mage, Rogue)
local currentGroup = AnimGroup.Idle
local currentDirection = AnimDirection.Front
local isFlippedX = false
local movementThreshold = 0.01  -- Minimum movement to trigger Walk animation

-- ============================================================================
-- LIFECYCLE FUNCTIONS
-- ============================================================================

--[[
    OnInit()
    Called once when the script is attached to an entity
]]--
function OnInit()
    entityID = self  -- 'self' is the entity ID from ScriptSystem

    -- Initialize animation state
    currentGroup = AnimGroup.Idle
    currentDirection = AnimDirection.Front
    isFlippedX = false

    -- Character type will be set externally via SetCharacterType()
    -- Default is "Warrior" if not set

    -- Set initial animation group and direction
    SetAnimationGroup(entityID, currentGroup)
    SetAnimationDirection(entityID, currentDirection)
    SetAnimationFlipX(entityID, isFlippedX)

    Log("AnimationController: Initialized for entity " .. entityID .. " (Character: " .. characterType .. ")")
end

--[[
    OnUpdate(dt)
    Called every frame to update animation state

    @param dt Delta time in seconds
]]--
function OnUpdate(dt)
    -- Get movement direction
    local dirX, dirY = GetMovementDirection(entityID)

    -- Check if entity is moving
    local moving = math.abs(dirX) > movementThreshold or math.abs(dirY) > movementThreshold

    -- Get current animation group
    local animGroup = GetAnimationGroup(entityID)

    -- =========================================================================
    -- DEATH ANIMATION OVERRIDE
    -- =========================================================================
    -- Death animation always takes priority and blocks all other logic
    if animGroup == AnimGroup.Death then
        -- Movement is already blocked in MovementSystem
        return
    end

    -- =========================================================================
    -- DIRECTION UPDATES (only when moving)
    -- =========================================================================
    if moving then
        local newDirection = currentDirection
        local newFlipX = isFlippedX

        -- Determine direction based on movement vector
        if dirY > 0 then
            -- Moving up
            newDirection = AnimDirection.Back
        elseif dirY < 0 then
            -- Moving down
            newDirection = AnimDirection.Front
        elseif dirX > 0 then
            -- Moving right
            newDirection = AnimDirection.Side
            newFlipX = false
        elseif dirX < 0 then
            -- Moving left
            newDirection = AnimDirection.Side
            newFlipX = true
        end

        -- Update direction if changed
        if newDirection ~= currentDirection then
            currentDirection = newDirection
            SetAnimationDirection(entityID, currentDirection)
        end

        -- Update flip state if changed
        if newFlipX ~= isFlippedX then
            isFlippedX = newFlipX
            SetAnimationFlipX(entityID, isFlippedX)
        end
    end
    -- DO NOT change direction when idle - preserves last facing direction

    -- =========================================================================
    -- ANIMATION GROUP TRANSITIONS
    -- =========================================================================
    if moving then
        -- Switch to Walk if not in special animation (Attack, Injured, Death)
        if animGroup ~= AnimGroup.Attack and
           animGroup ~= AnimGroup.Injured and
           animGroup ~= AnimGroup.Death then
            if animGroup ~= AnimGroup.Walk then
                currentGroup = AnimGroup.Walk
                SetAnimationGroup(entityID, currentGroup)
                SetAnimationPlaying(entityID, true)
            end
        end
    else
        -- Switch to Idle when stopped (only from Walk)
        if animGroup == AnimGroup.Walk then
            currentGroup = AnimGroup.Idle
            SetAnimationGroup(entityID, currentGroup)
        end
    end

    -- =========================================================================
    -- MANUAL ANIMATION TRIGGERS (Debug Keys)
    -- =========================================================================
    -- These keys manually trigger special animations for testing

    if IsKeyPressed(75) then  -- KEY_K = Attack
        currentGroup = AnimGroup.Attack
        SetAnimationGroup(entityID, currentGroup)
        SetAnimationLoop(entityID, false)  -- One-shot animation
        Log("AnimationController: Attack animation triggered")
    end

    if IsKeyPressed(74) then  -- KEY_J = Injured
        currentGroup = AnimGroup.Injured
        SetAnimationGroup(entityID, currentGroup)
        SetAnimationLoop(entityID, false)  -- One-shot animation
        Log("AnimationController: Injured animation triggered")
    end

    if IsKeyPressed(76) then  -- KEY_L = Death
        currentGroup = AnimGroup.Death
        SetAnimationGroup(entityID, currentGroup)
        SetAnimationLoop(entityID, false)  -- One-shot animation
        Log("AnimationController: Death animation triggered")
    end
end

--[[
    OnDestroy()
    Called when the script is removed or entity is destroyed
]]--
function OnDestroy()
    Log("AnimationController: Destroyed for entity " .. entityID)
end

-- ============================================================================
-- HELPER FUNCTIONS
-- ============================================================================

--[[
    SetCharacterType(type)
    Sets the character type for animation selection

    @param type Character type string ("Warrior", "Mage", "Rogue")

    USAGE: Call this after attaching the script to set character-specific animations
    Example:
        AddScriptComponentToEntity(entityID, "assets/scripts/AnimationController.lua")
        -- Then from another script or level script:
        -- SetEntityCharacterType(entityID, "Mage")
]]--
function SetCharacterType(type)
    characterType = type
    Log("AnimationController: Entity " .. entityID .. " set to character type: " .. characterType)

    -- Re-apply current animation with new character type
    SetAnimationGroup(entityID, currentGroup)
    SetAnimationDirection(entityID, currentDirection)
end

-- Export to global scope so it can be called from level scripts
_G.SetEntityCharacterType = function(entity, type)
    -- This is a workaround since we can't directly call functions in entity scripts
    -- The proper way is to implement SetCharacterAnimationPrefix(entityID, prefix) in C++
    Log("WARNING: SetEntityCharacterType() called but character type is per-script instance")
    Log("You need to implement character-specific animations in C++ or use separate scripts")
end

--[[
    GetMovementDirection(entity)
    Returns the current movement direction of an entity

    @param entity Entity ID
    @return dirX, dirY Movement vector components (0 if no Movement component)
]]--
function GetMovementDirection(entity)
    -- This function needs to be implemented in C++ API
    -- For now, return 0,0 if not available
    if GetEntityMovementDirection then
        return GetEntityMovementDirection(entity)
    else
        return 0, 0
    end
end
