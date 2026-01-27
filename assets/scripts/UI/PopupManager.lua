--[[
===============================================================================
 File:           PopupManager.lua
 Author:         Generated for one-time animation support
 Date:           2026-01-27
 ------------------------------------------------------------------------------
 Popup Manager - Handles temporary UI animations and pop-up effects

 Purpose:
    Manages one-time animations like:
    - Damage numbers
    - Status effect notifications
    - Tutorial hints
    - Item pickups
    - Ability cooldown indicators

 Usage:
    local PopupManager = require("PopupManager")
    PopupManager.Init()
    PopupManager.ShowDamageNumber(x, y, damage)
    PopupManager.ShowText(x, y, "Critical Hit!", color)

===============================================================================
]]--

local PopupManager = {}

-- ============================================================================
-- POPUP STATE
-- ============================================================================

-- Active popups list
local activePopups = {}
local nextPopupID = 1

-- Popup types
local PopupType = {
    DAMAGE_NUMBER = "damage",
    HEAL_NUMBER = "heal",
    TEXT = "text",
    ICON = "icon",
    SPRITE_ANIM = "sprite_anim"
}

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function PopupManager.Init()
    activePopups = {}
    nextPopupID = 1
    print("[PopupManager] Initialized")
end

-- ============================================================================
-- CREATE POPUP FUNCTIONS
-- ============================================================================

--[[
    ShowDamageNumber(x, y, damage)
    Shows a damage number that floats upward and fades out

    @param x World X position
    @param y World Y position
    @param damage Damage amount to display
    @return Popup ID
]]--
function PopupManager.ShowDamageNumber(x, y, damage)
    local popup = {
        id = nextPopupID,
        type = PopupType.DAMAGE_NUMBER,
        x = x,
        y = y,
        startY = y,
        text = tostring(damage),
        lifetime = 1.0,      -- Total duration in seconds
        elapsed = 0.0,       -- Time elapsed
        floatSpeed = 0.3,    -- Units per second upward
        fadeDelay = 0.3,     -- Start fading after this many seconds
        color = {r = 1.0, g = 0.2, b = 0.2}  -- Red for damage
    }

    table.insert(activePopups, popup)
    nextPopupID = nextPopupID + 1

    print("[PopupManager] Damage popup created: " .. damage .. " at (" .. x .. ", " .. y .. ")")
    return popup.id
end

--[[
    ShowHealNumber(x, y, amount)
    Shows a heal number that floats upward in green

    @param x World X position
    @param y World Y position
    @param amount Heal amount to display
    @return Popup ID
]]--
function PopupManager.ShowHealNumber(x, y, amount)
    local popup = {
        id = nextPopupID,
        type = PopupType.HEAL_NUMBER,
        x = x,
        y = y,
        startY = y,
        text = "+" .. tostring(amount),
        lifetime = 1.0,
        elapsed = 0.0,
        floatSpeed = 0.3,
        fadeDelay = 0.3,
        color = {r = 0.2, g = 1.0, b = 0.2}  -- Green for healing
    }

    table.insert(activePopups, popup)
    nextPopupID = nextPopupID + 1

    return popup.id
end

--[[
    ShowText(x, y, text, color, lifetime)
    Shows custom text that floats and fades

    @param x World X position
    @param y World Y position
    @param text Text to display
    @param color {r, g, b} color table (optional, defaults to white)
    @param lifetime Duration in seconds (optional, defaults to 1.5)
    @return Popup ID
]]--
function PopupManager.ShowText(x, y, text, color, lifetime)
    color = color or {r = 1.0, g = 1.0, b = 1.0}
    lifetime = lifetime or 1.5

    local popup = {
        id = nextPopupID,
        type = PopupType.TEXT,
        x = x,
        y = y,
        startY = y,
        text = text,
        lifetime = lifetime,
        elapsed = 0.0,
        floatSpeed = 0.2,
        fadeDelay = lifetime * 0.4,
        color = color
    }

    table.insert(activePopups, popup)
    nextPopupID = nextPopupID + 1

    print("[PopupManager] Text popup created: '" .. text .. "' at (" .. x .. ", " .. y .. ")")
    return popup.id
end

--[[
    ShowStatusEffect(x, y, statusName)
    Shows a status effect notification

    @param x World X position
    @param y World Y position
    @param statusName Name of status (e.g., "Poisoned", "Stunned")
    @return Popup ID
]]--
function PopupManager.ShowStatusEffect(x, y, statusName)
    local statusColors = {
        ["Poisoned"] = {r = 0.5, g = 1.0, b = 0.2},
        ["Stunned"] = {r = 1.0, g = 1.0, b = 0.3},
        ["Burning"] = {r = 1.0, g = 0.5, b = 0.0},
        ["Frozen"] = {r = 0.3, g = 0.8, b = 1.0}
    }

    local color = statusColors[statusName] or {r = 1.0, g = 1.0, b = 1.0}
    return PopupManager.ShowText(x, y, statusName, color, 2.0)
end

-- ============================================================================
-- UPDATE & RENDERING
-- ============================================================================

--[[
    Update(dt)
    Updates all active popups, removing expired ones

    @param dt Delta time in seconds
]]--
function PopupManager.Update(dt)
    -- Update all popups
    for i = #activePopups, 1, -1 do
        local popup = activePopups[i]
        popup.elapsed = popup.elapsed + dt

        -- Update position (float upward)
        popup.y = popup.startY + (popup.floatSpeed * popup.elapsed)

        -- Remove expired popups
        if popup.elapsed >= popup.lifetime then
            table.remove(activePopups, i)
        end
    end
end

--[[
    Draw()
    Renders all active popups
    Call this in your level's OnDraw() function
]]--
function PopupManager.Draw()
    for _, popup in ipairs(activePopups) do
        -- Calculate alpha (fade out near end of lifetime)
        local alpha = 1.0
        if popup.elapsed > popup.fadeDelay then
            local fadeTime = popup.lifetime - popup.fadeDelay
            local timeSinceFade = popup.elapsed - popup.fadeDelay
            alpha = 1.0 - (timeSinceFade / fadeTime)
        end

        -- Draw based on popup type
        if popup.type == PopupType.DAMAGE_NUMBER or
           popup.type == PopupType.HEAL_NUMBER or
           popup.type == PopupType.TEXT then

            -- Draw text with color and alpha
            if DrawText then
                DrawText(
                    "Sans48",           -- Font name
                    popup.text,         -- Text content
                    popup.x,            -- X position
                    popup.y,            -- Y position
                    popup.color.r,      -- Red
                    popup.color.g,      -- Green
                    popup.color.b,      -- Blue
                    alpha               -- Alpha
                )
            end
        end
    end
end

--[[
    Clear()
    Removes all active popups
]]--
function PopupManager.Clear()
    activePopups = {}
    print("[PopupManager] All popups cleared")
end

--[[
    GetActiveCount()
    Returns number of active popups

    @return Number of active popups
]]--
function PopupManager.GetActiveCount()
    return #activePopups
end

-- ============================================================================
-- EXPORT
-- ============================================================================

return PopupManager
