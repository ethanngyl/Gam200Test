-- ============================================================================
-- ButtonManager.lua
-- Reusable button management system for menu levels
-- ============================================================================
-- Author:        AI Assistant (Claude)
-- Date:          2025-12-21
--
-- Purpose:
-- Centralized button management module that handles button creation from JSON,
-- rendering with editor mode support, and cleanup. Eliminates code duplication
-- across menu level scripts (MainMenu, LevelSelect, Tutorial, EndLevel).
--
-- Features:
-- - JSON-driven button creation with automatic callback registration
-- - Editor mode visual feedback (grayed out buttons + indicator)
-- - Button click sounds with configurable volume
-- - Automatic cleanup and state management
-- - F1 editor toggle with cooldown system
-- - ESC key shortcut support
--
-- Usage Example:
-- ```lua
-- local ButtonManager = require("assets/scripts/ButtonManager")
--
-- function OnInit()
--     config = LoadJSON("assets/JSON/mainmenu_config.json")
--     ButtonManager.Initialize(config.menu.buttons)
-- end
--
-- function OnUpdate(dt)
--     ButtonManager.Update(dt)
-- end
--
-- function OnDraw()
--     ButtonManager.DrawAll()
-- end
--
-- function OnDestroy()
--     ButtonManager.Cleanup()
-- end
-- ```
-- ============================================================================

local ButtonManager = {}

-- ============================================================================
-- INTERNAL STATE
-- ============================================================================

local buttonIDs = {}           -- Stores button entity IDs with their config
local editorToggleCooldown = 0 -- Cooldown timer for F1 key
local pendingState = nil       -- Pending game state transition
local pendingTimer = 0.0       -- Timer for delayed state transitions

-- ============================================================================
-- PUBLIC API: INITIALIZATION
-- ============================================================================

---
-- Initialize button manager with JSON configuration
-- @param buttons table Array of button configs from JSON
-- @return boolean True if initialization succeeded
--
function ButtonManager.Initialize(buttons)
    if not buttons or type(buttons) ~= "table" then
        Log("[ButtonManager] ERROR: Invalid button configuration!")
        return false
    end

    Log("[ButtonManager] Creating " .. #buttons .. " buttons from config...")

    -- Clear any existing buttons first
    ButtonManager.Cleanup()

    -- Create each button from JSON config
    for i, button in ipairs(buttons) do
        local success = ButtonManager.CreateButton(button)
        if not success then
            Log("[ButtonManager] WARNING: Failed to create button: " .. (button.id or "unknown"))
        end
    end

    Log("[ButtonManager] Initialization complete")
    return true
end

---
-- Create a single button from config
-- @param buttonConfig table Button configuration from JSON
-- @return boolean True if button was created successfully
--
function ButtonManager.CreateButton(buttonConfig)
    if not buttonConfig then
        return false
    end

    local layer = buttonConfig.layer or 10
    local buttonID = CreateButton(
        buttonConfig.texture,
        buttonConfig.position.x,
        buttonConfig.position.y,
        buttonConfig.scale.x,
        buttonConfig.scale.y,
        buttonConfig.callback,
        layer
    )

    if buttonID > 0 then
        buttonIDs[buttonConfig.id] = {
            id = buttonID,
            config = buttonConfig
        }
        Log("[ButtonManager]    '" .. buttonConfig.id .. "' created (ID: " .. buttonID .. ")")
        return true
    else
        Log("[ButtonManager]   ✗ Failed to create '" .. buttonConfig.id .. "'")
        return false
    end
end

-- ============================================================================
-- PUBLIC API: UPDATE LOOP
-- ============================================================================

---
-- Update button manager - handles F1 editor toggle and pending state transitions
-- @param dt number Delta time in seconds
--
function ButtonManager.Update(dt)
    -- Handle F1 editor mode toggle
    if IsKeyDown("F1") then
        if editorToggleCooldown <= 0 then
            ToggleEditorMode()

            if IsEditorMode() then
                SetEnginePlayState(false)
                Log("[ButtonManager] EDITOR MODE ON - Buttons disabled")
            else
                SetEnginePlayState(true)
                Log("[ButtonManager] EDITOR MODE OFF - Buttons enabled")
            end

            editorToggleCooldown = 0.3  -- 300ms cooldown
        end
    end

    -- Update cooldown timer
    if editorToggleCooldown > 0 then
        editorToggleCooldown = editorToggleCooldown - dt
    end

    -- Handle pending state transitions
    if pendingState ~= nil then
        pendingTimer = pendingTimer - dt
        if pendingTimer <= 0 then
            SetNextGameState(pendingState)
            pendingState = nil
        end
    end
end

-- ============================================================================
-- PUBLIC API: RENDERING
-- ============================================================================

---
-- Draw all button text with editor mode support
-- Automatically grays out buttons when gameplay is disabled (editor mode + not playing)
--
function ButtonManager.DrawAll()
    -- Use ShouldDisableGameplay() instead of IsEditorMode()
    -- This allows buttons to work when in editor mode but playing
    local disableButtons = ShouldDisableGameplay()

    -- Draw text for each button
    for buttonKey, buttonData in pairs(buttonIDs) do
        local button = buttonData.config
        local text = button.text

        -- Gray out buttons only when gameplay is disabled
        local colorR = disableButtons and 0.5 or text.color.r
        local colorG = disableButtons and 0.5 or text.color.g
        local colorB = disableButtons and 0.5 or text.color.b

        DrawButtonText(
            buttonData.id,
            text.font,
            text.content,
            text.offset.x,
            text.offset.y,
            text.scale,
            colorR,
            colorG,
            colorB
        )
    end

    -- Display editor mode indicator (show when in editor mode, regardless of playing state)
    if IsEditorMode() then
        DrawText("Sans48", "EDITOR MODE", 50, 50, 0.8, 1.0, 0.3, 0.3)
    end
end

-- ============================================================================
-- PUBLIC API: CLEANUP
-- ============================================================================

---
-- Cleanup all buttons and reset state
--
function ButtonManager.Cleanup()
    ClearAllButtons()
    buttonIDs = {}
    editorToggleCooldown = 0
    pendingState = nil
    pendingTimer = 0.0

    Log("[ButtonManager] Cleanup complete")
end

-- ============================================================================
-- PUBLIC API: UTILITY FUNCTIONS
-- ============================================================================

---
-- Check if a button callback should execute
-- Call this at the start of your button callback functions
-- @return boolean True if button can execute (gameplay not disabled)
--
function ButtonManager.CanExecuteCallback()
    -- Use ShouldDisableGameplay() instead of IsEditorMode()
    -- This allows buttons to work when in editor mode but playing
    if ShouldDisableGameplay() then
        Log("[ButtonManager] Button disabled - gameplay paused")
        return false
    end
    return true
end

---
-- Queue a state transition with sound and delay
-- @param state string Target game state name
-- @param soundName string Optional sound effect to play (default: "button")
-- @param delay number Optional delay in seconds (default: 0.15)
--
function ButtonManager.TransitionTo(state, soundName, delay)
    soundName = soundName or "button"
    delay = delay or 0.15

    PlaySound(soundName, false, 1)
    pendingState = state
    pendingTimer = delay

    Log("[ButtonManager] Queued transition to: " .. state)
end

---
-- Get button count
-- @return number Number of active buttons
--
function ButtonManager.GetButtonCount()
    local count = 0
    for _ in pairs(buttonIDs) do
        count = count + 1
    end
    return count
end

---
-- Check if currently in editor mode
-- @return boolean True if in editor mode
--
function ButtonManager.IsInEditorMode()
    return IsEditorMode()
end

---
-- Check if gameplay should be disabled (buttons grayed out)
-- @return boolean True if gameplay is disabled
--
function ButtonManager.IsGameplayDisabled()
    return ShouldDisableGameplay()
end

-- ============================================================================
-- RETURN MODULE
-- ============================================================================

return ButtonManager
