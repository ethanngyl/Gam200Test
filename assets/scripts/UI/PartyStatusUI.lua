--[[
===============================================================================
 File:           PartyStatusUI.lua
 Author:         Auto-generated for party system implementation
 Date:           2026-01-16
 ------------------------------------------------------------------------------
 Party Status UI Component

 Purpose:
    Displays HP and AP status for all 3 party members
    Highlights the currently active character
    Grays out inactive characters

 Features:
    - Shows all 3 characters in a row
    - Health hearts (5 max)
    - AP crystals (5 max)
    - Active character highlight
    - Real-time updates
    - Responsive to party turn changes

 Usage:
    -- In Level3Clean.lua OnInit():
    partyUI = UIComponent:new(0)
    LoadScriptFromFile(partyUI.scriptState, "assets/scripts/UI/PartyStatusUI.lua")

===============================================================================
]]--

-- Import UIComponent base class (capture the returned module)
UIComponent = dofile("assets/scripts/UI/UIComponent.lua")

-- Inherit from UIComponent
PartyStatusUI = setmetatable({}, {__index = UIComponent})
PartyStatusUI.__index = PartyStatusUI

-- ============================================================================
-- CONSTRUCTOR
-- ============================================================================

function PartyStatusUI:new(entityID)
    -- Use UIComponent:New() (capital N) - base class constructor
    local instance = setmetatable(UIComponent:New(), PartyStatusUI)

    -- Store entity ID (unused for now, but kept for interface compatibility)
    instance.entityID = entityID

    -- UI configuration
    instance.baseX = -0.8          -- Left side of screen
    instance.baseY = -0.85         -- Bottom of screen
    instance.characterSpacing = 0.5  -- Space between characters
    instance.iconSize = 0.04       -- Size of HP/AP icons

    -- Character slots
    instance.characterSlots = {}   -- {character1, character2, character3}

    -- Icons
    instance.heartIcon = "assets/UI/heart.png"
    instance.apIcon = "assets/UI/gem.png"
    instance.highlightBorder = "assets/UI/HighlightBorder.png"

    -- Colors
    instance.activeColor = {r = 1.0, g = 1.0, b = 1.0}    -- White (active)
    instance.inactiveColor = {r = 0.5, g = 0.5, b = 0.5}  -- Gray (inactive)

    -- Cached party data
    instance.partyMembers = {}

    -- Initialization flag
    instance.initialized = false

    return instance
end

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function PartyStatusUI:OnInit()
    Log("[PartyStatusUI] Initializing party status display")

    -- Get party members
    self.partyMembers = GetPartyMembers()

    if not self.partyMembers or #self.partyMembers ~= 3 then
        Log("[PartyStatusUI] ERROR: Expected 3 party members, got " .. (#self.partyMembers or 0))
        return
    end

    -- Create UI for each character
    for i = 1, 3 do
        self:CreateCharacterSlot(i)
    end

    self.initialized = true
    Log("[PartyStatusUI] Initialized successfully with 3 characters")
end

-- ============================================================================
-- CHARACTER SLOT CREATION
-- ============================================================================

function PartyStatusUI:CreateCharacterSlot(slotIndex)
    local slot = {}
    local entityID = self.partyMembers[slotIndex]

    -- Calculate position for this slot
    local posX = self.baseX + (slotIndex - 1) * self.characterSpacing
    local posY = self.baseY

    -- Get character info
    local charInfo = GetPartyMemberInfo(slotIndex)
    local charName = charInfo and charInfo.name or ("Char" .. slotIndex)

    Log(string.format("[PartyStatusUI] Creating slot %d for %s at (%.2f, %.2f)",
        slotIndex, charName, posX, posY))

    -- Character name text
    slot.nameText = {
        x = posX,
        y = posY + 0.08,
        text = charName,
        size = 0.03
    }

    -- HP hearts (5 max)
    slot.hearts = {}
    for i = 1, 5 do
        local heartX = posX + (i - 1) * self.iconSize * 1.2
        local heartY = posY + 0.04
        local heartID = self:SpawnSprite(self.heartIcon, heartX, heartY, self.iconSize, self.iconSize, 100)
        table.insert(slot.hearts, heartID)
    end

    -- AP crystals (5 max)
    slot.apCrystals = {}
    for i = 1, 5 do
        local crystalX = posX + (i - 1) * self.iconSize * 1.2
        local crystalY = posY
        local crystalID = self:SpawnSprite(self.apIcon, crystalX, crystalY, self.iconSize, self.iconSize, 100)
        table.insert(slot.apCrystals, crystalID)
    end

    -- Highlight border (shown only for active character)
    slot.highlightBorder = self:SpawnSprite(
        self.highlightBorder,
        posX + 0.1,
        posY + 0.02,
        0.25,
        0.12,
        99  -- Behind icons
    )
    -- Start hidden
    SetSpriteVisibility(slot.highlightBorder, false)

    slot.entityID = entityID
    slot.slotIndex = slotIndex

    self.characterSlots[slotIndex] = slot
end

-- ============================================================================
-- UPDATE LOGIC
-- ============================================================================

function PartyStatusUI:OnUpdate(dt)
    if not self.initialized then
        return
    end

    -- Update each character slot
    for i = 1, #self.characterSlots do
        self:UpdateCharacterSlot(i)
    end
end

function PartyStatusUI:UpdateCharacterSlot(slotIndex)
    local slot = self.characterSlots[slotIndex]
    if not slot then
        return
    end

    local entityID = slot.entityID

    -- Get current HP and AP
    local currentHP, maxHP = GetEntityHP(entityID)
    local currentAP, maxAP = GetEntityAP(entityID)

    -- Update HP hearts visibility
    for i = 1, #slot.hearts do
        local visible = (i <= currentHP)
        SetSpriteVisibility(slot.hearts[i], visible)
    end

    -- Update AP crystals visibility
    for i = 1, #slot.apCrystals do
        local visible = (i <= currentAP)
        SetSpriteVisibility(slot.apCrystals[i], visible)
    end

    -- Update active/inactive state
    local isActive = IsActiveCharacter(entityID)

    if isActive then
        -- Active character: white color, show highlight
        self:SetSlotColor(slot, self.activeColor)
        SetSpriteVisibility(slot.highlightBorder, true)
    else
        -- Inactive character: gray color, hide highlight
        self:SetSlotColor(slot, self.inactiveColor)
        SetSpriteVisibility(slot.highlightBorder, false)
    end
end

-- ============================================================================
-- HELPER FUNCTIONS
-- ============================================================================

function PartyStatusUI:SetSlotColor(slot, color)
    -- Set color for hearts
    for i = 1, #slot.hearts do
        SetSpriteColor(slot.hearts[i], color.r, color.g, color.b)
    end

    -- Set color for AP crystals
    for i = 1, #slot.apCrystals do
        SetSpriteColor(slot.apCrystals[i], color.r, color.g, color.b)
    end
end

-- ============================================================================
-- DRAW TEXT (Optional - if text rendering is available)
-- ============================================================================

function PartyStatusUI:OnDraw()
    if not self.initialized then
        return
    end

    -- Draw character names
    for i = 1, #self.characterSlots do
        local slot = self.characterSlots[i]

        -- Get character name
        local charInfo = GetPartyMemberInfo(i)
        if charInfo then
            local isActive = IsActiveCharacter(slot.entityID)
            local color = isActive and self.activeColor or self.inactiveColor

            -- Draw character name (if DrawText API is available)
            -- DrawText(charInfo.name, slot.nameText.x, slot.nameText.y, slot.nameText.size, color.r, color.g, color.b)

            -- Also show current HP/AP as text
            local currentHP, maxHP = GetEntityHP(slot.entityID)
            local currentAP, maxAP = GetEntityAP(slot.entityID)

            -- DrawText(string.format("HP:%d/%d AP:%d/%d", currentHP, maxHP, currentAP, maxAP),
            --          slot.nameText.x, slot.nameText.y - 0.04, 0.02, color.r, color.g, color.b)
        end
    end
end

-- ============================================================================
-- CLEANUP
-- ============================================================================

function PartyStatusUI:OnDestroy()
    Log("[PartyStatusUI] Destroying party status UI")

    -- Destroy all character slot entities
    for i = 1, #self.characterSlots do
        local slot = self.characterSlots[i]

        -- Destroy hearts
        for j = 1, #slot.hearts do
            DestroyEntity(slot.hearts[j])
        end

        -- Destroy AP crystals
        for j = 1, #slot.apCrystals do
            DestroyEntity(slot.apCrystals[j])
        end

        -- Destroy highlight border
        if slot.highlightBorder then
            DestroyEntity(slot.highlightBorder)
        end
    end

    self.characterSlots = {}
    self.initialized = false
end

-- Return the class for use in other scripts
return PartyStatusUI
