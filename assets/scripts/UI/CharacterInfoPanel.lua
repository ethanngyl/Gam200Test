--[[
===============================================================================
 File:           CharacterInfoPanel.lua
 Date:           2026-03-18
 ------------------------------------------------------------------------------
 CHARACTER INFO PANEL

 Purpose:
    Press I to open/close an overlay showing the active character's info:
    name, animated portrait, HP/AP crystals, 4 skill slots (name + icon +
    AP cost + description).

 IMPORTANT: This module must run in the LEVEL/GLOBAL Lua state (Level3Clean),
    NOT from per-entity scripts. DrawText only works during OnDraw(), and
    GetCameraPosition requires the global Lua state's graphicsSystem.

 Usage (from Level3Clean.lua):
    local CharacterInfoPanel = require("UI/CharacterInfoPanel")
    local charInfoPanel = CharacterInfoPanel.new()

    -- In ToggleCharInfoPanel(playerIndex, entityID):
    charInfoPanel:Toggle(playerIndex, entityID)

    -- In Level3Clean.OnDraw():
    if charInfoPanel.isOpen then charInfoPanel:OnDraw() end

    -- In CloseCharInfoPanel():
    charInfoPanel:_hide()

 Copyright (C) 2026 DigiPen Institute of Technology.
===============================================================================
]]--

local HealthUI = require("UI/HealthUI")

local CharacterInfoPanel = {}
CharacterInfoPanel.__index = CharacterInfoPanel

-- ============================================================================
-- CONSTANTS
-- ============================================================================

local CHARACTER_CONFIG = {
    [1] = {
        name    = "Knight",
        portrait = "assets/Warrior/FrontView/WarriorWalkingFront.png",
        animRows = 1, animCols = 8, animFrames = 8, animFrameTime = 0.12,
    },
    [2] = {
        name    = "Mage",
        portrait = "assets/Mage/FrontView/Mage_Walk_Front-Sheet.png",
        animRows = 1, animCols = 8, animFrames = 8, animFrameTime = 0.12,
    },
    [3] = {
        name    = "Rogue",
        portrait = "assets/Berserker/FrontView/Berserker_Walk_Front-Sheet.png",
        animRows = 1, animCols = 8, animFrames = 8, animFrameTime = 0.12,
    },
}

local SKILL_ICON_BASE = "assets/SkillIcons/"
local DEFAULT_ICON    = "assets/UI/skill_circle.png"
local BTN_BG          = "assets/Menu/Ui_btn.png"
local PANEL_BG        = "assets/new assets/skill_bubble_holder.png"
local FONT            = "Jersey20Regular"

-- World-space layout constants (offsets from panel center)
local PANEL_W      = 1.80
local PANEL_H      = 0.88
local PORTRAIT_W   = 0.26
local PORTRAIT_H   = 0.42
local ICON_SIZE    = 0.045
local BTN_W        = 0.15
local BTN_H        = 0.068
local PANEL_LAYER  = 6   -- Must be < 100 (far clip is z=1.0, layer*0.01 must stay < 1.0)

-- ============================================================================
-- SKILL ICON RESOLUTION
-- ============================================================================

local _iconCache = {}
local function ResolveSkillIcon(skillID)
    if not skillID or skillID == "" then return DEFAULT_ICON end
    if _iconCache[skillID] ~= nil then return _iconCache[skillID] end
    local path = SKILL_ICON_BASE .. skillID .. ".png"
    local f = io.open(path, "rb")
    if f then f:close(); _iconCache[skillID] = path; return path end
    _iconCache[skillID] = DEFAULT_ICON
    return DEFAULT_ICON
end

-- ============================================================================
-- SKILL DEFINITIONS (loaded once from JSON)
-- ============================================================================

local _skillDefs = nil
local function GetSkillDefs()
    if _skillDefs then return _skillDefs end
    _skillDefs = {}
    if LoadJSON then
        local raw = LoadJSON("assets/JSON/Skills.json")
        if raw and type(raw) == "table" then
            -- Skills.json format: { "skills": { "Thrust": { name, description, ... }, ... } }
            local skillsTable = raw.skills or raw
            for skillID, sk in pairs(skillsTable) do
                if type(sk) == "table" then
                    _skillDefs[skillID] = {
                        name        = sk.name or skillID,
                        description = sk.description or "",
                        apCost      = sk.attackAPCost or sk.apCost or 0,
                    }
                end
            end
        end
    end
    local count = 0; for _ in pairs(_skillDefs) do count = count + 1 end
    print("[CharInfoPanel] GetSkillDefs loaded " .. count .. " skills")
    return _skillDefs
end

-- ============================================================================
-- CONSTRUCTOR
-- ============================================================================

function CharacterInfoPanel.new()
    local self = setmetatable({}, CharacterInfoPanel)
    self.isOpen      = false
    self.spriteIDs   = {}
    self.offsets     = {}   -- {id, dx, dy} camera-relative offsets
    self.portraitID  = 0
    self.bgID        = 0
    self.skillBtnIDs = {}
    self.skillIconIDs= {}
    self._healthUI   = nil  -- HealthUI sub-instance for stat bars
    self.lastCamX    = nil
    self.lastCamY    = nil
    self.playerIndex = 1
    self.entityID    = 0
    return self
end

-- ============================================================================
-- TOGGLE
-- ============================================================================

function CharacterInfoPanel:Toggle(playerIndex, entityID)
    print("[CharInfoPanel] Toggle isOpen=" .. tostring(self.isOpen)
          .. " player=" .. tostring(playerIndex))
    if self.isOpen then
        self:_hide()
    else
        self:_show(playerIndex, entityID)
    end
end

-- ============================================================================
-- SHOW / HIDE
-- ============================================================================

function CharacterInfoPanel:_show(playerIndex, entityID)
    print("[CharInfoPanel] _show player=" .. tostring(playerIndex))
    self:_hide()  -- clear any previous

    self.isOpen      = true
    self.playerIndex = playerIndex or 1
    self.entityID    = entityID or 0
    self.spriteIDs   = {}
    self.offsets     = {}
    self.skillBtnIDs = {}
    self.skillIconIDs= {}

    -- Camera position (works in global Lua state)
    local cx, cy = 0, 0
    if GetCameraPosition then
        local x, y = GetCameraPosition()
        cx = x or 0
        cy = y or 0
    end
    self.lastCamX = cx
    self.lastCamY = cy
    print("[CharInfoPanel] camera=(" .. cx .. "," .. cy .. ")")

    -- Helper: spawn static sprite and record offset
    local function spawnS(tex, dx, dy, w, h, layer, rot)
        local id = SpawnSprite(tex, cx+dx, cy+dy, w, h, layer, rot or 0)
        if id and id > 0 then
            table.insert(self.spriteIDs, id)
            table.insert(self.offsets, {id=id, dx=dx, dy=dy})
        end
        return id
    end
    local function spawnA(tex, dx, dy, w, h, layer, rows, cols, frames, ft, loop)
        local id = SpawnAnimatedSprite(tex, cx+dx, cy+dy, w, h, layer, rows, cols, frames, ft, loop)
        if id and id > 0 then
            table.insert(self.spriteIDs, id)
            table.insert(self.offsets, {id=id, dx=dx, dy=dy})
        end
        return id
    end

    -- 1. Background (swap W/H so after 90° rotation visual = PANEL_W wide × PANEL_H tall)
    self.bgID = spawnS(PANEL_BG, 0, 0, PANEL_H, PANEL_W, PANEL_LAYER, 90)
    print("[CharInfoPanel] bgID=" .. tostring(self.bgID))

    -- 2. Portrait
    local charCfg = CHARACTER_CONFIG[playerIndex] or CHARACTER_CONFIG[1]
    local pdx = (-PANEL_W*0.5 + PORTRAIT_W*0.5 + 0.04) + 0.25
    local pdy = 0.04
    self.portraitID = spawnA(
        charCfg.portrait, pdx, pdy, PORTRAIT_W, PORTRAIT_H, PANEL_LAYER+1,
        charCfg.animRows, charCfg.animCols, charCfg.animFrames, charCfg.animFrameTime, true)

    -- 3. Stat bars (HealthUI composite: HP circle + Attack AP bar + Move AP bar)
    --    Positioned below portrait, left-aligned inside the panel.
    --    Uses same textures/config as UIManager's main HUD.
    local eid = entityID or 0
    self._healthUI = HealthUI:New()
    self._healthUI:Init({
        holderTexture      = "assets/UI/health_ap_movement_holder.png",
        holderFrameTexture = "assets/UI/health_ap_movement_holder frame only.png",
        holderScaleX       = 0.55,
        holderScaleY       = 0.23,
        holderOffsetX      = -0.55,
        holderOffsetY      = -0.28,
        holderFrameScaleX  = 0.55,
        holderFrameScaleY  = 0.23,
        holderFrameOffsetX = -0.533,
        holderFrameOffsetY = -0.263,
        hpFillOffsetX      = -0.181,
        hpFillOffsetY      = 0.05,
        hpFillWidth        = 0.085,
        hpFillHeight       = 0.085,
        hpFillColor        = { r = 0.9, g = 0.1, b = 0.1, a = 1.0 },
        hpFillTexture      = "assets/TileMap/Attack_Indicator.png",
        hpTextShowPercent  = false,
        hpTextOffsetY      = 0.0097,
        hpTextOffsetX      = 0.0025,
        attackFillOffsetX  = 0.0429,
        attackFillOffsetY  = 0.05,
        attackFillWidth    = 0.365,
        attackFillHeight   = 0.07,
        attackFillColor    = { r = 0.1, g = 0.25, b = 0.7, a = 1.0 },
        attackTextOffsetY  = 0.010,
        moveTextOffsetY    = 0.010,
        moveFillOffsetX    = 0.015,
        moveFillOffsetY    = -0.047,
        moveFillWidth      = 0.427,
        moveFillHeight     = 0.07,
        moveFillColor      = { r = 0.65, g = 0.35, b = 0.15, a = 1.0 },
        textColor          = { r = 0.0, g = 0.0, b = 0.0, a = 1.0 },
        textScale          = 0.80,
        getMovementAPFunc  = function(e) return GetEntityAP(e or eid) end,
        getAttackAPFunc    = function(e) return GetEntityAttackAP(e or eid) end,
        layer              = PANEL_LAYER + 1,
        textureBasePath    = "assets/UI/Health_",
    })
    -- Force tracking the specific panel entity (bypass active-character detection)
    self._healthUI.trackedCharID = eid

    -- 4. Skill slots
    local skills = self:_getPlayerSkills(playerIndex)
    local skillDx0   = -PANEL_W*0.5 + PORTRAIT_W + 0.35   -- tighter gap after portrait, shifted left
    local skillDyTop = PANEL_H*0.5 - 0.3
    local rowSpacing = PANEL_H * 0.1
    for slot = 1, 4 do
        local dy    = skillDyTop - (slot-1)*rowSpacing
        local skillID = skills[tostring(slot)]
        local btnID = spawnS(BTN_BG, skillDx0+BTN_W*0.5, dy, BTN_W, BTN_H, PANEL_LAYER+1)
        self.skillBtnIDs[slot] = btnID
        local iconID = spawnS(ResolveSkillIcon(skillID), skillDx0+BTN_W+ICON_SIZE*0.5+0.02, dy,
                              ICON_SIZE, ICON_SIZE, PANEL_LAYER+2)
        self.skillIconIDs[slot] = iconID
    end

    self:_updateStats()
    print("[CharInfoPanel] _show DONE: " .. #self.spriteIDs .. " sprites")
end

function CharacterInfoPanel:_hide()
    self.isOpen = false
    for _, sid in ipairs(self.spriteIDs) do
        if sid and sid > 0 and DestroyEntity then DestroyEntity(sid) end
    end
    self.spriteIDs    = {}
    self.offsets      = {}
    self.portraitID   = 0
    self.bgID         = 0
    self.skillBtnIDs  = {}
    self.skillIconIDs = {}
    if self._healthUI then
        self._healthUI:Destroy()
        self._healthUI = nil
    end
end

-- ============================================================================
-- HELPERS
-- ============================================================================

function CharacterInfoPanel:_getPlayerSkills(playerIndex)
    local state = _G._skillUIState
    if state and state.players and state.players[playerIndex] then
        return state.players[playerIndex]
    end
    return {}
end

function CharacterInfoPanel:_reposition(cx, cy)
    if not SetSpritePosition then return end
    for _, o in ipairs(self.offsets) do
        SetSpritePosition(o.id, cx+o.dx, cy+o.dy)
    end
    self.lastCamX = cx
    self.lastCamY = cy
end

function CharacterInfoPanel:_updateStats()
    -- Stats are now handled by the _healthUI sub-instance (updated each frame in OnDraw)
end

-- ============================================================================
-- DRAW (call from Level3Clean.OnDraw each frame while open)
-- ============================================================================

function CharacterInfoPanel:OnDraw()
    if not self.isOpen then return end

    -- Read camera position at the TOP of OnDraw so cx/cy are in scope for the whole function
    local cx, cy = 0, 0
    if GetCameraPosition then
        local x, y = GetCameraPosition()
        cx = x or 0; cy = y or 0
    end

    -- Follow camera every frame
    if cx ~= self.lastCamX or cy ~= self.lastCamY then
        self:_reposition(cx, cy)
    end

    -- Update HealthUI sub-instance (repositions bars + refreshes fill widths)
    if self._healthUI then
        local camPos = { x = cx, y = cy, z = 0 }
        self._healthUI:UpdateComposite(0, camPos, self.entityID)
    end

    -- Screen-space text overlay
    -- DrawText uses Y=0 at BOTTOM, Y increases upward.
    local fbW, fbH = 1920, 1080
    if GetFramebufferSize then
        local w, h = GetFramebufferSize()
        fbW = w or 1920; fbH = h or 1080
    end
    local scaleRef = fbW / 1920.0

    local charCfg = CHARACTER_CONFIG[self.playerIndex] or CHARACTER_CONFIG[1]
    local skills  = self:_getPlayerSkills(self.playerIndex)
    local defs    = GetSkillDefs()

    -- Name (above portrait)
    DrawText(FONT, charCfg.name, fbW*0.2, fbH*0.65, 1.0*scaleRef, 0, 0, 0)

    -- Stat text: HP / Attack AP / Move AP
    -- Use WorldToScreen to map world bar-center positions → screen pixels.
    -- The HealthUI holder is at: cam + (-0.55, -0.28)
    -- Fill offsets (relative to holder): hp(-0.181,+0.05), atk(+0.043,+0.05), mov(+0.015,-0.047)
    local eid = self.entityID
    if WorldToScreen and eid and eid > 0 then
        local holderX = cx - 0.55
        local holderY = cy - 0.28
        local tScale  = 1.4 * scaleRef

        -- HP (inside the circle, show current/max)
        local hp, maxHP = 0, 0
        if GetEntityHP then hp, maxHP = GetEntityHP(eid); hp = hp or 0; maxHP = maxHP or 0 end
        local hpSX, hpSY = WorldToScreen(holderX - 0.181, holderY + 0.05, true)
        if hpSX then
            local hpTxt = tostring(hp)
            DrawText(FONT, hpTxt, hpSX - #hpTxt*8*scaleRef, hpSY - 10*scaleRef, tScale, 1, 1, 1)
        end

        -- Attack AP (inside blue bar, show current/max)
        local aap, maxAAP = 0, 0
        if GetEntityAttackAP then aap, maxAAP = GetEntityAttackAP(eid); aap = aap or 0; maxAAP = maxAAP or 0 end
        local aSX, aSY = WorldToScreen(holderX + 0.043, holderY + 0.05, true)
        if aSX then
            local aTxt = aap .. "/" .. maxAAP
            DrawText(FONT, aTxt, aSX - #aTxt*5*scaleRef, aSY - 10*scaleRef, tScale, 1, 1, 1)
        end

        -- Move AP (inside brown bar, show current/max)
        local ap, maxAP = 0, 0
        if GetEntityAP then ap, maxAP = GetEntityAP(eid); ap = ap or 0; maxAP = maxAP or 0 end
        local mSX, mSY = WorldToScreen(holderX + 0.015, holderY - 0.047, true)
        if mSX then
            local mTxt = ap .. "/" .. maxAP
            DrawText(FONT, mTxt, mSX - #mTxt*5*scaleRef, mSY - 10*scaleRef, tScale, 1, 1, 1)
        end
    end

    -- Debug: confirm text rendering is working at center of screen
    local s1id = skills["1"] or "nil"
    local defCount = 0; for _ in pairs(defs) do defCount = defCount + 1 end

    -- Skill rows: fixed screen-space coords (same approach as "Knight" name / "[I] Close")
    local nameScale2 = 0.5 * scaleRef
    local descScale  = 0.5 * scaleRef
    local nameX      = fbW * 0.35
    local descX      = fbW * 0.52
    local slot1Y     = fbH * 0.65
    local slotGap    = fbH * 0.10

    for slot = 1, 4 do
        local skillID = skills[tostring(slot)]
        local rowY    = slot1Y - (slot - 1) * slotGap

        if skillID and defs[skillID] then
            local sk      = defs[skillID]
            local name    = sk.name or skillID
            DrawText(FONT, name, nameX, rowY, nameScale2, 1, 1, 1)
            DrawText(FONT, "[" .. (sk.apCost or 0) .. " AP]",
                     nameX, rowY - 18*scaleRef, 0.50*scaleRef, 0.4, 0.8, 1)
            -- Description to the right of icon
            local desc = sk.description or ""
            if #desc > 35 then
                local bp = desc:match(".*()%s") or 35
                DrawText(FONT, desc:sub(1, bp),  descX, rowY,               descScale, 0, 0, 0)
                DrawText(FONT, desc:sub(bp + 1), descX, rowY - 18*scaleRef, descScale, 0, 0, 0)
            else
                DrawText(FONT, desc, descX, rowY, descScale, 0, 0, 0)
            end
        else
            DrawText(FONT, " ", nameX + 10*scaleRef, rowY, nameScale2, 0, 0, 0)
        end
    end

    DrawText(FONT, "[I] Close", fbW*0.85, fbH*0.24, 0.44*scaleRef, 0, 0, 0)
end

-- ============================================================================
-- CLEANUP
-- ============================================================================

function CharacterInfoPanel:Destroy()
    self:_hide()
end

return CharacterInfoPanel
