--[[
===============================================================================
 File:          SkillSwapUI.lua
 Description:   Between-level skill selection, one character at a time.

 Flow:
    Warrior -> pick skill -> NEXT -> Mage -> pick -> NEXT -> Rogue -> FINISH

 Layout:
    Left:  Large animated character sprite + name
    Right: Current skills + 3 offers + nav button (scroll overlay buttons)

 Usage:
    local SkillSwapUI = require("SkillSwapUI")
    _G.SkillSwapUI = SkillSwapUI
    SkillSwapUI.Show(onDoneCallback)
    SkillSwapUI.Update(dt)
    SkillSwapUI.Draw()
===============================================================================
]]--

local SkillSwapUI = {}

-- ============================================================================
-- STATE
-- ============================================================================

local active        = false
local onDone        = nil
local currentChar   = 1

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

local charNames = { "Warrior", "Mage", "Rogue" }
local charColors = {
    { 1.0, 0.7, 0.3 },
    { 0.4, 0.6, 1.0 },
    { 0.3, 1.0, 0.5 },
}

local charAnims = {
    [1] = {
        sprite = "assets/Warrior/FrontView/WarriorTopDownView.png",
        rows = 1, columns = 12, frameCount = 12,
        frameTime = 0.55, loop = true
    },
    [2] = {
        sprite = "assets/Warrior/FrontView/WarriorTopDownView.png",
        rows = 1, columns = 12, frameCount = 12,
        frameTime = 0.55, loop = true
    },
    [3] = {
        sprite = "assets/Warrior/FrontView/WarriorTopDownView.png",
        rows = 1, columns = 12, frameCount = 12,
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
local CHAR_SCROLL_LAYER = 51  -- Between BG and character sprite

-- Right side
local RIGHT_X           = 0.22

-- Existing skills
local EXISTING_START_Y  = 0.25
local EXISTING_SPACING  = 0.14
local SLOT_W            = 0.42
local SLOT_H            = 0.10

-- Offer skills
local OFFER_START_Y     = -0.18
local OFFER_SPACING     = 0.14

-- Nav button
local NAV_BTN_Y = -0.45
local NAV_BTN_W = 0.42
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
        loadout[2] = { ["1"] = "Fireball",     ["2"] = "PiercingShot" }
        loadout[3] = { ["1"] = "SwiftBlow" }
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

local function saveLoadout()
    local f = io.open("assets/JSON/SkillLoadout.json", "w")
    if not f then
        Log("[SkillSwapUI] ERROR: Cannot write SkillLoadout.json")
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
    Log("[SkillSwapUI] Saved loadout")
end

-- Character-specific skill pools (matches Skills.json order)
local charSkillPools = {
    -- Warrior (first 8)
    { "Thrust", "SweepingSlash", "Guard", "SwiftBlow", "KnightsOath", "Parry", "ExploitWeakness", "Bash" },
    -- Mage (next 8)
    { "Fireball", "PiercingShot", "LightningStrike", "EarthenBind", "ManaDrain", "Overload", "SoulRend", "SoulMerge" },
    -- Rogue (last 8)
    { "Slam", "SiphonCharge", "FutileResistance", "DarkOmens", "Cannibalism", "Groundshatter", "BloodyWarcry", "BladedWhirlwind" },
}

local NUM_OFFERS = 2

local function generateOffers()
    math.randomseed(os.time())

    for pi = 1, 3 do
        -- Collect skills already in this character's loadout
        local existing = {}
        for slot = 1, 4 do
            local sid = loadout[pi][tostring(slot)]
            if sid then existing[sid] = true end
        end

        -- Filter to character-specific pool, excluding already owned skills
        local pool = {}
        for _, sid in ipairs(charSkillPools[pi]) do
            if not existing[sid] then
                table.insert(pool, sid)
            end
        end

        -- Shuffle
        for i = #pool, 2, -1 do
            local j = math.random(1, i)
            pool[i], pool[j] = pool[j], pool[i]
        end

        -- Pick offers
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

local function destroyCharPage()
    ClearAllButtons()
    buttonIDs = {}
    existingBtnMap = {}
    offerBtnMap = {}
    navBtnID = 0

    if charScrollID and charScrollID > 0 then
        DestroyEntity(charScrollID)
        charScrollID = 0
    end
    if charSpriteID and charSpriteID > 0 then
        DestroyEntity(charSpriteID)
        charSpriteID = 0
    end

    for si = 1, 2 do _G["OnExisting_" .. si] = nil end
    for oi = 1, NUM_OFFERS do _G["OnOffer_" .. oi] = nil end
    _G["OnNavBtn"] = nil
end

local function buildCharPage()
    local pi = currentChar
    local camX, camY = GetCameraPosition()

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
        SetEntityRotation(charScrollID, 1.9199)  -- ~110 degrees
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

    -- Existing skills (scroll buttons, read-only)
    for si = 1, 2 do
        local slotY = camY + EXISTING_START_Y - (si - 1) * EXISTING_SPACING
        local cbName = "OnExisting_" .. si
        _G[cbName] = function() end

        local btnID = CreateButton(
            SCROLL_TEXTURE,
            camX + RIGHT_X, slotY,
            SLOT_W, SLOT_H,
            cbName,
            BTN_LAYER
        )
        existingBtnMap[si] = btnID
        table.insert(buttonIDs, btnID)
    end

    -- Offer skills (scroll buttons, clickable)
    for oi = 1, NUM_OFFERS do
        local offerY = camY + OFFER_START_Y - (oi - 1) * OFFER_SPACING
        local cbName = "OnOffer_" .. oi
        _G[cbName] = function()
            local offeredSkill = skillOffers[pi][oi]
            if not offeredSkill then return end
            chosenSkill[pi] = offeredSkill
            loadout[pi][targetSlot] = offeredSkill
            Log("[SkillSwapUI] " .. charNames[pi] .. " chose: " .. getSkillName(offeredSkill))
        end

        local btnID = CreateButton(
            SCROLL_TEXTURE,
            camX + RIGHT_X, offerY,
            SLOT_W, SLOT_H,
            cbName,
            BTN_LAYER
        )
        offerBtnMap[oi] = btnID
        table.insert(buttonIDs, btnID)
    end

    -- Nav button
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
        camX + RIGHT_X, camY + NAV_BTN_Y,
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

-- Which loadout slot to fill (set by caller, defaults to 3)
local targetSlot = "3"

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

    local camX, camY = GetCameraPosition()
    bgSpriteID = SpawnSprite(
        "assets/Menu/WoodBackground.png",
        camX, camY,
        BG_SCALE, BG_SCALE,
        BG_LAYER
    )

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

    TogglePause()

    if onDone then
        onDone()
        onDone = nil
    end
    Log("[SkillSwapUI] Closed")
end

function SkillSwapUI.Update(dt)
    if not active then return end
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
    -- RIGHT: Current Skills label
    -- ========================================
    local rightScreenX = fbW * 0.55
    local existLabelY = fbH - 110 * scaleRef
    DrawText("Jersey20Regular", "Current Skills:",
        rightScreenX - 75 * scaleRef, existLabelY,
        0.7 * scaleRef, 0.8, 0.8, 0.8)

    -- Existing skill text
    for si = 1, 2 do
        local skillID = loadout[pi][tostring(si)]
        local skillName = getSkillName(skillID)

        if existingBtnMap[si] then
            DrawButtonText(
                existingBtnMap[si],
                "Jersey20Regular",
                skillName,
                -35 * scaleRef, -5 * scaleRef,
                0.42 * scaleRef,
                0.2, 0.15, 0.1
            )
        end
    end

    -- ========================================
    -- RIGHT: Pick one label
    -- ========================================
    local offerLabelY = fbH - 500 * scaleRef
    DrawText("Jersey20Regular", "Pick a new skill:",
        rightScreenX - 90 * scaleRef, offerLabelY,
        0.7 * scaleRef, 1.0, 0.9, 0.5)

    -- Offer skill text
    for oi = 1, NUM_OFFERS do
        local offeredSkill = skillOffers[pi][oi]
        local skillName = getSkillName(offeredSkill)

        local sr, sg, sb = 0.2, 0.15, 0.1
        if chosenSkill[pi] and chosenSkill[pi] == offeredSkill then
            sr, sg, sb = 0.1, 0.5, 0.1
        end

        if offerBtnMap[oi] then
            DrawButtonText(
                offerBtnMap[oi],
                "Jersey20Regular",
                skillName,
                -35 * scaleRef, -5 * scaleRef,
                0.42 * scaleRef,
                sr, sg, sb
            )
        end
    end

    -- ========================================
    -- NAV BUTTON TEXT
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

        DrawButtonText(
            navBtnID,
            "Jersey20Regular",
            navLabel,
            -40 * scaleRef, -6 * scaleRef,
            0.5 * scaleRef,
            nr, ng, nb
        )
    end
end

return SkillSwapUI