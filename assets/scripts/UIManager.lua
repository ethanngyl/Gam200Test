--[[
===============================================================================
File:        UIManager.lua
Author:      Ethan Ng Yong Le
Co Author:   Sim Kah Yan
Email:       n.ethanyongle@digipen.edu, kahyan.sim@digipen.edu
Date:        2026-02-04 (yyyy-mm-dd)
Contribution: Ethan Ng Yong Le 82%(238 Lines of 291 total) Sim Kah Yan 18% (53 lines of 291 total)
-------------------------------------------------------------------------------
Brief:
Central UI coordinator that creates, updates, draws, and destroys all game UI
components (movement AP, attack AP, health, turn indicator, turn scroll).
Supports party/active-character AP and toggles player UI off during enemy turn.

Details:
- Requires UI/APIndicatorUI, AttackAPIndicatorUI, HealthUI, TurnIndicatorUI,
  and ScrollOpen (TurnScrollUI). Holds components in UIManager.components.
- Init(config): Creates movementAP (GetActiveCharacterAP for party), attackAP,
  health, turnIndicator, turnScroll with fixed configs (paths, offsets, layers,
  animation/sprite sheet settings). Sets UIManager.initialized = true.
- Update(dt): Gets camera position; if GetCurrentTurn() == "Enemy", disables
  movementAP, attackAP, health; then calls component:Update(dt, cameraPos) on all.
- Draw(): Calls component:Draw() on any component that has it. IsAPAnimating()
  returns movementAP:IsAnimating(). Destroy() destroys all components and clears state.
- Access: GetComponent(name), GetComponentCount(), EnableComponent(name),
  DisableComponent(name). PrintStatus() for debug logging.

Notes:
- GetActiveCharacterAP() uses GetActiveCharacter + GetEntityAP when available,
  else falls back to GetPlayerAP. Camera position passed as {x,y,z} table.
- Attack AP uses AP_Crystal.png 4x4 (or 5x4) sprite sheet; movement AP uses MovP.png
  with tint/grayscale. Turn scroll uses ScrollOpen.png and text "Your Turn".

Safety:
- Update/Draw/Destroy check UIManager.initialized. Component existence and
  Update/Draw/Destroy methods checked before calling. GetCurrentTurn guarded
  with GetCurrentTurn and GetCurrentTurn() or "Player".

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
===============================================================================
]]

-- ============================================================================
-- UIManager.lua
-- Central UI coordination system
-- ============================================================================
-- Manages all UI components in a unified, organized way
-- Handles initialization, updates, and cleanup for all UI elements
-- ============================================================================

local UIManager = {}

-- Import UI components
local APIndicatorUI = require("UI/APIndicatorUI")
local AttackAPIndicatorUI = require("UI/AttackAPIndicatorUI")
local HealthUI = require("UI/HealthUI")
local TurnIndicatorUI = require("UI/TurnIndicatorUI")
local TurnScrollUI = require("ScrollOpen")
local SkillBubbleHolderUI = require("UI/SkillBubbleHolderUI")

-- ============================================================================
-- STATE
-- ============================================================================

UIManager.components = {}
UIManager.initialized = false
UIManager.cameraMoveThreshold = 0.01

-- ============================================================================
-- SKILL UI STATE BRIDGE
-- ============================================================================
-- Shared state between entity Lua states (PlayerScript) and level Lua state
-- (SkillBubbleHolderUI). PlayerScript pushes data here via CallLevelFunction,
-- and SkillBubbleHolderUI reads from it directly (same Lua state).

_G._skillUIState = {
    -- Per-player skill assignments: [playerIndex] = { ["1"] = "SkillID", ... }
    players = {},
    -- Per-player AP costs: [playerIndex] = { ["1"] = apCost, ... }
    apCosts = {},
    -- Per-player entity IDs: [playerIndex] = entityID
    entityIDs = {},
    -- Currently active (previewed) skill slot key ("1"-"4") or nil
    activeSlotKey = nil,
    -- Which player index is currently active
    activePlayerIndex = nil,
}

-- Called by PlayerScript (via CallLevelFunction) to register a player's skills
-- Args: playerIndex, entityID, slot1SkillID, slot2SkillID, slot3SkillID, slot4SkillID,
--        slot1APCost, slot2APCost, slot3APCost, slot4APCost
function _G.RegisterPlayerSkills(playerIndex, entityID, s1, s2, s3, s4, ap1, ap2, ap3, ap4)
    _G._skillUIState.players[playerIndex] = {}
    _G._skillUIState.apCosts[playerIndex] = {}
    _G._skillUIState.entityIDs[playerIndex] = entityID
    local slots = { s1, s2, s3, s4 }
    local costs = { ap1, ap2, ap3, ap4 }
    for i = 1, 4 do
        local key = tostring(i)
        if slots[i] and slots[i] ~= "" then
            _G._skillUIState.players[playerIndex][key] = slots[i]
            _G._skillUIState.apCosts[playerIndex][key] = costs[i] or 1
        end
    end
end

-- Called by PlayerScript (via CallLevelFunction) when a skill slot is selected
function _G.SetSkillUIActiveSlot(playerIndex, slotKey)
    _G._skillUIState.activePlayerIndex = playerIndex
    if slotKey == "" then slotKey = nil end
    _G._skillUIState.activeSlotKey = slotKey
end

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

-- Helper function to get active character's movement AP (for party system)
local function GetActiveCharacterAP()
    if GetActiveCharacter then
        local activeEntityID = GetActiveCharacter()
        if activeEntityID and activeEntityID > 0 then
            return GetEntityAP(activeEntityID)
        end
    end
    -- Fallback to old single-player API
    return GetPlayerAP()
end

function UIManager.Init(config)
    config = config or {}

    Log("========================================")
    Log("[UIManager] Initializing UI System...")
    Log("========================================")

    -- Create Movement AP Indicator
    UIManager.components.movementAP = APIndicatorUI:New()
    UIManager.components.movementAP:Init({
        maxAP = 5,
        size = 0.06,
        spacing = 0.1,
        offsetX = -0.82,
        offsetY = -0.42,
        layer = 4,
        filledTexture = "assets/UI/MovP.png",
        useTint = true,
        useGray = true,
        emptyGrayAmount = 1.0,
        filledGrayAmount = 0.0,
        emptyTint = { r = 0.45, g = 0.45, b = 0.45 },
        getAPFunc = GetActiveCharacterAP  -- Use active character's AP
    })

    -- Create Attack AP Indicator
    -- AP_Crystal.png is a 4x4 sprite sheet with two animation sequences:
    --   Rows 1-2 (frames 0-7): Idle/filled animation (loops)
    --   Rows 3-4 (frames 8-15): Consume animation (plays once when AP spent)
    UIManager.components.attackAP = AttackAPIndicatorUI:New()
    UIManager.components.attackAP:Init({
        maxAP = 3,
        size = 0.06,
        spacing = 0.1,
        offsetX = -0.64,
        offsetY = -0.32,
        layer = 4,
        filledTexture = "assets/UI/AP_Crystal.png",
        useTint = true,
        useGray = true,
        emptyGrayAmount = 1.0,
        filledGrayAmount = 0.0,
        emptyTint = { r = 0.45, g = 0.45, b = 0.45 },
        filledTint = { r = 0.75, g = 0.85, b = 1.0 },
        -- Sprite sheet config (4 columns x 5 rows, but only 4 rows have content)
        -- Image is 1280x1600, with 320x320 frames. Bottom row (row 5) is empty.
        useAnimatedSprite = true,
        spriteRows = 5,  -- 1600 / 320 = 5 rows (last row is empty)
        spriteCols = 4,  -- 1280 / 320 = 4 columns
        -- Filled animation: frames 0-7 (rows 1-2), loops
        filledStartFrame = 0,
        filledFrameCount = 8,
        -- Consume animation: frames 8-19 (rows 3-5), plays once
        consumeStartFrame = 8,
        consumeFrameCount = 12,  -- 3 rows × 4 columns = 12 frames
        frameTime = 0.1,     -- 100ms per frame
        animationLoop = true
    })

    -- Create Health UI
    UIManager.components.health = HealthUI:New()
    UIManager.components.health:Init({
        maxHP = 5,
        scale = 0.10,
        offsetX = -0.82,
        offsetY = -0.32,
        layer = 4,
        textureBasePath = "assets/UI/Health_"
    })

    -- Create Turn Indicator (Animated sprite sheet)
    UIManager.components.turnIndicator = TurnIndicatorUI:New()
    UIManager.components.turnIndicator:Init({
        offsetX = -0.82,
        offsetY = -0.19,
        scaleX = 0.18,
        scaleY = 0.18,
        layer = 4,
        texture = "assets/UI/End_Turn_Button.png",
        rows = 2,           -- Row 0: Player turn, Row 1: Enemy turn
        cols = 2,           -- 2 columns per row
        frameTime = 0.3     -- Animation speed (seconds per frame)
    })

    -- Create Turn Scroll UI (Your Turn animation)
    UIManager.components.turnScroll = TurnScrollUI:New()
    UIManager.components.turnScroll:Init({
        offsetX = 0.0,
        offsetY = 0.05,
        scaleX = 0.9,
        scaleY = 0.35,
        layer = 6,
        texture = "assets/UI/ScrollOpen.png",
        animName = "ScrollOpen",
        frameTime = 0.1,
        text = "Your Turn",
        textFont = "Sans48",
        textScale = 0.9,
        textColor = {0.0, 0.0, 0.0},
        textStartFrame = 8,
        textEndFrame = 28
    })

    -- Create Skill Bubble Holder (far right side - placeholder for skill icons)
    UIManager.components.skillBubbleHolder = SkillBubbleHolderUI:New()
    UIManager.components.skillBubbleHolder:Init({
        offsetX = 0.82,
        offsetY = -0.15,
        scaleX = 0.15,
        scaleY = 0.55,
        layer = 4,
        texture = "assets/new assets/skill_bubble_holder.png"
    })

    -- TODO: Add more components as needed:
    -- - ChestProgressUI
    -- - PlayerIconsUI (boots, sword)
    -- - MinimapUI
    -- - etc.

    UIManager.initialized = true

    Log("[UIManager] UI System initialized successfully")
    Log("  Components: " .. UIManager.GetComponentCount())
    Log("========================================")
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function UIManager.Update(dt)
    if not UIManager.initialized then
        return
    end

    -- Get camera position once per frame
    local camX, camY, camZ = GetCameraPosition()
    local cameraPos = {x = camX, y = camY, z = camZ}

    -- Check current turn to hide player UI during enemy turn
    local currentTurn = GetCurrentTurn and GetCurrentTurn() or "Player"
    local isEnemyTurn = (currentTurn == "Enemy")

    -- Hide AP indicators during enemy turn (player can't use AP anyway)
    if UIManager.components.movementAP then
        UIManager.components.movementAP:SetEnabled(not isEnemyTurn)
    end
    if UIManager.components.attackAP then
        UIManager.components.attackAP:SetEnabled(not isEnemyTurn)
    end
    -- IMPORTANT: Keep HealthUI enabled even during enemy turn!
    -- This allows players to see their HP decrease when enemies attack
    -- Health is ALWAYS visible (unlike AP which is only relevant during player turn)
    -- Amended for now
    if UIManager.components.health then
        UIManager.components.health:SetEnabled(not isEnemyTurn)  -- Always enabled
    end
    -- Keep skill bubble holder visible during enemy turn
    -- so players can still see skill info

    -- Update all components
    for name, component in pairs(UIManager.components) do
        if component and component.Update then
            component:Update(dt, cameraPos)
        end
    end
end

-- ============================================================================
-- DRAW
-- ============================================================================

function UIManager.Draw()
    if not UIManager.initialized then
        return
    end

    -- Draw all components that have a Draw method
    for name, component in pairs(UIManager.components) do
        if component and component.Draw then
            component:Draw()
        end
    end
end

-- ============================================================================
-- ANIMATION STATE QUERIES
-- ============================================================================

function UIManager.IsAPAnimating()
    if not UIManager.initialized then
        return false
    end

    -- Check if movement AP is animating
    if UIManager.components.movementAP and UIManager.components.movementAP.IsAnimating then
        return UIManager.components.movementAP:IsAnimating()
    end

    return false
end

-- Check if the turn scroll animation is currently playing
function UIManager.IsTurnScrollPlaying()
    if not UIManager.initialized then
        return false
    end

    if UIManager.components.turnScroll and UIManager.components.turnScroll.IsPlaying then
        return UIManager.components.turnScroll:IsPlaying()
    end

    return false
end

-- ============================================================================
-- CLEANUP
-- ============================================================================

function UIManager.Destroy()
    if not UIManager.initialized then
        return
    end

    Log("[UIManager] Destroying UI System...")

    -- Destroy all components
    for name, component in pairs(UIManager.components) do
        if component and component.Destroy then
            component:Destroy()
        end
    end

    UIManager.components = {}
    UIManager.initialized = false

    Log("[UIManager] UI System destroyed")
end

-- ============================================================================
-- COMPONENT ACCESS
-- ============================================================================

function UIManager.GetComponent(name)
    return UIManager.components[name]
end

function UIManager.GetComponentCount()
    local count = 0
    for _ in pairs(UIManager.components) do
        count = count + 1
    end
    return count
end

function UIManager.EnableComponent(name)
    if UIManager.components[name] then
        UIManager.components[name]:SetEnabled(true)
    end
end

function UIManager.DisableComponent(name)
    if UIManager.components[name] then
        UIManager.components[name]:SetEnabled(false)
    end
end

-- ============================================================================
-- DEBUG
-- ============================================================================

function UIManager.PrintStatus()
    Log("========================================")
    Log("[UIManager] Status Report")
    Log("  Initialized: " .. tostring(UIManager.initialized))
    Log("  Components: " .. UIManager.GetComponentCount())
    for name, component in pairs(UIManager.components) do
        local status = component:IsEnabled() and "ENABLED" or "DISABLED"
        Log("    - " .. name .. ": " .. status)
    end
    Log("========================================")
end

return UIManager
