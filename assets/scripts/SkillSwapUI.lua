--[[
===============================================================================
 File:          SkillSwapUI.lua
 Description:   Between-level skill selection overlay.
                Shows after clearing a level, lets players assign skills
                to each character's 4 slots from the full skill pool.

 Usage (from level script):
    local SkillSwapUI = require("SkillSwapUI")
    SkillSwapUI.Show(onDoneCallback)   -- pause + create UI
    SkillSwapUI.Update(dt)             -- in OnUpdate
    SkillSwapUI.Draw()                 -- in OnDraw

 Persistence:
    Writes assets/JSON/SkillLoadout.json via io.open.
    PlayerScript.lua reads it on init to populate PlayerSkills.
===============================================================================
]]--

local SkillSwapUI = {}

-- ============================================================================
-- STATE
-- ============================================================================

local active       = false
local onDone       = nil       -- callback when player clicks Continue

local bgSpriteID   = 0
local buttonIDs    = {}        -- all spawned button IDs for cleanup
local selectedSlot = nil       -- { player = 1..3, slot = 1..4 } or nil

-- Current loadout: playerIndex -> { [slotKey] = skillID }
local loadout = {
    [1] = {},
    [2] = {},
    [3] = {},
}

-- All skill IDs available in the pool
local allSkillIDs = {}
local skillDefs   = {}         -- loaded from Skills.json

-- Character display names
local charNames = { "Player 1", "Player 2", "Player 3" }

-- Layout constants (world coords, camera at 0,0)
local SLOT_W      = 0.35
local SLOT_H      = 0.10
local POOL_W      = 0.35
local POOL_H      = 0.10
local CHAR_START_Y = 0.52
local CHAR_SPACING = 0.22
local SLOT_START_X = -0.55
local SLOT_SPACING = 0.38
local POOL_START_Y = -0.18
local POOL_ROW_H   = 0.14
local POOL_COLS    = 5
local POOL_START_X = -0.72
local POOL_COL_W   = 0.36
local BTN_LAYER    = 15
local BG_LAYER     = 12

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
        Log("[SkillSwapUI] Loaded existing loadout from SkillLoadout.json")
    else
        -- Initialise with sensible defaults
        loadout[1] = { ["1"] = "Thrust",       ["2"] = "Guard" }
        loadout[2] = { ["1"] = "Fireball",     ["2"] = "PiercingShot" }
        loadout[3] = { ["1"] = "SwiftBlow" }
        Log("[SkillSwapUI] No existing loadout, using defaults")
    end
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
    Log("[SkillSwapUI] Saved loadout to SkillLoadout.json")
end

local function spawnBtn(texture, x, y, w, h, cb)
    local id = CreateButton(texture, x, y, w, h, cb, BTN_LAYER)
    table.insert(buttonIDs, id)
    return id
end

-- ============================================================================
-- BUILD / DESTROY UI
-- ============================================================================

local slotBtnMap = {}   -- [playerIdx][slotIdx] = buttonID
local poolBtnMap = {}   -- [skillIdx]           = buttonID

local function buildUI()
    local camX, camY = GetCameraPosition()

    -- Background
    bgSpriteID = SpawnSprite(
        "assets/Menu/WoodBackground.png",
        camX, camY,
        4.0, 4.0,
        BG_LAYER
    )

    -- Register global callbacks for slots
    slotBtnMap = {}
    for pi = 1, 3 do
        slotBtnMap[pi] = {}
        for si = 1, 4 do
            local cbName = "OnSkillSlot_" .. pi .. "_" .. si
            _G[cbName] = function()
                if selectedSlot and selectedSlot.player == pi and selectedSlot.slot == si then
                    -- Deselect
                    selectedSlot = nil
                    Log("[SkillSwapUI] Deselected slot")
                else
                    selectedSlot = { player = pi, slot = si }
                    Log("[SkillSwapUI] Selected slot P" .. pi .. " S" .. si)
                end
            end
            local bx = camX + SLOT_START_X + (si - 1) * SLOT_SPACING
            local by = camY + CHAR_START_Y - (pi - 1) * CHAR_SPACING
            slotBtnMap[pi][si] = spawnBtn("assets/Menu/Ui_btn.png", bx, by, SLOT_W, SLOT_H, cbName)
        end
    end

    -- Register global callbacks for pool skills
    poolBtnMap = {}
    for idx, skillID in ipairs(allSkillIDs) do
        local cbName = "OnPoolSkill_" .. idx
        _G[cbName] = function()
            if selectedSlot then
                -- Assign this skill to the selected slot
                loadout[selectedSlot.player][tostring(selectedSlot.slot)] = skillID
                Log("[SkillSwapUI] Assigned " .. skillID .. " to P" .. selectedSlot.player .. " S" .. selectedSlot.slot)
                selectedSlot = nil
            else
                Log("[SkillSwapUI] No slot selected - click a slot first")
            end
        end
        local row = math.floor((idx - 1) / POOL_COLS)
        local col = (idx - 1) % POOL_COLS
        local bx = camX + POOL_START_X + col * POOL_COL_W
        local by = camY + POOL_START_Y - row * POOL_ROW_H
        poolBtnMap[idx] = spawnBtn("assets/Menu/Ui_btn.png", bx, by, POOL_W, POOL_H, cbName)
    end

    -- Clear slot button (removes skill from selected slot)
    _G["OnClearSlot"] = function()
        if selectedSlot then
            loadout[selectedSlot.player][tostring(selectedSlot.slot)] = nil
            Log("[SkillSwapUI] Cleared P" .. selectedSlot.player .. " S" .. selectedSlot.slot)
            selectedSlot = nil
        end
    end
    local clearY = camY + POOL_START_Y - math.ceil(#allSkillIDs / POOL_COLS) * POOL_ROW_H
    spawnBtn("assets/Menu/Ui_btn.png", camX - 0.4, clearY - 0.05, POOL_W, POOL_H, "OnClearSlot")

    -- Continue button
    _G["OnSkillSwapContinue"] = function()
        Log("[SkillSwapUI] Continue clicked")
        saveLoadout()
        SkillSwapUI.Hide()
    end
    spawnBtn("assets/Menu/Ui_btn.png", camX + 0.4, clearY - 0.05, 0.5, 0.14, "OnSkillSwapContinue")
end

local function destroyUI()
    ClearAllButtons()
    buttonIDs = {}
    slotBtnMap = {}
    poolBtnMap = {}
    if bgSpriteID > 0 then
        DestroyEntity(bgSpriteID)
        bgSpriteID = 0
    end
    -- Clean up global callbacks
    for pi = 1, 3 do
        for si = 1, 4 do
            _G["OnSkillSlot_" .. pi .. "_" .. si] = nil
        end
    end
    for idx = 1, #allSkillIDs do
        _G["OnPoolSkill_" .. idx] = nil
    end
    _G["OnClearSlot"] = nil
    _G["OnSkillSwapContinue"] = nil
end

-- ============================================================================
-- PUBLIC API
-- ============================================================================

function SkillSwapUI.IsActive()
    return active
end

function SkillSwapUI.Show(doneCallback)
    if active then return end
    active = true
    onDone = doneCallback
    selectedSlot = nil

    loadSkillDefs()
    loadExistingLoadout()

    TogglePause()
    buildUI()
    Log("[SkillSwapUI] Skill swap screen opened")
end

function SkillSwapUI.Hide()
    if not active then return end
    active = false
    destroyUI()
    TogglePause()  -- unpause

    if onDone then
        onDone()
        onDone = nil
    end
    Log("[SkillSwapUI] Skill swap screen closed")
end

function SkillSwapUI.Update(dt)
    if not active then return end
    -- Buttons are handled by the engine's UI system automatically
end

function SkillSwapUI.Draw()
    if not active then return end

    local fbW, fbH = GetFramebufferSize()
    if not fbW or fbW == 0 then return end

    local cx = fbW * 0.5
    local scaleRef = fbW / 1920  -- reference 1920 wide

    -- Title
    DrawText("Jersey20Regular", "SKILL SELECTION", cx - 120 * scaleRef, 50 * scaleRef, 1.2 * scaleRef, 1, 1, 1)

    -- Character labels + slot text
    for pi = 1, 3 do
        local labelY = 120 * scaleRef + (pi - 1) * 100 * scaleRef
        DrawText("Jersey20Regular", charNames[pi], 60 * scaleRef, labelY, 0.7 * scaleRef, 1, 0.9, 0.6)

        for si = 1, 4 do
            local sid = loadout[pi][tostring(si)]
            local label = sid and (skillDefs[sid] and skillDefs[sid].name or sid) or "---"

            -- Highlight selected slot
            local r, g, b = 1, 1, 1
            if selectedSlot and selectedSlot.player == pi and selectedSlot.slot == si then
                r, g, b = 1, 1, 0  -- yellow highlight
            end

            if slotBtnMap[pi] and slotBtnMap[pi][si] then
                DrawButtonText(slotBtnMap[pi][si], "Jersey20Regular", label,
                    -40 * scaleRef, -8 * scaleRef, 0.55 * scaleRef, r, g, b)
            end
        end
    end

    -- Pool title
    local poolLabelY = 120 * scaleRef + 3 * 100 * scaleRef
    DrawText("Jersey20Regular", "Available Skills (click slot first, then skill):",
        60 * scaleRef, poolLabelY, 0.6 * scaleRef, 0.8, 0.8, 1.0)

    -- Pool skill labels
    for idx, skillID in ipairs(allSkillIDs) do
        local name = skillDefs[skillID] and skillDefs[skillID].name or skillID
        if poolBtnMap[idx] then
            DrawButtonText(poolBtnMap[idx], "Jersey20Regular", name,
                -40 * scaleRef, -8 * scaleRef, 0.45 * scaleRef, 1, 1, 1)
        end
    end

    -- Clear slot button text
    local clearBtnIdx = #buttonIDs - 1  -- second to last button
    if buttonIDs[clearBtnIdx] then
        DrawButtonText(buttonIDs[clearBtnIdx], "Jersey20Regular", "CLEAR SLOT",
            -40 * scaleRef, -8 * scaleRef, 0.55 * scaleRef, 1, 0.4, 0.4)
    end

    -- Continue button text
    local contBtnIdx = #buttonIDs  -- last button
    if buttonIDs[contBtnIdx] then
        DrawButtonText(buttonIDs[contBtnIdx], "Jersey20Regular", "CONTINUE",
            -50 * scaleRef, -10 * scaleRef, 0.7 * scaleRef, 0.2, 1, 0.2)
    end
end

return SkillSwapUI
