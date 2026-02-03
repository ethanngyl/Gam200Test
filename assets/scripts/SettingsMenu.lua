--[[
===============================================================================
| File:          SettingsMenu.lua
| Author:        Claude (AI Assistant)
| Date:          2026-02-01
| ------------------------------------------------------------------------------
|  Settings Menu System with Volume Slider
|
|  Features:
|  - Wood background
|  - "Settings" title above volume slider
|  - Back button (top-left) with hover highlight (like MainMenu)
|  - Volume slider with 10 levels (sprite_volumeslider00-09.png)
|
|  Usage:
|    local SettingsMenu = require("SettingsMenu")
|    SettingsMenu.Init()
|    SettingsMenu.Update(dt)
|    SettingsMenu.Draw()
===============================================================================
--]]

local SettingsMenu = {}

-- ============================================================================
-- CONFIGURATION
-- ============================================================================

local config = {
    -- Background
    background = {
        texture = "assets/Menu/WoodBackground.png",
        scale = { x = 2.0, y = 1.5 },
        layer = 52  -- Above pause menu
    },
    
    -- Title
    title = {
        text = "Settings",
        scale = 2.0,
        color = { r = 255, g = 255, b = 255 }
    },
    
    -- Back button (top-left) - uses CreateButton for hover effect
    backButton = {
        texture = "assets/Menu/Ui_btn.png",
        scale = { x = 0.25, y = 0.08 },
        layer = 53,
        offsetX = -0.70,  -- World offset from camera
        offsetY = 0.35,
        -- Text config for DrawButtonText
        text = {
            content = "Back",
            font = "Playfair48",
            scale = 0.7,
            offsetX = -25,
            offsetY = -10,
            color = { r = 255, g = 255, b = 255 }
        }
    },
    
    -- Volume slider
    volumeSlider = {
        -- Sprites: 00 = full (1.0), 09 = empty (0.0)
        textureBase = "assets/UI/sprite_volumeslider",
        scale = { x = 0.5, y = 0.12 },
        layer = 53,
        offsetX = 0.0,
        offsetY = -0.05,
        label = "Volume",
        textScale = 0.8,
        textColor = { r = 0.2, g = 0.15, b = 0.1 }
    }
}

-- ============================================================================
-- STATE
-- ============================================================================

local state = {
    active = false,
    
    -- Entity IDs
    backgroundID = 0,
    backButtonID = 0,  -- Now a CreateButton ID (pointer)
    volumeSliderID = 0,
    
    -- Volume level (0-9, where 0 = max volume, 9 = min volume)
    volumeLevel = 0,
    
    -- Input tracking for volume slider
    wasLeftPressed = false,
    wasRightPressed = false,
    wasEscapePressed = false,
    
    -- Callback when closing
    onClose = nil
}

-- ============================================================================
-- HELPER: Get volume slider texture for current level
-- ============================================================================

local function GetVolumeSliderTexture(level)
    -- level 0-9, with leading zero
    return config.volumeSlider.textureBase .. string.format("%02d", level) .. ".png"
end

-- ============================================================================
-- HELPER: Convert volume level to actual volume (0.0 - 1.0)
-- ============================================================================

local function LevelToVolume(level)
    -- Level 0 = 1.0, Level 9 = 0.0 (actually let's keep some minimum)
    -- Level 0 = 1.0, Level 1 = 0.9, ..., Level 9 = 0.1
    return 1.0 - (level * 0.1)
end

-- ============================================================================
-- HELPER: Convert volume (0.0 - 1.0) to level (0-9)
-- ============================================================================

local function VolumeToLevel(volume)
    -- Clamp volume to valid range
    volume = math.max(0.1, math.min(1.0, volume))
    -- Convert: 1.0 -> 0, 0.9 -> 1, ..., 0.1 -> 9
    local level = math.floor((1.0 - volume) * 10 + 0.5)
    return math.max(0, math.min(9, level))
end

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function SettingsMenu.Init()
    -- Load saved volume from C++ AudioLoader (persisted in audio_config.json)
    if GetMasterVolume then
        local savedVolume = GetMasterVolume()
        state.volumeLevel = VolumeToLevel(savedVolume)
        Log("[SettingsMenu] Loaded saved volume: " .. savedVolume .. " (level " .. state.volumeLevel .. ")")
    else
        state.volumeLevel = 0  -- Default to max volume if API not available
        Log("[SettingsMenu] GetMasterVolume not available, using default")
    end
    
    -- Reset input states
    state.wasLeftPressed = false
    state.wasRightPressed = false
    state.wasEscapePressed = false
    
    Log("[SettingsMenu] Initialized (volume level: " .. state.volumeLevel .. ")")
end

-- ============================================================================
-- OPEN SETTINGS MENU
-- ============================================================================

function SettingsMenu.Open(onCloseCallback)
    if state.active then return end
    
    state.active = true
    state.onClose = onCloseCallback
    
    local camX, camY, camZ = GetCameraPosition()
    
    -- Create background
    state.backgroundID = SpawnSprite(
        config.background.texture,
        camX, camY,
        config.background.scale.x,
        config.background.scale.y,
        config.background.layer
    )
    
    -- Create back button using CreateButton (with hover highlight)
    local backCfg = config.backButton
    state.backButtonID = CreateButton(
        backCfg.texture,
        camX + backCfg.offsetX,
        camY + backCfg.offsetY,
        backCfg.scale.x,
        backCfg.scale.y,
        "OnSettingsBackClicked",  -- Global callback function name
        backCfg.layer
    )
    
    -- Create volume slider
    local sliderCfg = config.volumeSlider
    state.volumeSliderID = SpawnSprite(
        GetVolumeSliderTexture(state.volumeLevel),
        camX + sliderCfg.offsetX,
        camY + sliderCfg.offsetY,
        sliderCfg.scale.x,
        sliderCfg.scale.y,
        sliderCfg.layer
    )
    
    Log("[SettingsMenu] Opened")
end

-- Global callback for back button (called by CreateButton system)
function OnSettingsBackClicked()
    PlaySound("button2", false, 0.7)
    SettingsMenu.Close()
end

-- ============================================================================
-- CLOSE SETTINGS MENU
-- ============================================================================

function SettingsMenu.Close()
    if not state.active then return end
    
    -- Destroy background
    if state.backgroundID and state.backgroundID > 0 then
        DestroyEntity(state.backgroundID)
        state.backgroundID = 0
    end
    
    -- Clear the back button (created with CreateButton)
    ClearAllButtons()
    state.backButtonID = 0
    
    -- Destroy volume slider
    if state.volumeSliderID and state.volumeSliderID > 0 then
        DestroyEntity(state.volumeSliderID)
        state.volumeSliderID = 0
    end
    
    state.active = false
    
    -- Call close callback
    if state.onClose then
        state.onClose()
    end
    
    Log("[SettingsMenu] Closed")
end

-- ============================================================================
-- UPDATE VOLUME SLIDER SPRITE
-- ============================================================================

local function UpdateVolumeSliderSprite()
    -- Apply volume first
    local volume = LevelToVolume(state.volumeLevel)
    SetMasterVolume(volume)
    
    -- Save volume to persistent storage (audio_config.json)
    if SaveMasterVolume then
        SaveMasterVolume(volume)
        Log("[SettingsMenu] Volume saved to config: " .. tostring(volume))
    end
    
    Log("[SettingsMenu] Volume set to: " .. tostring(volume) .. " (level " .. tostring(state.volumeLevel) .. ")")
    
    -- Destroy old slider and create new one with updated texture
    if state.volumeSliderID and state.volumeSliderID > 0 then
        DestroyEntity(state.volumeSliderID)
        state.volumeSliderID = 0
    end
    
    -- Create new slider with correct texture
    local camX, camY, camZ = GetCameraPosition()
    local sliderCfg = config.volumeSlider
    local newTexture = GetVolumeSliderTexture(state.volumeLevel)
    
    state.volumeSliderID = SpawnSprite(
        newTexture,
        camX + sliderCfg.offsetX,
        camY + sliderCfg.offsetY,
        sliderCfg.scale.x,
        sliderCfg.scale.y,
        sliderCfg.layer
    )
    
    Log("[SettingsMenu] Created new slider with texture: " .. newTexture)
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function SettingsMenu.Update(dt)
    if not state.active then return end
    
    -- Input handling
    local isLeftPressed = IsKeyDown("Left") or IsKeyDown("A")
    local isRightPressed = IsKeyDown("Right") or IsKeyDown("D")
    local isEscapePressed = IsKeyDown("Escape")
    
    -- Escape to go back
    if isEscapePressed and not state.wasEscapePressed then
        PlaySound("button", false, 0.5)
        SettingsMenu.Close()
        state.wasEscapePressed = isEscapePressed
        return
    end
    state.wasEscapePressed = isEscapePressed
    
    -- Left/Right to adjust volume
    if isLeftPressed and not state.wasLeftPressed then
        -- Left = decrease volume = increase level number (00->01->02...)
        if state.volumeLevel < 9 then
            state.volumeLevel = state.volumeLevel + 1
            UpdateVolumeSliderSprite()
            PlaySound("button", false, 0.3)
        end
    end
    if isRightPressed and not state.wasRightPressed then
        -- Right = increase volume = decrease level number (09->08->07...)
        if state.volumeLevel > 0 then
            state.volumeLevel = state.volumeLevel - 1
            UpdateVolumeSliderSprite()
            PlaySound("button", false, 0.3)
        end
    end
    state.wasLeftPressed = isLeftPressed
    state.wasRightPressed = isRightPressed
    
    -- ========================================================================
    -- MOUSE INPUT for Volume Slider
    -- ========================================================================
    local mouseX, mouseY = GetMousePosition()
    local fbWidth, fbHeight = GetFramebufferSize()
    local centerX = fbWidth * 0.5
    local centerY = fbHeight * 0.5
    
    local scaleFactorX = fbWidth / 1920
    local scaleFactorY = fbHeight / 1080
    
    -- Check volume slider hover and click
    local sliderCenterX = centerX + (config.volumeSlider.offsetX * fbWidth * 0.5)
    local sliderCenterY = centerY - (config.volumeSlider.offsetY * fbHeight * 0.5)
    local sliderHalfW = 200 * scaleFactorX
    local sliderHalfH = 40 * scaleFactorY
    
    local hoveringSlider = mouseX >= sliderCenterX - sliderHalfW and 
                           mouseX <= sliderCenterX + sliderHalfW and
                           mouseY >= sliderCenterY - sliderHalfH and 
                           mouseY <= sliderCenterY + sliderHalfH
    
    -- Mouse click on slider
    local isMousePressed = IsMouseButtonPressed("Left")
    if isMousePressed and hoveringSlider then
        -- Calculate which part of slider was clicked (left = decrease, right = increase)
        if mouseX < sliderCenterX then
            -- Left side - decrease volume
            if state.volumeLevel < 9 then
                state.volumeLevel = state.volumeLevel + 1
                UpdateVolumeSliderSprite()
            end
        else
            -- Right side - increase volume
            if state.volumeLevel > 0 then
                state.volumeLevel = state.volumeLevel - 1
                UpdateVolumeSliderSprite()
            end
        end
    end
end

-- ============================================================================
-- DRAW
-- ============================================================================

function SettingsMenu.Draw()
    if not state.active then return end
    
    local fbWidth, fbHeight = GetFramebufferSize()
    local centerX = fbWidth * 0.5
    local centerY = fbHeight * 0.5
    
    local scaleFactorX = fbWidth / 1920
    local scaleFactorY = fbHeight / 1080
    local scaleFactor = math.min(scaleFactorX, scaleFactorY)
    
    -- ========================================================================
    -- DRAW BACK BUTTON TEXT (using DrawButtonText for proper positioning)
    -- ========================================================================
    if state.backButtonID and state.backButtonID > 0 then
        local textCfg = config.backButton.text
        DrawButtonText(
            state.backButtonID,
            textCfg.font,
            textCfg.content,
            textCfg.offsetX,
            textCfg.offsetY,
            textCfg.scale,
            textCfg.color.r,
            textCfg.color.g,
            textCfg.color.b
        )
    end
    
    -- ========================================================================
    -- DRAW TITLE ("Settings" - above volume slider)
    -- ========================================================================
    local titleCfg = config.title
    local titleText = titleCfg.text
    local titleScale = titleCfg.scale * scaleFactor
    
    -- Use the manually adjusted position
    local approxTitleWidth = #titleText * 30 * titleScale
    local titleX = centerX - (approxTitleWidth * 0.5) + 50
    local titleY = centerY + 150 * scaleFactorY
    
    DrawText("Playfair48", titleText, titleX, titleY, titleScale,
             titleCfg.color.r, titleCfg.color.g, titleCfg.color.b)
    
    -- ========================================================================
    -- DRAW VOLUME LABEL (above the slider bar)
    -- ========================================================================
    local sliderCfg = config.volumeSlider
    
    local volumeLabelX = centerX -100
    local volumeLabelY = centerY - (sliderCfg.offsetY * fbHeight) - 40 * scaleFactorY
    local volumeScale = sliderCfg.textScale * scaleFactor
    
    local volumeLabel = "< " .. sliderCfg.label .. " >"
    
    DrawText("Playfair48", volumeLabel, volumeLabelX, volumeLabelY, volumeScale,
             sliderCfg.textColor.r, sliderCfg.textColor.g, sliderCfg.textColor.b)
    
    -- ========================================================================
    -- DRAW INSTRUCTIONS (below the slider)
    -- ========================================================================
    local instructionText = "Use the left and right arrow keys or click the slider to adjust the volume."
    local instructionScale = 0.8 * scaleFactor
    local approxInstructionWidth = #instructionText * 15 * instructionScale
    local instructionX = centerX - (approxInstructionWidth * 0.5) - 100
    local instructionY = centerY - 150 * scaleFactorY  -- Below the slider
    
    DrawText("Playfair48", instructionText, instructionX, instructionY, instructionScale,
             255, 255, 255)  -- Dark brown color
end

-- ============================================================================
-- QUERIES
-- ============================================================================

function SettingsMenu.IsActive()
    return state.active
end

-- Get the current saved volume (0.0 - 1.0)
function SettingsMenu.GetSavedVolume()
    -- Use C++ API if available (reads from audio_config.json)
    if GetMasterVolume then
        return GetMasterVolume()
    end
    -- Fallback to local state
    return LevelToVolume(state.volumeLevel)
end

-- Get the volume level (0-9)
function SettingsMenu.GetVolumeLevel()
    return state.volumeLevel
end

-- ============================================================================
-- CLEANUP
-- ============================================================================

function SettingsMenu.Destroy()
    SettingsMenu.Close()
    Log("[SettingsMenu] Destroyed")
end

-- ============================================================================
-- RETURN MODULE
-- ============================================================================

return SettingsMenu
