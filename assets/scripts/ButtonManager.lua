-- ============================================================================
-- ButtonManager.lua
-- Reusable button management system for menu levels
-- ============================================================================
-- Author:        Ethan Ng Yong Le 
-- Email:         n.ethanyongle@digipen.edu
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
local transitionDuration = 4.0 -- Fade duration before committing transition
local fadeElapsed = 0.0
local fadeAlpha = 0.0
local fadeOverlayID = 0
local FADE_TEXTURE = "assets/Menu/WoodBackground.png"
local FADE_SCALE_X = 100.0
local FADE_SCALE_Y = 100.0
-- RenderQueue packs layer into 8 bits; use -1 so it wraps to 255 (topmost).
local FADE_LAYER = -1

local function EnsureFadeOverlay()
    if fadeOverlayID and fadeOverlayID > 0 then
        return
    end

    local camX, camY = 0.0, 0.0
    if GetCameraPosition then
        camX, camY = GetCameraPosition()
    end

    fadeOverlayID = SpawnSprite(FADE_TEXTURE, camX, camY, FADE_SCALE_X, FADE_SCALE_Y, FADE_LAYER)
    if fadeOverlayID and fadeOverlayID > 0 then
        if SetSpriteBlendMode then
            -- Force opaque alpha so source texture transparency cannot punch holes
            -- through the fade overlay.
            SetSpriteBlendMode(fadeOverlayID, "AlphaBlend", true)
        end
        if SetSpriteColor then
            SetSpriteColor(fadeOverlayID, 0.0, 0.0, 0.0, 0.0)
        end
        if SetSpriteFilterMode then
            SetSpriteFilterMode(fadeOverlayID, true)
        end
    end
end

local function UpdateFadeOverlay()
    if not fadeOverlayID or fadeOverlayID <= 0 then
        return
    end

    local camX, camY = 0.0, 0.0
    if GetCameraPosition then
        camX, camY = GetCameraPosition()
    end

    local progress = math.max(0.0, math.min(1.0, fadeAlpha))
    local currentWidth = math.max(0.001, FADE_SCALE_X * progress)
    local leftX = camX - (FADE_SCALE_X * 0.5)
    local centerX = leftX + (currentWidth * 0.5)

    if SetScale then
        SetScale(fadeOverlayID, currentWidth, FADE_SCALE_Y)
    end

    SetSpritePosition(fadeOverlayID, centerX, camY)

    -- Opaque wipe: covered region is fully black, uncovered region remains unchanged.
    local overlayAlpha = (progress > 0.0) and 1.0 or 0.0
    SetSpriteColor(fadeOverlayID, 0.0, 0.0, 0.0, overlayAlpha)
end

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

    -- Setup a fullscreen fade overlay used for menu-level transitions.
    EnsureFadeOverlay()

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
        Log("[ButtonManager]   Failed to create '" .. buttonConfig.id .. "'")
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
        fadeElapsed = fadeElapsed + dt
        pendingTimer = pendingTimer - dt

        if transitionDuration > 0 then
            fadeAlpha = math.min(1.0, fadeElapsed / transitionDuration)
        else
            fadeAlpha = 1.0
        end

        UpdateFadeOverlay()

        if pendingTimer <= 0 then
            SetNextGameState(pendingState)
            pendingState = nil
        end

        _G.__transitionWipeProgress = fadeAlpha
    else
        if fadeAlpha ~= 0.0 then
            fadeAlpha = 0.0
            UpdateFadeOverlay()
        end
        _G.__transitionWipeProgress = 0.0
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
        DrawText("Playfair48", "EDITOR MODE", 50, 50, 0.8, 1.0, 0.3, 0.3)
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
    if fadeOverlayID and fadeOverlayID > 0 then
        DestroyEntity(fadeOverlayID)
    end
    fadeOverlayID = 0
    buttonIDs = {}
    editorToggleCooldown = 0
    pendingState = nil
    pendingTimer = 0.0
    transitionDuration = 4.0
    fadeElapsed = 0.0
    fadeAlpha = 0.0

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
    if pendingState ~= nil then
        return false
    end

    if ShouldDisableGameplay() then
        Log("[ButtonManager] Button disabled - gameplay paused")
        return false
    end
    return true
end

---
-- Queue a state transition with sound and fade duration
-- @param state string Target game state name
-- @param soundName string Optional sound effect to play (default: "button")
-- @param delay number Optional fade duration in seconds (default: 4.0)
--
function ButtonManager.TransitionTo(state, soundName, delay)
    soundName = soundName or "button"
    delay = delay or 4.0

    PlaySound(soundName, false, 1)
    pendingState = state
    transitionDuration = math.max(0.0, delay)
    pendingTimer = transitionDuration
    -- Start fade slightly progressed so black appears almost immediately after click.
    fadeElapsed = math.min(0.18, transitionDuration * 0.12)
    if transitionDuration > 0 then
        fadeAlpha = math.min(1.0, fadeElapsed / transitionDuration)
    else
        fadeAlpha = 1.0
    end

    EnsureFadeOverlay()
    UpdateFadeOverlay()

    Log("[ButtonManager] Queued transition to: " .. state)
end

function ButtonManager.GetTransitionFadeAlpha()
    return fadeAlpha
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
