--[[
===============================================================================
 File:          HealthUI.lua
 Authors:       
 Co-Authors:    kahyan.sim
 Date:          
 Contribution:  
 ------------------------------------------------------------------------------

HEALTH UI - Player Health & AP Display Component

 Brief:
   Supports two modes:
   1) Legacy: single heart sprite with texture swap (Health_0..5.png)
   2) Composite HUD: a holder texture with three fills:
      - HP (vertical fill in circle)
      - Attack AP (horizontal fill, right -> left)
      - Movement AP (horizontal fill, right -> left)

 Features:
    - Single sprite approach with texture swapping
    - Supports HP values 0-5 (configurable max)
    - Camera-relative positioning
    - Party system integration (tracks active character)
    - Smooth handling during enemy turns

 Usage:
    local HealthUI = require("UI/HealthUI")
    
    local healthDisplay = HealthUI:New()
    healthDisplay:Init({
        maxHP = 5,
        scale = 0.10,
        offsetX = -0.72,
        offsetY = -0.22,
        layer = 4,
        textureBasePath = "assets/UI/Health_"
    })

    -- In update loop
    healthDisplay:Update(dt, cameraPos)

    -- Cleanup
    healthDisplay:Destroy()


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
]]--

local UIComponent = require("UI/UIComponent")
local HealthUI = {}
setmetatable(HealthUI, {__index = UIComponent})
HealthUI.__index = HealthUI

-- ============================================================================
-- CONSTRUCTOR
-- ============================================================================

function HealthUI:New()
    local instance = UIComponent:New()
    setmetatable(instance, self)
    return instance
end

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function HealthUI:Init(config)
    self.config = config or {}

    -- Configuration
    self.maxHP = self.config.maxHP or 5
    self.scale = self.config.scale or 0.10
    self.offsetX = self.config.offsetX or -0.72
    self.offsetY = self.config.offsetY or -0.22
    self.layer = self.config.layer or 4

    -- Composite HUD config (if holderTexture is provided, we use composite mode)
    self.holderTexture = self.config.holderTexture
    self.holderFrameTexture = self.config.holderFrameTexture
    self.holderOnly = self.config.holderOnly or false
    self.enableHPFill = self.config.enableHPFill
    if self.enableHPFill == nil then self.enableHPFill = true end
    self.enableAttackFill = self.config.enableAttackFill
    if self.enableAttackFill == nil then self.enableAttackFill = true end
    self.enableMoveFill = self.config.enableMoveFill
    if self.enableMoveFill == nil then self.enableMoveFill = true end
    self.holderScaleX = self.config.holderScaleX or self.config.scale or 0.30
    self.holderScaleY = self.config.holderScaleY or self.config.scale or 0.30
    self.holderOffsetX = self.config.holderOffsetX or self.offsetX
    self.holderOffsetY = self.config.holderOffsetY or self.offsetY
    self.holderFrameScaleX = self.config.holderFrameScaleX or self.holderScaleX
    self.holderFrameScaleY = self.config.holderFrameScaleY or self.holderScaleY
    self.holderFrameOffsetX = self.config.holderFrameOffsetX or self.holderOffsetX
    self.holderFrameOffsetY = self.config.holderFrameOffsetY or self.holderOffsetY

    self.hpFillOffsetX = self.config.hpFillOffsetX or -0.18
    self.hpFillOffsetY = self.config.hpFillOffsetY or 0.05
    self.hpFillWidth = self.config.hpFillWidth or 0.12
    self.hpFillHeight = self.config.hpFillHeight or 0.12
    self.hpFillColor = self.config.hpFillColor or { r = 0.9, g = 0.1, b = 0.1, a = 1.0 }
    self.hpFillTexture = self.config.hpFillTexture or ""

    self.attackFillOffsetX = self.config.attackFillOffsetX or 0.08
    self.attackFillOffsetY = self.config.attackFillOffsetY or 0.06
    self.attackFillWidth = self.config.attackFillWidth or 0.42
    self.attackFillHeight = self.config.attackFillHeight or 0.06
    self.attackFillColor = self.config.attackFillColor or { r = 0.2, g = 0.45, b = 1.0, a = 1.0 }
    self.maxAttackAP = self.config.maxAttackAP or 3

    self.moveFillOffsetX = self.config.moveFillOffsetX or 0.08
    self.moveFillOffsetY = self.config.moveFillOffsetY or -0.06
    self.moveFillWidth = self.config.moveFillWidth or 0.42
    self.moveFillHeight = self.config.moveFillHeight or 0.06
    self.moveFillColor = self.config.moveFillColor or { r = 0.65, g = 0.35, b = 0.15, a = 1.0 }
    self.maxMoveAP = self.config.maxMoveAP or 5

    self.getMovementAPFunc = self.config.getMovementAPFunc
    self.getAttackAPFunc = self.config.getAttackAPFunc

    -- Value text (numbers inside fills)
    self.showValueText = self.config.showValueText
    if self.showValueText == nil then self.showValueText = true end
    self.textFont = self.config.textFont or "Jersey20Regular"
    self.textScale = self.config.textScale or 0.3
    self.hpTextScale = self.config.hpTextScale or self.textScale
    self.apTextScale = self.config.apTextScale or self.textScale
    self.textColor = self.config.textColor or { r = 1.0, g = 1.0, b = 1.0, a = 1.0 }
    self.hpTextColor = self.config.hpTextColor or self.textColor
    self.attackTextColor = self.config.attackTextColor or self.textColor
    self.moveTextColor = self.config.moveTextColor or self.textColor
    self.hpTextOffsetX = self.config.hpTextOffsetX or 0.0
    self.hpTextOffsetY = self.config.hpTextOffsetY or 0.0
    self.attackTextOffsetX = self.config.attackTextOffsetX or 0.0
    self.attackTextOffsetY = self.config.attackTextOffsetY or 0.0
    self.moveTextOffsetX = self.config.moveTextOffsetX or 0.0
    self.moveTextOffsetY = self.config.moveTextOffsetY or 0.0
    self.textBaseSize = self.config.textBaseSize or 48
    self.textWidthFactor = self.config.textWidthFactor or 0.6
    self.hpTextShowPercent = self.config.hpTextShowPercent
    if self.hpTextShowPercent == nil then self.hpTextShowPercent = true end
    self.useViewportCoordsForText = self.config.useViewportCoordsForText
    if self.useViewportCoordsForText == nil then self.useViewportCoordsForText = true end

    -- Texture base path (legacy mode)
    self.textureBasePath = self.config.textureBasePath or "assets/UI/Health_"

    -- State
    self.healthSpriteID = 0  -- Legacy single sprite
    self.holderSpriteID = 0
    self.holderFrameSpriteID = 0
    self.hpFillSpriteID = 0
    self.attackFillSpriteID = 0
    self.moveFillSpriteID = 0
    self.lastHP = -1
    self.lastAttackAP = -1
    self.lastMoveAP = -1
    self.lastMaxAttackAP = -1
    self.lastMaxMoveAP = -1
    self.trackedCharID = 0   -- Currently tracked character ID

    -- Get camera position
    local camX, camY, camZ = GetCameraPosition()

    -- Get character to track: try party system first, fallback to FindPlayer
    local charID = nil
    if GetActiveCharacter then
        charID = GetActiveCharacter()
    end
    if (not charID or charID <= 0) and FindPlayer then
        charID = FindPlayer()
    end
    
    self.trackedCharID = charID or 0
    local curHP, maxHP = 5, 5  -- Default
    if charID and charID > 0 then
        curHP, maxHP = GetEntityHP(charID)
    end
    if maxHP and maxHP > 0 then
        self.maxHP = maxHP
    end
    if not curHP then
        curHP = self.maxHP
    end
    self.lastHP = curHP

    if self.holderTexture then
        -- Composite HUD: holder + fills
        local x = camX + self.holderOffsetX
        local y = camY + self.holderOffsetY
        self.holderSpriteID = self:SpawnSprite(self.holderTexture, x, y, self.holderScaleX, self.holderScaleY, self.layer)

        if self.holderOnly then
            Log("[HealthUI] Initialized - Holder only")
            return
        end

        -- HP fill (vertical)
        if self.enableHPFill then
            self.hpFillSpriteID = self:SpawnSprite(self.hpFillTexture, x + self.hpFillOffsetX, y + self.hpFillOffsetY, self.hpFillWidth, self.hpFillHeight, self.layer + 1)
            if self.hpFillSpriteID and self.hpFillSpriteID > 0 then
                SetSpriteColor(self.hpFillSpriteID, self.hpFillColor.r, self.hpFillColor.g, self.hpFillColor.b, self.hpFillColor.a or 1.0)
            end
        end

        -- Attack AP fill (top bar)
        if self.enableAttackFill then
            self.attackFillSpriteID = self:SpawnSprite("", x + self.attackFillOffsetX, y + self.attackFillOffsetY, self.attackFillWidth, self.attackFillHeight, self.layer + 1)
            if self.attackFillSpriteID and self.attackFillSpriteID > 0 then
                SetSpriteColor(self.attackFillSpriteID, self.attackFillColor.r, self.attackFillColor.g, self.attackFillColor.b, self.attackFillColor.a or 1.0)
            end
        end

        -- Movement AP fill (bottom bar)
        if self.enableMoveFill then
            self.moveFillSpriteID = self:SpawnSprite("", x + self.moveFillOffsetX, y + self.moveFillOffsetY, self.moveFillWidth, self.moveFillHeight, self.layer + 1)
            if self.moveFillSpriteID and self.moveFillSpriteID > 0 then
                SetSpriteColor(self.moveFillSpriteID, self.moveFillColor.r, self.moveFillColor.g, self.moveFillColor.b, self.moveFillColor.a or 1.0)
            end
        end

        -- Holder frame overlay (on top of fills)
        if self.holderFrameTexture then
            self.holderFrameSpriteID = self:SpawnSprite(self.holderFrameTexture, x + (self.holderFrameOffsetX - self.holderOffsetX), y + (self.holderFrameOffsetY - self.holderOffsetY), self.holderFrameScaleX, self.holderFrameScaleY, self.layer + 2)
        end

        -- Initial fill update
        self:UpdateCompositeFills(curHP, maxHP, charID)

        Log("[HealthUI] Initialized - Composite HUD (" .. curHP .. "/" .. self.maxHP .. " HP)")
        return
    end

    -- Legacy: create a SINGLE health sprite with the current HP texture
    local texture = self.textureBasePath .. tostring(curHP) .. ".png"
    local x = camX + self.offsetX
    local y = camY + self.offsetY

    self.healthSpriteID = self:SpawnSprite(texture, x, y, self.scale, self.scale, self.layer)

    Log("[HealthUI] Initialized - " .. curHP .. "/" .. self.maxHP .. " HP (single sprite approach)")
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function HealthUI:Update(dt, cameraPos)
    if not self.enabled then return end

    local displayCharID = self:GetDisplayCharacterID()
    if not displayCharID then
        return
    end

    if self.holderTexture then
        if self.holderOnly then
            if self:ShouldUpdatePosition(cameraPos) then
                self:UpdatePositions(cameraPos)
            end
            return
        end
        self:UpdateComposite(dt, cameraPos, displayCharID)
        return
    end

    -- Legacy single-sprite HP
    local curHP, maxHP = GetEntityHP(displayCharID)
    if not curHP then
        return
    end

    if self:ShouldUpdatePosition(cameraPos) then
        self:UpdatePositions(cameraPos)
    end

    if curHP ~= self.lastHP then
        self:HandleHPChange(curHP)
        self.lastHP = curHP
    end
end

-- ============================================================================
-- HELPER METHODS
-- ============================================================================

function HealthUI:UpdatePositions(cameraPos)
    if self.holderTexture then
        local x = cameraPos.x + self.holderOffsetX
        local y = cameraPos.y + self.holderOffsetY
        if self.holderSpriteID and self.holderSpriteID > 0 then
            SetSpritePosition(self.holderSpriteID, x, y)
        end
        if self.holderOnly then
            return
        end
        if self.hpFillSpriteID and self.hpFillSpriteID > 0 then
            SetSpritePosition(self.hpFillSpriteID, x + self.hpFillOffsetX, y + self.hpFillOffsetY)
        end
        if self.attackFillSpriteID and self.attackFillSpriteID > 0 then
            SetSpritePosition(self.attackFillSpriteID, x + self.attackFillOffsetX, y + self.attackFillOffsetY)
        end
        if self.moveFillSpriteID and self.moveFillSpriteID > 0 then
            SetSpritePosition(self.moveFillSpriteID, x + self.moveFillOffsetX, y + self.moveFillOffsetY)
        end
        if self.holderFrameSpriteID and self.holderFrameSpriteID > 0 then
            SetSpritePosition(self.holderFrameSpriteID, x + (self.holderFrameOffsetX - self.holderOffsetX), y + (self.holderFrameOffsetY - self.holderOffsetY))
        end
        return
    end

    local x = cameraPos.x + self.offsetX
    local y = cameraPos.y + self.offsetY

    if self.healthSpriteID and self.healthSpriteID > 0 then
        SetSpritePosition(self.healthSpriteID, x, y)
    end
end

function HealthUI:HandleHPChange(newHP)
    -- Clamp HP to valid range
    if newHP < 0 then newHP = 0 end
    if newHP > self.maxHP then newHP = self.maxHP end

    -- Change the texture on our single sprite
    local newTexture = self.textureBasePath .. tostring(newHP) .. ".png"

    if self.healthSpriteID and self.healthSpriteID > 0 and SetSpriteTexture then
        SetSpriteTexture(self.healthSpriteID, newTexture)
    end

    Log("[HealthUI] HP changed: " .. self.lastHP .. " -> " .. newHP)
end

-- ============================================================================
-- COMPOSITE HUD MODE
-- ============================================================================

function HealthUI:GetDisplayCharacterID()
    -- Get character to display HP for
    -- Priority: 1) Active character from party system, 2) Keep last tracked character during enemy turn
    local charID = nil
    local currentTurn = GetCurrentTurn and GetCurrentTurn() or "Player"
    local partyMembers = nil

    if GetPartyMembers then
        partyMembers = GetPartyMembers()
    end

    local function IsPartyMember(id)
        if not id or id <= 0 then return false end
        if not partyMembers then return true end
        for i = 1, #partyMembers do
            if partyMembers[i] == id then
                return true
            end
        end
        return false
    end

    if GetActiveCharacter then
        charID = GetActiveCharacter()
    end

    -- If we have a valid active character, always follow it
    if charID and charID > 0 and IsPartyMember(charID) and self.trackedCharID ~= charID then
        self.trackedCharID = charID
        self.lastHP = -1
        self.lastAttackAP = -1
        self.lastMoveAP = -1
        self.lastMaxAttackAP = -1
        self.lastMaxMoveAP = -1
    end

    -- If active character is NOT a party member (e.g., enemy turn), fall back to tracked/party
    if charID and charID > 0 and not IsPartyMember(charID) then
        charID = nil
    end

    -- During enemy turn, keep tracking the last active character (don't switch)
    if currentTurn == "Enemy" and self.trackedCharID and self.trackedCharID > 0 then
        charID = self.trackedCharID
    end

    -- Fallback to FindPlayer only if we have no tracked character at all
    if (not charID or charID <= 0) and (not self.trackedCharID or self.trackedCharID <= 0) then
        if FindPlayer then
            charID = FindPlayer()
        end
    end

    -- Still no character? Use last tracked character if valid
    if (not charID or charID <= 0) and self.trackedCharID and self.trackedCharID > 0 then
        charID = self.trackedCharID
    end

    -- If we have party members, prefer the first party member as a safe fallback
    if (not charID or charID <= 0) and partyMembers and #partyMembers > 0 then
        charID = partyMembers[1]
    end

    if not charID or charID <= 0 then
        return nil
    end

    return self.trackedCharID or charID
end

function HealthUI:UpdateComposite(dt, cameraPos, displayCharID)
    local curHP, maxHP = GetEntityHP(displayCharID)
    if not curHP then
        return
    end

    if maxHP and maxHP > self.maxHP then
        self.maxHP = maxHP
    end
    local effectiveMaxHP = self.maxHP
    if not effectiveMaxHP or effectiveMaxHP <= 0 then
        effectiveMaxHP = maxHP
    end

    local currentAttackAP, maxAttackAP = 0, 0
    local currentMoveAP, maxMoveAP = 0, 0
    if self.enableAttackFill then
        currentAttackAP, maxAttackAP = self:GetAttackAP(displayCharID)
    end
    if self.enableMoveFill then
        currentMoveAP, maxMoveAP = self:GetMoveAP(displayCharID)
    end
    if maxAttackAP and maxAttackAP > 0 then
        self.maxAttackAP = maxAttackAP
    end
    if maxMoveAP and maxMoveAP > 0 then
        self.maxMoveAP = maxMoveAP
    end

    local positionUpdated = false
    if self:ShouldUpdatePosition(cameraPos) then
        self:UpdatePositions(cameraPos)
        positionUpdated = true
    end

    if curHP ~= self.lastHP or positionUpdated then
        self:UpdateHPFill(curHP, effectiveMaxHP)
        self.lastHP = curHP
    end

    if self.enableAttackFill then
        if currentAttackAP ~= self.lastAttackAP or maxAttackAP ~= self.lastMaxAttackAP or positionUpdated then
            self:UpdateAttackFill(currentAttackAP, maxAttackAP)
            self.lastAttackAP = currentAttackAP
            self.lastMaxAttackAP = maxAttackAP
        end
    end

    if self.enableMoveFill then
        if currentMoveAP ~= self.lastMoveAP or maxMoveAP ~= self.lastMaxMoveAP or positionUpdated then
            self:UpdateMoveFill(currentMoveAP, maxMoveAP)
            self.lastMoveAP = currentMoveAP
            self.lastMaxMoveAP = maxMoveAP
        end
    end
end

function HealthUI:GetAttackAP(entityID)
    local currentAP, maxAP = nil, nil
    if self.getAttackAPFunc then
        currentAP, maxAP = self.getAttackAPFunc(entityID)
        if currentAP == nil then currentAP = 0 end
        if not maxAP or maxAP <= 0 then maxAP = self.maxAttackAP end
        return currentAP, maxAP
    end
    if entityID and entityID > 0 and GetEntityAttackAP then
        currentAP, maxAP = GetEntityAttackAP(entityID)
        if currentAP == nil then currentAP = 0 end
        if not maxAP or maxAP <= 0 then maxAP = self.maxAttackAP end
        return currentAP, maxAP
    end
    if GetPlayerAttackAP then
        currentAP, maxAP = GetPlayerAttackAP()
        if currentAP == nil then currentAP = 0 end
        if not maxAP or maxAP <= 0 then maxAP = self.maxAttackAP end
        return currentAP, maxAP
    end
    return 0, self.maxAttackAP
end

function HealthUI:GetMoveAP(entityID)
    local currentAP, maxAP = nil, nil
    if self.getMovementAPFunc then
        currentAP, maxAP = self.getMovementAPFunc(entityID)
        if currentAP == nil then currentAP = 0 end
        if not maxAP or maxAP <= 0 then maxAP = self.maxMoveAP end
        return currentAP, maxAP
    end
    if entityID and entityID > 0 and GetEntityAP then
        currentAP, maxAP = GetEntityAP(entityID)
        if currentAP == nil then currentAP = 0 end
        if not maxAP or maxAP <= 0 then maxAP = self.maxMoveAP end
        return currentAP, maxAP
    end
    if GetPlayerAP then
        currentAP, maxAP = GetPlayerAP()
        if currentAP == nil then currentAP = 0 end
        if not maxAP or maxAP <= 0 then maxAP = self.maxMoveAP end
        return currentAP, maxAP
    end
    return 0, self.maxMoveAP
end

function HealthUI:UpdateCompositeFills(curHP, maxHP, entityID)
    if self.enableHPFill then
        self:UpdateHPFill(curHP, maxHP)
    end
    if self.enableAttackFill then
        local currentAttackAP, maxAttackAP = self:GetAttackAP(entityID)
        self:UpdateAttackFill(currentAttackAP, maxAttackAP)
    end
    if self.enableMoveFill then
        local currentMoveAP, maxMoveAP = self:GetMoveAP(entityID)
        self:UpdateMoveFill(currentMoveAP, maxMoveAP)
    end
end

function HealthUI:UpdateHPFill(curHP, maxHP)
    if not self.hpFillSpriteID or self.hpFillSpriteID <= 0 then return end
    if not maxHP or maxHP <= 0 then return end

    local ratio = curHP / maxHP
    if ratio < 0 then ratio = 0 end
    if ratio > 1 then ratio = 1 end

    local fillWidth = self.hpFillWidth
    local fillHeight = self.hpFillHeight * ratio
    local camX, camY, camZ = GetCameraPosition()
    local baseX = camX + self.holderOffsetX + self.hpFillOffsetX
    local baseY = camY + self.holderOffsetY + self.hpFillOffsetY

    -- Anchor from bottom to top (fill shrinks upward)
    local bottomY = baseY - (self.hpFillHeight * 0.5)
    local centerY = bottomY + (fillHeight * 0.5)

    SetSpritePosition(self.hpFillSpriteID, baseX, centerY)
    SetScale(self.hpFillSpriteID, fillWidth, fillHeight)

    -- If available, crop UVs to avoid vertical squish (cutoff effect)
    if SetSpriteUVRect then
        SetSpriteUVRect(self.hpFillSpriteID, 0.0, 0.0, 1.0, ratio)
    end
end

function HealthUI:UpdateAttackFill(currentAP, maxAP)
    if not self.attackFillSpriteID or self.attackFillSpriteID <= 0 then return end
    if not maxAP or maxAP <= 0 then return end

    local ratio = currentAP / maxAP
    if ratio < 0 then ratio = 0 end
    if ratio > 1 then ratio = 1 end

    local fillWidth = self.attackFillWidth * ratio
    local camX, camY, camZ = GetCameraPosition()
    local barCenterX = camX + self.holderOffsetX + self.attackFillOffsetX
    local barCenterY = camY + self.holderOffsetY + self.attackFillOffsetY
    local barLeftX = barCenterX - (self.attackFillWidth * 0.5)
    local fillCenterX = barLeftX + (fillWidth * 0.5)

    SetSpritePosition(self.attackFillSpriteID, fillCenterX, barCenterY)
    SetScale(self.attackFillSpriteID, fillWidth, self.attackFillHeight)
end

function HealthUI:UpdateMoveFill(currentAP, maxAP)
    if not self.moveFillSpriteID or self.moveFillSpriteID <= 0 then return end
    if not maxAP or maxAP <= 0 then return end

    local ratio = currentAP / maxAP
    if ratio < 0 then ratio = 0 end
    if ratio > 1 then ratio = 1 end

    local fillWidth = self.moveFillWidth * ratio
    local camX, camY, camZ = GetCameraPosition()
    local barCenterX = camX + self.holderOffsetX + self.moveFillOffsetX
    local barCenterY = camY + self.holderOffsetY + self.moveFillOffsetY
    local barLeftX = barCenterX - (self.moveFillWidth * 0.5)
    local fillCenterX = barLeftX + (fillWidth * 0.5)

    SetSpritePosition(self.moveFillSpriteID, fillCenterX, barCenterY)
    SetScale(self.moveFillSpriteID, fillWidth, self.moveFillHeight)
end

-- ============================================================================
-- DRAW (Value Text)
-- ============================================================================

function HealthUI:Draw()
    if not self.enabled then return end
    if not self.holderTexture or self.holderOnly then return end
    if not self.showValueText then return end
    if not DrawText or not WorldToScreen then return end
    if IsPaused and IsPaused() then return end

    local fbW, fbH = GetFramebufferSize()
    if not fbW or fbW <= 0 or not fbH or fbH <= 0 then return end

    local camX, camY, camZ = GetCameraPosition()
    local scaleRef = fbW / 1920

    local function drawCenteredText(worldX, worldY, text, scale, color)
        if not text or text == "" then return end
        local screenX, screenY = WorldToScreen(worldX, worldY, self.useViewportCoordsForText)
        if not screenX or not screenY then return end

        local finalScale = scale * scaleRef
        local approxWidth = #text * self.textBaseSize * self.textWidthFactor * finalScale
        local approxHeight = self.textBaseSize * finalScale
        local x = screenX - (approxWidth * 0.5)
        local y = screenY - (approxHeight * 0.5)

        DrawText(self.textFont, text, x, y, finalScale, color.r, color.g, color.b)
    end

    -- HP text (scale to 0-100)
    local maxHP = self.maxHP
    if not maxHP or maxHP <= 0 then maxHP = 1 end
    local curHP = self.lastHP
    if not curHP or curHP < 0 then curHP = 0 end
    local hpPercent = math.floor((curHP / maxHP) * 100 + 0.5)
    local hpText = self.hpTextShowPercent and tostring(hpPercent) or tostring(curHP)
    local hpWorldX = camX + self.holderOffsetX + self.hpFillOffsetX + self.hpTextOffsetX
    local hpWorldY = camY + self.holderOffsetY + self.hpFillOffsetY + self.hpTextOffsetY
    drawCenteredText(hpWorldX, hpWorldY, hpText, self.hpTextScale, self.hpTextColor)

    -- Attack AP text (current/max)
    if self.enableAttackFill then
        local curAttack = self.lastAttackAP
        local maxAttack = self.lastMaxAttackAP
        if not maxAttack or maxAttack <= 0 then maxAttack = self.maxAttackAP end
        if not curAttack or curAttack < 0 then curAttack = 0 end
        local attackText = string.format("%d/%d", curAttack, maxAttack)
        local attackWorldX = camX + self.holderOffsetX + self.attackFillOffsetX + self.attackTextOffsetX
        local attackWorldY = camY + self.holderOffsetY + self.attackFillOffsetY + self.attackTextOffsetY
        drawCenteredText(attackWorldX, attackWorldY, attackText, self.apTextScale, self.attackTextColor)
    end

    -- Movement AP text (current/max)
    if self.enableMoveFill then
        local curMove = self.lastMoveAP
        local maxMove = self.lastMaxMoveAP
        if not maxMove or maxMove <= 0 then maxMove = self.maxMoveAP end
        if not curMove or curMove < 0 then curMove = 0 end
        local moveText = string.format("%d/%d", curMove, maxMove)
        local moveWorldX = camX + self.holderOffsetX + self.moveFillOffsetX + self.moveTextOffsetX
        local moveWorldY = camY + self.holderOffsetY + self.moveFillOffsetY + self.moveTextOffsetY
        drawCenteredText(moveWorldX, moveWorldY, moveText, self.apTextScale, self.moveTextColor)
    end
end

function HealthUI:Destroy()
    UIComponent.Destroy(self)
    self.healthSpriteID = 0
    self.holderSpriteID = 0
    self.holderFrameSpriteID = 0
    self.hpFillSpriteID = 0
    self.attackFillSpriteID = 0
    self.moveFillSpriteID = 0
    Log("[HealthUI] Destroyed")
end

return HealthUI
