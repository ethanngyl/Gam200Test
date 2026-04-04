-- ============================================================================
-- SkillSetsLevel.lua
-- Author:        Sim Kah Yan
-- Email:         kahyan.sim@digipen.edu
-- Date:          2026-02-24
-- Contribution:  100%
-- ----------------------------------------------------------------------------
--  JSON-Driven Skill Sets Page with ButtonManager Integration
-- ============================================================================

-- Load ButtonManager module
local ButtonManager = require("assets/scripts/ButtonManager")

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local initialized = false
local config = nil
local backgroundSpriteID = 0
local overlaySprites = {}
local pageIndex = 1
local hoverSkillEntries = {}
local hoverTooltipID = 0
local hoverTooltipVisible = false
local hoveredSkillName = nil
local hoveredSkillDescription = nil
local hoverTooltipX = 0.0
local hoverTooltipY = 0.0
local lastHoveredSkillName = nil

local HOVER_TOOLTIP_TEXTURE = "assets/Menu/Scroll Overlay.png"
local HOVER_TOOLTIP_SCALE_X = 0.58
local HOVER_TOOLTIP_SCALE_Y = 0.30
local HOVER_TOOLTIP_OFFSET_X = 0.62
local HOVER_TOOLTIP_LAYER = 30
local HOVER_FONT = "Jersey20Regular"
local HOVER_TITLE_SCALE = 0.42
local HOVER_BODY_SCALE = 0.28
local HOVER_WRAP_CHARS = 44
local HOVER_MIN_HITBOX = 0.18
local HOVER_TEXT_COLOR = { r = 0.08, g = 0.08, b = 0.08 }
local pageConfigs = {
    "assets/JSON/skillsets_config.json",
    "assets/JSON/skillsets_mage_config.json",
    "assets/JSON/skillsets_berserker_config.json"
}

local function ClearOverlaySprites()
    for overlayID, spriteID in pairs(overlaySprites) do
        if spriteID > 0 then
            DestroyEntity(spriteID)
            Log("Overlay sprite '" .. overlayID .. "' destroyed")
        end
    end
    overlaySprites = {}
end

local function GetMouseFramebufferPosition()
    if not GetMousePosition or not GetFramebufferSize then
        return nil, nil
    end

    local fbW, fbH = GetFramebufferSize()
    if not fbW or not fbH or fbW <= 0 or fbH <= 0 then
        return nil, nil
    end

    local mouseX, mouseY = GetMousePosition()
    return mouseX, (fbH - mouseY)
end

local function WrapText(input, maxChars)
    if not input or input == "" then
        return ""
    end

    local out = {}
    local line = ""
    for word in tostring(input):gmatch("%S+") do
        if line == "" then
            line = word
        elseif (#line + #word + 1) <= maxChars then
            line = line .. " " .. word
        else
            table.insert(out, line)
            line = word
        end
    end
    if line ~= "" then
        table.insert(out, line)
    end
    return table.concat(out, "\n")
end

local function BuildSkillDisplayName(iconID)
    local id = tostring(iconID or "")
    id = id:gsub("^icon_", "")
    id = id:gsub("^[^_]+_", "")
    id = id:gsub("_", " ")
    id = id:gsub("%f[%a]%l", string.upper)
    return id
end

local function BuildHoverSkillEntries()
    hoverSkillEntries = {}
    if not config or not config.menu then return end

    local icons = {}
    for _, overlay in ipairs(config.menu.overlaySprites or {}) do
        if overlay.id and overlay.position and overlay.scale then
            local oid = tostring(overlay.id)
            if oid:match("^icon_") and not oid:match("^icon_bg_") then
                table.insert(icons, overlay)
            end
        end
    end

    local descRows = {}
    for _, btn in ipairs(config.menu.buttons or {}) do
        local bid = tostring(btn.id or "")
        if bid:match("^text_.+_%d+$") and btn.text and btn.position then
            table.insert(descRows, btn)
        end
    end

    table.sort(icons, function(a, b)
        return (a.position.y or 0) > (b.position.y or 0)
    end)
    table.sort(descRows, function(a, b)
        return (a.position.y or 0) > (b.position.y or 0)
    end)

    for i, icon in ipairs(icons) do
        local desc = descRows[i]
        table.insert(hoverSkillEntries, {
            name = BuildSkillDisplayName(icon.id),
            description = (desc and desc.text and desc.text.content) or "No description available.",
            x = icon.position.x,
            y = icon.position.y,
            scaleX = icon.scale.x or 0.12,
            scaleY = icon.scale.y or 0.12
        })
    end

    Log("[SkillSetsHover] built entries: " .. tostring(#hoverSkillEntries))
end

local function EnsureHoverTooltipSprite()
    if hoverTooltipID and hoverTooltipID > 0 then
        return
    end

    hoverTooltipID = SpawnSprite(HOVER_TOOLTIP_TEXTURE, 0.0, 0.0, HOVER_TOOLTIP_SCALE_X, HOVER_TOOLTIP_SCALE_Y, HOVER_TOOLTIP_LAYER)
    Log("[SkillSetsHover] tooltip sprite id: " .. tostring(hoverTooltipID))
    if hoverTooltipID and hoverTooltipID > 0 and SetSpriteVisibility then
        SetSpriteVisibility(hoverTooltipID, false)
    end
end

local function UpdateHoverTooltip()
    hoveredSkillName = nil
    hoveredSkillDescription = nil

    local mouseWX, mouseWY = nil, nil
    if GetMouseWorldPosition then
        mouseWX, mouseWY = GetMouseWorldPosition()
    end

    local mouseX, mouseY = nil, nil
    if (not mouseWX or not mouseWY) then
        mouseX, mouseY = GetMouseFramebufferPosition()
        if not mouseX or not mouseY then
            if hoverTooltipID > 0 and SetSpriteVisibility and hoverTooltipVisible then
                SetSpriteVisibility(hoverTooltipID, false)
            end
            hoverTooltipVisible = false
            return
        end
    end

    local hovered = nil
    for _, entry in ipairs(hoverSkillEntries) do
        local baseHalfW = (entry.scaleX or 0.12) * 0.5
        local baseHalfH = (entry.scaleY or 0.12) * 0.5
        local halfW = math.max(baseHalfW, HOVER_MIN_HITBOX * 0.5)
        local halfH = math.max(baseHalfH, HOVER_MIN_HITBOX * 0.5)

        if mouseWX and mouseWY then
            local left = entry.x - halfW
            local right = entry.x + halfW
            local bottom = entry.y - halfH
            local top = entry.y + halfH
            if mouseWX >= left and mouseWX <= right and mouseWY >= bottom and mouseWY <= top then
                hovered = entry
                break
            end
        end

        if WorldToScreen and mouseX and mouseY then
            local leftScreen, bottomScreen = WorldToScreen(entry.x - halfW, entry.y - halfH, true)
            local rightScreen, topScreen = WorldToScreen(entry.x + halfW, entry.y + halfH, true)
            if leftScreen and bottomScreen and rightScreen and topScreen then
                local minX = math.min(leftScreen, rightScreen)
                local maxX = math.max(leftScreen, rightScreen)
                local minY = math.min(bottomScreen, topScreen)
                local maxY = math.max(bottomScreen, topScreen)
                if mouseX >= minX and mouseX <= maxX and mouseY >= minY and mouseY <= maxY then
                    hovered = entry
                    break
                end
            end
        end
    end

    local show = hovered ~= nil
    if show then
        hoveredSkillName = hovered.name
        hoveredSkillDescription = hovered.description
        hoverTooltipX = hovered.x + HOVER_TOOLTIP_OFFSET_X
        hoverTooltipY = hovered.y
        if hoverTooltipID > 0 then
            SetSpritePosition(hoverTooltipID, hoverTooltipX, hoverTooltipY)
        end
    end

    local currentName = show and hoveredSkillName or nil
    if currentName ~= lastHoveredSkillName then
        if currentName then
            Log("[SkillSetsHover] hovered: " .. tostring(currentName))
        else
            Log("[SkillSetsHover] hovered: none")
        end
        lastHoveredSkillName = currentName
    end

    if hoverTooltipID > 0 and SetSpriteVisibility and hoverTooltipVisible ~= show then
        SetSpriteVisibility(hoverTooltipID, show)
    end
    hoverTooltipVisible = show
end

local function DrawHoverTooltipText()
    if not hoverTooltipVisible or not hoveredSkillName or not DrawText or not WorldToScreen then
        return
    end

    local fbW = GetFramebufferSize()
    if not fbW or fbW <= 0 then return end

    local halfW = HOVER_TOOLTIP_SCALE_X * 0.5
    local halfH = HOVER_TOOLTIP_SCALE_Y * 0.5
    local leftScreen, bottomScreen = WorldToScreen(hoverTooltipX - halfW, hoverTooltipY - halfH, true)
    local rightScreen, topScreen = WorldToScreen(hoverTooltipX + halfW, hoverTooltipY + halfH, true)
    if not leftScreen or not bottomScreen or not rightScreen or not topScreen then
        return
    end

    local minX = math.min(leftScreen, rightScreen)
    local maxY = math.max(bottomScreen, topScreen)
    local scaleRef = fbW / 1920

    local wrapped = WrapText(hoveredSkillDescription, HOVER_WRAP_CHARS)
    local textX = minX + (56 * scaleRef)
    local titleY = maxY - (54 * scaleRef)
    local bodyY = maxY - (100 * scaleRef)
    local spacing = 24 * scaleRef

    DrawText(HOVER_FONT, hoveredSkillName, textX, titleY, HOVER_TITLE_SCALE * scaleRef, HOVER_TEXT_COLOR.r, HOVER_TEXT_COLOR.g, HOVER_TEXT_COLOR.b)

    local lineIndex = 0
    for line in wrapped:gmatch("[^\n]+") do
        DrawText(HOVER_FONT, line, textX, bodyY - (lineIndex * spacing), HOVER_BODY_SCALE * scaleRef, HOVER_TEXT_COLOR.r, HOVER_TEXT_COLOR.g, HOVER_TEXT_COLOR.b)
        lineIndex = lineIndex + 1
    end
end

local function SpawnOverlaySprites()
    if not config or not config.menu.overlaySprites then
        return
    end

    Log("Creating overlay sprites...")
    for i, overlay in ipairs(config.menu.overlaySprites) do
        local spriteID = SpawnSprite(
            overlay.texture,
            overlay.position.x,
            overlay.position.y,
            overlay.scale.x,
            overlay.scale.y,
            overlay.layer,
            overlay.rotation or 0
        )

        if spriteID > 0 then
            overlaySprites[overlay.id] = spriteID
            if overlay.color and SetSpriteColor then
                SetSpriteColor(spriteID, overlay.color.r or 1.0, overlay.color.g or 1.0, overlay.color.b or 1.0)
            end
            Log("  Overlay sprite '" .. overlay.id .. "' created (ID: " .. spriteID .. ")")
        else
            Log("  FAILED to create overlay sprite: " .. overlay.id)
        end
    end
    Log("Overlay sprites complete!")
end

local function LoadPageConfig(index)
    local configPath = pageConfigs[index]
    if not configPath then
        Log("ERROR: Invalid skill sets page index: " .. tostring(index))
        return nil
    end

    local loadedConfig = LoadJSON(configPath)
    if not loadedConfig then
        Log("ERROR: Failed to load JSON configuration: " .. configPath)
        return nil
    end

    config = loadedConfig
    Log("Successfully loaded configuration for: " .. config.menu.name)
    return loadedConfig
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("SkillSets Level Script Initialized (ButtonManager Version)")
    Log("Loading configuration from JSON file...")

    pageIndex = 1
    if not LoadPageConfig(pageIndex) then
        return
    end

    local cam = config.menu.camera
    SetCameraPosition(cam.position.x, cam.position.y, cam.position.z)
    SetCameraZoom(cam.zoom)

    DisableImGui()
    SetEnginePlayState(true)

    -- Create background
    local background = config.menu.background
    Log("Creating background sprite: " .. background.texture)
    backgroundSpriteID = SpawnSprite(
        background.texture,
        background.position.x,
        background.position.y,
        background.scale.x,
        background.scale.y,
        background.layer
    )

    if backgroundSpriteID > 0 then
        Log("Background sprite created (ID: " .. backgroundSpriteID .. ")")
    else
        Log("WARNING: Failed to create background sprite")
    end

    -- Create overlay sprites (scroll, icons, etc.)
    SpawnOverlaySprites()
    BuildHoverSkillEntries()
    EnsureHoverTooltipSprite()

    local music = config.menu.music
    PlayMusic(music.name, 0.8, music.loop)
    Log("Playing skill sets page music: " .. music.name)

    -- Initialize buttons
    ButtonManager.Initialize(config.menu.buttons)

    initialized = true
    Log("Skill sets page initialization complete")
    Log("Press F1 to toggle editor mode")
end

-- ============================================================================
-- BUTTON CALLBACKS
-- ============================================================================

function OnBackButtonClicked()
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    if pageIndex > 1 then
        pageIndex = pageIndex - 1
        Log("Skill sets page back: " .. tostring(pageIndex))
        if LoadPageConfig(pageIndex) then
            ClearOverlaySprites()
            SpawnOverlaySprites()
            BuildHoverSkillEntries()
            ButtonManager.Initialize(config.menu.buttons)
        end
        return
    end

    Log("BACK button clicked! Returning to CONTROL2")
    ButtonManager.TransitionTo("CONTROL2")
end

function OnNextButtonClicked()
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    if pageIndex < #pageConfigs then
        pageIndex = pageIndex + 1
        Log("Skill sets page next: " .. tostring(pageIndex))
        if LoadPageConfig(pageIndex) then
            ClearOverlaySprites()
            SpawnOverlaySprites()
            BuildHoverSkillEntries()
            ButtonManager.Initialize(config.menu.buttons)
        end
        return
    end

    Log("NEXT button clicked! Returning to CONTROL2")
    ButtonManager.TransitionTo("CONTROL2")
end

function OnReturnButtonClicked()
    if not ButtonManager.CanExecuteCallback() then
        return
    end

    Log("RETURN button clicked! Returning to main menu")
    ButtonManager.TransitionTo("mainMenu")
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    UpdateAudio(dt)
    ButtonManager.Update(dt)
    UpdateHoverTooltip()
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDraw
-- ============================================================================

function OnDraw()
    if not config then
        return
    end

    ButtonManager.DrawAll()
    DrawHoverTooltipText()
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("Skill sets page cleanup...")

    -- Keep menu BGM playing across menu page transitions.

    ButtonManager.Cleanup()

    if backgroundSpriteID > 0 then
        DestroyEntity(backgroundSpriteID)
        Log("Background sprite destroyed")
    end

    for overlayID, spriteID in pairs(overlaySprites) do
        if spriteID > 0 then
            DestroyEntity(spriteID)
            Log("Overlay sprite '" .. overlayID .. "' destroyed")
        end
    end

    if hoverTooltipID and hoverTooltipID > 0 then
        DestroyEntity(hoverTooltipID)
    end

    config = nil
    initialized = false
    backgroundSpriteID = 0
    overlaySprites = {}
    hoverSkillEntries = {}
    hoverTooltipID = 0
    hoverTooltipVisible = false
    hoveredSkillName = nil
    hoveredSkillDescription = nil
    hoverTooltipX = 0.0
    hoverTooltipY = 0.0
    lastHoveredSkillName = nil

    Log("Skill sets page cleanup complete")
end

-- ============================================================================
-- DEBUG FUNCTIONS
-- ============================================================================

function PrintConfig()
    if not config then
        Log("No configuration loaded!")
        return
    end

    Log("═══════════════════════════════════════")
    Log("Skill Sets Configuration:")
    Log("  Menu Name: " .. config.menu.name)
    Log("  Music: " .. config.menu.music.name)
    Log("  Camera Zoom: " .. config.menu.camera.zoom)
    Log("  Total Buttons: " .. ButtonManager.GetButtonCount())
    Log("═══════════════════════════════════════")
end
