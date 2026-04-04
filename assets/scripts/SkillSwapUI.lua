--[[
===============================================================================
 File:          SkillSwapUI.lua
 Authors:       Josh Ong
 Co-Authors:    
 Date:          
 Contribution:  
 ------------------------------------------------------------------------------

 Skill Swap UI - A character loadout interface for swapping skills between a fixed set of options.

 Brief:
    - Provides a UI for players to customize their skill loadout by swapping out existing skills for new ones.
    - Displays the current skills and offered alternatives side by side, with character portraits and themed backgrounds.
    - Offers tooltips with skill descriptions when hovering over options.
    - Saves the chosen loadout to a JSON file for use in gameplay.

 Features:
    - Character-specific skill pools with random offers that exclude currently equipped skills.
    - Camera-relative UI elements that maintain consistent positioning even if the camera moves.
    - Animated character portraits and themed scroll backgrounds for visual flair.
    - Hover tooltips that show skill names, descriptions, and AP costs in a styled overlay. 


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
]]--

local SkillSwapUI = {}

-- ============================================================================
-- STATE
-- ============================================================================

local active        = false
local onDone        = nil
local currentChar   = 1

-- Camera position locked when the UI opens; all subsequent pages use the same origin
-- so sprites and buttons never drift when the camera moves between frames.
local lockedCamX    = 0
local lockedCamY    = 0

local bgSpriteID    = 0
local charSpriteID  = 0
local charScrollID  = 0
local buttonIDs     = {}

local loadout = {
    [1] = {},
    [2] = {},
    [3] = {},
}

local skillDefs    = {}
local allSkillIDs  = {}
local skillOffers  = { {}, {}, {} }
local chosenSkill  = { nil, nil, nil }

local hoverTooltipID = 0
local hoverTooltipVisible = false
local hoveredSkillID = nil
local hoveredSkillName = nil
local hoveredSkillDescription = nil
local hoveredSkillCost = 0
local hoverTooltipX = 0.0
local hoverTooltipY = 0.0

local HOVER_TOOLTIP_TEXTURE = "assets/Menu/Scroll Overlay.png"
local HOVER_TOOLTIP_SCALE_X = 0.42
local HOVER_TOOLTIP_SCALE_Y = 0.24
local HOVER_TOOLTIP_OFFSET_X = 0.58
local HOVER_TOOLTIP_LAYER = 56
local HOVER_FONT = "Jersey20Regular"
local HOVER_TITLE_SCALE = 0.40
local HOVER_COST_SCALE = 0.34
local HOVER_BODY_SCALE = 0.29
local HOVER_TEXT_COLOR = { r = 0.08, g = 0.08, b = 0.08 }
local HOVER_WRAP_CHARS = 36
local HOVER_PADDING_X_PX = 62
local HOVER_HEADER_TOP_INSET_PX = 56
local HOVER_DESC_START_INSET_PX = 102
local HOVER_DESC_LINE_SPACING_PX = 24

-- CHANGED: "Rogue" -> "Berserker"
local charNames = { "Warrior", "Mage", "Berserker" }
local charColors = {
    { 1.0, 0.7, 0.3 },
    { 0.4, 0.6, 1.0 },
    { 0.3, 1.0, 0.5 },  -- CHANGED: Berserker keeps green, change if you want a different color
}

local charAnims = {
    [1] = {
        sprite = "assets/Warrior/FrontView/WarriorTopDownView.png",
        rows = 1, columns = 12, frameCount = 12,
        frameTime = 0.55, loop = true
    },
    [2] = {
        sprite = "assets/Mage/FrontView/Mage_Idle_Front-Sheet.png",
        rows = 1, columns = 12, frameCount = 12,
        frameTime = 0.55, loop = true
    },
    [3] = {
        sprite = "assets/Berserker/FrontView/Berserker_Walk_Front-Sheet.png",
        rows = 1, columns = 6, frameCount = 6,
        frameTime = 0.55, loop = true
    },
}

-- ============================================================================
-- LAYOUT
-- ============================================================================

local BG_LAYER   = 50
local SPR_LAYER  = 52
local BTN_LAYER  = 53

-- Button texture
local SCROLL_TEXTURE = "assets/Menu/Scroll Overlay.png"

-- Character sprite (left, big)
local SPRITE_X      = -0.42
local SPRITE_Y      = -0.05
local SPRITE_SCALE  = 0.55

-- Scroll behind character (rotated 90 degrees)
local CHAR_SCROLL_LAYER = 51

-- Current skills column (center-left)
local CURRENT_X         = -0.05
local CURRENT_START_Y   = 0.25
local CURRENT_SPACING   = 0.12
local SLOT_W            = 0.28
local SLOT_H            = 0.10

-- Offer skills column (right, with more gap from current)
local OFFER_X           = 0.50
local OFFER_START_Y     = 0.25
local OFFER_SPACING     = 0.12

-- Nav button (bottom-right corner)
local NAV_BTN_X = 0.50
local NAV_BTN_Y = -0.35
local NAV_BTN_W = 0.28
local NAV_BTN_H = 0.10

local BG_SCALE = 5.0

-- ============================================================================
-- HELPERS
-- ============================================================================

local function loadSkillDefs()
    local data = LoadJSON("assets/JSON/Skills.json")
    if data and data.skills then
        skillDefs = data.skills
        allSkillIDs = {}
        for id, _ in pairs(skillDefs) do
            table.insert(allSkillIDs, id)
        end
        table.sort(allSkillIDs)
    else
        skillDefs = {}
        allSkillIDs = {}
    end
end

local function loadExistingLoadout()
    local data = LoadJSON("assets/JSON/SkillLoadout.json")
    if data and data.players then
        for i = 1, 3 do
            local p = data.players[tostring(i)]
            if p then
                loadout[i] = {}
                for slot = 1, 4 do
                    loadout[i][tostring(slot)] = p[tostring(slot)] or nil
                end
            end
        end
        Log("[SkillSwapUI] Loaded existing loadout")
    else
        loadout[1] = { ["1"] = "Thrust",       ["2"] = "Guard" }
        loadout[2] = { ["1"] = "Fireball",     ["2"] = "LightningStrike" }
        loadout[3] = { ["1"] = "Slam",         ["2"] = "SiphonCharge" }
        Log("[SkillSwapUI] Using default loadout")
    end
end

local function getSkillName(skillID)
    if not skillID then return "- Empty -" end
    if skillDefs[skillID] and skillDefs[skillID].name then
        return skillDefs[skillID].name
    end
    return skillID
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

local function GetEstimatedTextWidthPx(text, textScale, scaleRef)
    local s = tostring(text or "")
    local avgGlyphWidthPx = 24 * textScale * scaleRef
    return #s * avgGlyphWidthPx
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

local function EnsureHoverTooltipSprite()
    if hoverTooltipID and hoverTooltipID > 0 then
        return
    end

    local camX, camY = GetCameraPosition()
    hoverTooltipID = SpawnSprite(
        HOVER_TOOLTIP_TEXTURE,
        camX + OFFER_X + HOVER_TOOLTIP_OFFSET_X,
        camY + CURRENT_START_Y,
        HOVER_TOOLTIP_SCALE_X,
        HOVER_TOOLTIP_SCALE_Y,
        HOVER_TOOLTIP_LAYER
    )

    if hoverTooltipID and hoverTooltipID > 0 and SetSpriteVisibility then
        SetSpriteVisibility(hoverTooltipID, false)
    end
end

local function UpdateHoverTooltip()
    hoveredSkillID = nil
    hoveredSkillName = nil
    hoveredSkillDescription = nil
    hoveredSkillCost = 0

    local pi = currentChar
    if not pi then
        return
    end

    local camX, camY = GetCameraPosition()
    local entries = {}

    local numExisting = tonumber(targetSlot) - 1
    for si = 1, numExisting do
        local skillID = loadout[pi][tostring(si)]
        if skillID then
            table.insert(entries, {
                skillID = skillID,
                x = camX + CURRENT_X,
                y = camY + CURRENT_START_Y - (si - 1) * CURRENT_SPACING,
                sx = SLOT_W,
                sy = SLOT_H,
            })
        end
    end

    for oi = 1, NUM_OFFERS do
        local skillID = skillOffers[pi][oi]
        if skillID then
            table.insert(entries, {
                skillID = skillID,
                x = camX + OFFER_X,
                y = camY + CURRENT_START_Y - (oi - 1) * CURRENT_SPACING,
                sx = SLOT_W,
                sy = SLOT_H,
            })
        end
    end

    local mouseWX, mouseWY = nil, nil
    if GetMouseWorldPosition then
        mouseWX, mouseWY = GetMouseWorldPosition()
    end
    local mouseFX, mouseFY = nil, nil
    if (not mouseWX or not mouseWY) then
        mouseFX, mouseFY = GetMouseFramebufferPosition()
    end

    local hovered = nil
    for _, entry in ipairs(entries) do
        local halfW = (entry.sx or SLOT_W) * 0.5
        local halfH = (entry.sy or SLOT_H) * 0.5

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

        if WorldToScreen and mouseFX and mouseFY then
            local leftScreen, bottomScreen = WorldToScreen(entry.x - halfW, entry.y - halfH, true)
            local rightScreen, topScreen = WorldToScreen(entry.x + halfW, entry.y + halfH, true)
            if leftScreen and bottomScreen and rightScreen and topScreen then
                local minX = math.min(leftScreen, rightScreen)
                local maxX = math.max(leftScreen, rightScreen)
                local minY = math.min(bottomScreen, topScreen)
                local maxY = math.max(bottomScreen, topScreen)
                if mouseFX >= minX and mouseFX <= maxX and mouseFY >= minY and mouseFY <= maxY then
                    hovered = entry
                    break
                end
            end
        end
    end

    local show = hovered ~= nil
    if show then
        local sid = hovered.skillID
        local data = skillDefs[sid] or {}
        hoveredSkillID = sid
        hoveredSkillName = data.name or sid
        hoveredSkillDescription = data.description or "No description available."
        hoveredSkillCost = math.floor((tonumber(data.apCost) or 0) + 0.5)
        hoverTooltipX = hovered.x + HOVER_TOOLTIP_OFFSET_X
        hoverTooltipY = hovered.y
        if hoverTooltipID and hoverTooltipID > 0 then
            SetSpritePosition(hoverTooltipID, hoverTooltipX, hoverTooltipY)
        end
    end

    if hoverTooltipID and hoverTooltipID > 0 and SetSpriteVisibility and hoverTooltipVisible ~= show then
        SetSpriteVisibility(hoverTooltipID, show)
    end
    hoverTooltipVisible = show
end

local function DrawHoverTooltip()
    if not hoverTooltipVisible or not hoveredSkillName or not DrawText or not WorldToScreen then
        return
    end

    local fbW, fbH = GetFramebufferSize()
    if not fbW or fbW <= 0 then
        return
    end

    local halfW = HOVER_TOOLTIP_SCALE_X * 0.5
    local halfH = HOVER_TOOLTIP_SCALE_Y * 0.5
    local leftScreen, bottomScreen = WorldToScreen(hoverTooltipX - halfW, hoverTooltipY - halfH, true)
    local rightScreen, topScreen = WorldToScreen(hoverTooltipX + halfW, hoverTooltipY + halfH, true)
    if not leftScreen or not rightScreen or not bottomScreen or not topScreen then
        return
    end

    local minX = math.min(leftScreen, rightScreen)
    local maxX = math.max(leftScreen, rightScreen)
    local topY = math.max(bottomScreen, topScreen)

    local scaleRef = fbW / 1920
    local wrappedDescription = WrapText(hoveredSkillDescription, HOVER_WRAP_CHARS)
    local titleX = minX + (HOVER_PADDING_X_PX * scaleRef)
    local titleY = topY - (HOVER_HEADER_TOP_INSET_PX * scaleRef)
    local costText = string.format("AP %d", math.floor((tonumber(hoveredSkillCost) or 0) + 0.5))
    local costWidthPx = GetEstimatedTextWidthPx(costText, HOVER_COST_SCALE, scaleRef)
    local costX = maxX - (HOVER_PADDING_X_PX * scaleRef) - costWidthPx
    local costY = titleY

    DrawText(
        HOVER_FONT,
        hoveredSkillName,
        titleX,
        titleY,
        HOVER_TITLE_SCALE * scaleRef,
        HOVER_TEXT_COLOR.r,
        HOVER_TEXT_COLOR.g,
        HOVER_TEXT_COLOR.b
    )

    DrawText(
        HOVER_FONT,
        costText,
        costX,
        costY,
        HOVER_COST_SCALE * scaleRef,
        HOVER_TEXT_COLOR.r,
        HOVER_TEXT_COLOR.g,
        HOVER_TEXT_COLOR.b
    )

    local lineIndex = 0
    local descYStart = topY - (HOVER_DESC_START_INSET_PX * scaleRef)
    for line in wrappedDescription:gmatch("[^\n]+") do
        DrawText(
            HOVER_FONT,
            line,
            titleX,
            descYStart - (lineIndex * HOVER_DESC_LINE_SPACING_PX * scaleRef),
            HOVER_BODY_SCALE * scaleRef,
            HOVER_TEXT_COLOR.r,
            HOVER_TEXT_COLOR.g,
            HOVER_TEXT_COLOR.b
        )
        lineIndex = lineIndex + 1
    end
end

local function saveLoadout()
    print("[SkillSwapUI] saveLoadout() called")
    local f = io.open("assets/JSON/SkillLoadout.json", "w")
    if not f then
        print("[SkillSwapUI] ERROR: Cannot open assets/JSON/SkillLoadout.json for writing!")
        return
    end
    f:write("{\n  \"players\": {\n")
    for i = 1, 3 do
        f:write("    \"" .. i .. "\": {\n")
        local entries = {}
        for slot = 1, 4 do
            local sid = loadout[i][tostring(slot)]
            if sid then
                table.insert(entries, "      \"" .. slot .. "\": \"" .. sid .. "\"")
            end
        end
        f:write(table.concat(entries, ",\n") .. "\n")
        if i < 3 then
            f:write("    },\n")
        else
            f:write("    }\n")
        end
    end
    f:write("  }\n}\n")
    f:close()
    for i = 1, 3 do
        local skills = {}
        for slot = 1, 4 do
            local sid = loadout[i][tostring(slot)]
            if sid then table.insert(skills, slot .. "=" .. sid) end
        end
        print("[SkillSwapUI] Saved " .. charNames[i] .. ": " .. table.concat(skills, ", "))
    end
end

-- Character-specific skill pools (matches Skills.json order)
local charSkillPools = {
    -- Warrior (first 8)
    { "Thrust", "SweepingSlash", "Guard", "SwiftBlow", "KnightsOath", "Parry", "ExploitWeakness", "Bash" },
    -- Mage (next 8)
    { "Fireball", "PiercingShot", "LightningStrike", "EarthenBind", "ManaDrain", "Overload", "SoulRend", "SoulMerge" },
    -- CHANGED: Berserker (last 8)
    { "Slam", "SiphonCharge", "FutileResistance", "DarkOmens", "Cannibalism", "Groundshatter", "BloodyWarcry", "BladedWhirlwind" },
}

local NUM_OFFERS = 2

local function generateOffers()
    math.randomseed(os.time())

    for pi = 1, 3 do
        local existing = {}
        for slot = 1, 4 do
            local sid = loadout[pi][tostring(slot)]
            if sid then existing[sid] = true end
        end

        local pool = {}
        for _, sid in ipairs(charSkillPools[pi]) do
            if not existing[sid] then
                table.insert(pool, sid)
            end
        end

        for i = #pool, 2, -1 do
            local j = math.random(1, i)
            pool[i], pool[j] = pool[j], pool[i]
        end

        skillOffers[pi] = {}
        for i = 1, math.min(NUM_OFFERS, #pool) do
            table.insert(skillOffers[pi], pool[i])
        end

        Log("[SkillSwapUI] " .. charNames[pi] .. " offers: " ..
            table.concat(skillOffers[pi], ", "))
    end

    chosenSkill = { nil, nil, nil }
end

-- ============================================================================
-- BUILD / DESTROY
-- ============================================================================

local existingBtnMap = {}
local offerBtnMap = {}
local navBtnID = 0

local targetSlot = "3"

local function destroyCharPage()
    if charScrollID and charScrollID > 0 then
        DestroyEntity(charScrollID)
        charScrollID = 0
    end
    if charSpriteID and charSpriteID > 0 then
        DestroyEntity(charSpriteID)
        charSpriteID = 0
    end

    ClearAllButtons()
    buttonIDs = {}
    existingBtnMap = {}
    offerBtnMap = {}
    navBtnID = 0

    for si = 1, 4 do _G["OnExisting_" .. si] = nil end
    for oi = 1, NUM_OFFERS do _G["OnOffer_" .. oi] = nil end
    _G["OnNavBtn"] = nil
end

local function buildCharPage()
    local pi = currentChar
    -- Always use the camera position that was captured when Show() was called.
    -- Calling GetCameraPosition() again here can return a slightly different value
    -- if even one frame has passed, causing sprites to drift away from the background.
    local camX, camY = lockedCamX, lockedCamY

    existingBtnMap = {}
    offerBtnMap = {}
    buttonIDs = {}

    -- Scroll behind character (rotated 90 degrees)
    charScrollID = SpawnSprite(
        SCROLL_TEXTURE,
        camX + SPRITE_X, camY + SPRITE_Y,
        0.60, 0.80,
        CHAR_SCROLL_LAYER
    )
    if charScrollID and charScrollID > 0 then
        SetEntityRotation(charScrollID, 1.9199)
    end

    -- Animated character sprite (left, big)
    local anim = charAnims[pi]
    charSpriteID = SpawnAnimatedSprite(
        anim.sprite,
        camX + SPRITE_X, camY + SPRITE_Y,
        SPRITE_SCALE, SPRITE_SCALE,
        SPR_LAYER,
        anim.rows, anim.columns,
        anim.frameCount, anim.frameTime,
        anim.loop
    )

    -- CHANGED: Current skills (center column)
    local numExisting = tonumber(targetSlot) - 1
    for si = 1, numExisting do
        local slotY = camY + CURRENT_START_Y - (si - 1) * CURRENT_SPACING
        local cbName = "OnExisting_" .. si
        _G[cbName] = function() end

        local btnID = CreateButton(
            SCROLL_TEXTURE,
            camX + CURRENT_X, slotY,
            SLOT_W, SLOT_H,
            cbName,
            BTN_LAYER
        )
        existingBtnMap[si] = btnID
        table.insert(buttonIDs, btnID)
    end

    -- CHANGED: Offer skills (right column, aligned with current skills row by row)
    -- Offers start at the same Y as current skills so they sit side by side
    for oi = 1, NUM_OFFERS do
        local offerY = camY + CURRENT_START_Y - (oi - 1) * CURRENT_SPACING
        local cbName = "OnOffer_" .. oi
        _G[cbName] = function()
            print("[SkillSwapUI] CLICK: " .. cbName .. " fired for " .. charNames[pi])
            local offeredSkill = skillOffers[pi][oi]
            if not offeredSkill then
                print("[SkillSwapUI] ERROR: No offered skill for " .. charNames[pi] .. " offer " .. oi)
                return
            end
            chosenSkill[pi] = offeredSkill
            loadout[pi][targetSlot] = offeredSkill
            print("[SkillSwapUI] " .. charNames[pi] .. " chose: " .. getSkillName(offeredSkill) .. " -> slot " .. targetSlot)
        end
        print("[SkillSwapUI] Registered callback: " .. cbName .. " for " .. charNames[pi])

        local btnID = CreateButton(
            SCROLL_TEXTURE,
            camX + OFFER_X, offerY,    -- CHANGED: uses OFFER_X (right column)
            SLOT_W, SLOT_H,
            cbName,
            BTN_LAYER
        )
        offerBtnMap[oi] = btnID
        table.insert(buttonIDs, btnID)
    end

    -- CHANGED: Nav button (bottom-right)
    _G["OnNavBtn"] = function()
        if not chosenSkill[pi] then
            Log("[SkillSwapUI] Must choose a skill first!")
            return
        end
        if currentChar < 3 then
            destroyCharPage()
            currentChar = currentChar + 1
            buildCharPage()
            Log("[SkillSwapUI] Moving to " .. charNames[currentChar])
        else
            Log("[SkillSwapUI] All done - saving")
            saveLoadout()
            SkillSwapUI.Hide()
        end
    end

    navBtnID = CreateButton(
        SCROLL_TEXTURE,
        camX + NAV_BTN_X, camY + NAV_BTN_Y,   -- CHANGED: bottom-right position
        NAV_BTN_W, NAV_BTN_H,
        "OnNavBtn",
        BTN_LAYER
    )
    table.insert(buttonIDs, navBtnID)
end

-- ============================================================================
-- PUBLIC API
-- ============================================================================

function SkillSwapUI.IsActive()
    return active
end

function SkillSwapUI.Show(doneCallback, slot)
    if active then return end
    active = true
    onDone = doneCallback
    targetSlot = tostring(slot or 3)
    currentChar = 1

    loadSkillDefs()
    loadExistingLoadout()
    generateOffers()

    TogglePause()

    -- Lock the camera origin once; all pages (Warrior → Mage → Berserker) share it.
    lockedCamX, lockedCamY = GetCameraPosition()

    bgSpriteID = SpawnSprite(
        "assets/Menu/WoodBackground.png",
        lockedCamX, lockedCamY,
        BG_SCALE, BG_SCALE,
        BG_LAYER
    )

    EnsureHoverTooltipSprite()

    buildCharPage()
    Log("[SkillSwapUI] Opened - starting with " .. charNames[1])
end

function SkillSwapUI.Hide()
    if not active then return end
    active = false
    destroyCharPage()

    if bgSpriteID > 0 then
        DestroyEntity(bgSpriteID)
        bgSpriteID = 0
    end

    if hoverTooltipID and hoverTooltipID > 0 then
        DestroyEntity(hoverTooltipID)
        hoverTooltipID = 0
    end
    hoverTooltipVisible = false
    hoveredSkillID = nil
    hoveredSkillName = nil
    hoveredSkillDescription = nil
    hoveredSkillCost = 0
    hoverTooltipX = 0.0
    hoverTooltipY = 0.0

    lockedCamX, lockedCamY = 0, 0

    TogglePause()

    if onDone then
        onDone()
        onDone = nil
    end
    Log("[SkillSwapUI] Closed")
end

function SkillSwapUI.Update(dt)
    if not active then return end
    UpdateHoverTooltip()
end

function SkillSwapUI.Draw()
    if not active then return end

    local fbW, fbH = GetFramebufferSize()
    if not fbW or fbW == 0 then return end

    local scaleRef = fbW / 1920
    local pi = currentChar
    local cr, cg, cb = charColors[pi][1], charColors[pi][2], charColors[pi][3]

    -- ========================================
    -- TITLE
    -- ========================================
    local titleText = "SKILL SELECTION"
    local titleW = string.len(titleText) * 18 * scaleRef
    local titleX = (fbW - titleW) * 0.5
    local titleY = fbH - 35 * scaleRef
    DrawText("Jersey20Regular", titleText, titleX, titleY, 1.0 * scaleRef, 1, 1, 1)

    -- Subtitle
    local subText = "Choose a new skill for " .. charNames[pi]
    local subW = string.len(subText) * 10 * scaleRef
    local subX = (fbW - subW) * 0.5
    local subY = fbH - 75 * scaleRef
    DrawText("Jersey20Regular", subText, subX, subY, 0.6 * scaleRef, cr, cg, cb)

    -- Counter
    local counterText = currentChar .. " / 3"
    DrawText("Jersey20Regular", counterText,
        fbW - 100 * scaleRef, fbH - 35 * scaleRef,
        0.5 * scaleRef, 0.6, 0.6, 0.6)

    -- ========================================
    -- LEFT: Character name
    -- ========================================
    local nameX = fbW * 0.08
    local nameY = fbH - 200 * scaleRef
    DrawText("Jersey20Regular", charNames[pi],
        nameX, nameY,
        1.4 * scaleRef, cr, cg, cb)

    -- ========================================
    -- Center column label - "Current Skills:"
    -- ========================================
    local currentScreenX = fbW * 0.35
    local existLabelY = fbH - 145 * scaleRef
    DrawText("Jersey20Regular", "Current Skills:",
        currentScreenX - 50 * scaleRef, existLabelY,
        0.7 * scaleRef, 0.8, 0.8, 0.8)

    -- Existing skill text (centered on button)
    local numExisting = tonumber(targetSlot) - 1
    for si = 1, numExisting do
        local skillID = loadout[pi][tostring(si)]
        local skillName = getSkillName(skillID)

        if existingBtnMap[si] then
            local textOffsetX = -string.len(skillName) * 3.5 * scaleRef
            DrawButtonText(
                existingBtnMap[si],
                "Jersey20Regular",
                skillName,
                textOffsetX, -5 * scaleRef,
                0.35 * scaleRef,
                0.2, 0.15, 0.1
            )
        end
    end

    -- ========================================
    -- Right column label - "Pick a new skill:"
    -- ========================================
    local offerScreenX = fbW * 0.78
    local offerLabelY = fbH - 145 * scaleRef
    DrawText("Jersey20Regular", "Pick a new skill:",
        offerScreenX - 65 * scaleRef, offerLabelY,
        0.7 * scaleRef, 1.0, 0.9, 0.5)

    -- Offer skill text (centered on button)
    for oi = 1, NUM_OFFERS do
        local offeredSkill = skillOffers[pi][oi]
        local skillName = getSkillName(offeredSkill)

        local sr, sg, sb = 0.2, 0.15, 0.1
        if chosenSkill[pi] and chosenSkill[pi] == offeredSkill then
            sr, sg, sb = 0.1, 0.5, 0.1
        end

        if offerBtnMap[oi] then
            local textOffsetX = -string.len(skillName) * 3.5 * scaleRef
            DrawButtonText(
                offerBtnMap[oi],
                "Jersey20Regular",
                skillName,
                textOffsetX, -5 * scaleRef,
                0.35 * scaleRef,
                sr, sg, sb
            )
        end
    end

    -- ========================================
    -- CHANGED: NAV BUTTON TEXT (bottom-right)
    -- ========================================
    if navBtnID and navBtnID ~= 0 then
        local navLabel = "SELECT A SKILL"
        local nr, ng, nb = 0.4, 0.3, 0.2

        if chosenSkill[pi] then
            if currentChar < 3 then
                navLabel = "NEXT"
            else
                navLabel = "FINISH"
            end
            nr, ng, nb = 0.1, 0.5, 0.1
        end

        local textOffsetX = -string.len(navLabel) * 3.5 * scaleRef
        DrawButtonText(
            navBtnID,
            "Jersey20Regular",
            navLabel,
            textOffsetX, -6 * scaleRef,
            0.40 * scaleRef,
            nr, ng, nb
        )
    end

    DrawHoverTooltip()
end

return SkillSwapUI