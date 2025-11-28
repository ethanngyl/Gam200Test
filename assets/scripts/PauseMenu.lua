--[[
===============================================================================
 File:          PauseMenu.lua
 Author:        Claude (AI Assistant)
 Date:          2025-11-28
 ------------------------------------------------------------------------------
  Reusable Pause Menu System for Lua Levels

  Usage:
    local PauseMenu = require("PauseMenu")

    -- In OnInit:
    PauseMenu.Init()

    -- In OnUpdate:
    PauseMenu.Update(dt)

    -- In OnDraw:
    PauseMenu.Draw()
===============================================================================
--]]

local PauseMenu = {}

-- ============================================================================
-- STATE
-- ============================================================================

local state = {
    selectedOption = 0,  -- 0=Resume, 1=MainMenu, 2=Exit
    wasUpPressed = false,
    wasDownPressed = false,
    wasEnterPressed = false,
    wasPPressed = false,
    wasEscapePressed = false
}

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function PauseMenu.Init()
    state.selectedOption = 0
    state.wasUpPressed = false
    state.wasDownPressed = false
    state.wasEnterPressed = false
    state.wasPPressed = false
    state.wasEscapePressed = false
    Log("PauseMenu initialized")
end

-- ============================================================================
-- UPDATE (HANDLES INPUT)
-- ============================================================================

function PauseMenu.Update(dt)
    -- Toggle pause with P or Escape key
    local isPPressed = IsKeyDown("P")
    local isEscapePressed = IsKeyDown("Escape")

    if (isPPressed and not state.wasPPressed) or (isEscapePressed and not state.wasEscapePressed) then
        TogglePause()

        if IsPaused() then
            Log("Game PAUSED")
            SetMasterVolume(0.0)
            state.selectedOption = 0  -- Reset to Resume option
        else
            Log("Game RESUMED")
            SetMasterVolume(1.0)
        end
    end

    state.wasPPressed = isPPressed
    state.wasEscapePressed = isEscapePressed

    -- If not paused, don't handle menu input
    if not IsPaused() then
        return
    end

    -- ========================================================================
    -- PAUSE MENU INPUT
    -- ========================================================================

    -- Arrow key / WASD navigation
    local isUpPressed = IsKeyDown("Up") or IsKeyDown("W")
    local isDownPressed = IsKeyDown("Down") or IsKeyDown("S")
    local isEnterPressed = IsKeyDown("Enter") or IsKeyDown("Space")

    -- Edge detection for navigation
    if isUpPressed and not state.wasUpPressed then
        state.selectedOption = state.selectedOption - 1
        if state.selectedOption < 0 then
            state.selectedOption = 2
        end
    end

    if isDownPressed and not state.wasDownPressed then
        state.selectedOption = state.selectedOption + 1
        if state.selectedOption > 2 then
            state.selectedOption = 0
        end
    end

    state.wasUpPressed = isUpPressed
    state.wasDownPressed = isDownPressed

    -- Number key shortcuts (1=Resume, 2=MainMenu, 3=Exit)
    if IsKeyDown("1") then
        PauseMenu.OnResume()
        return
    elseif IsKeyDown("2") then
        PauseMenu.OnMainMenu()
        return
    elseif IsKeyDown("3") then
        PauseMenu.OnExit()
        return
    end

    -- Enter/Space to select
    if isEnterPressed and not state.wasEnterPressed then
        if state.selectedOption == 0 then
            PauseMenu.OnResume()
        elseif state.selectedOption == 1 then
            PauseMenu.OnMainMenu()
        elseif state.selectedOption == 2 then
            PauseMenu.OnExit()
        end
    end

    state.wasEnterPressed = isEnterPressed
end

-- ============================================================================
-- DRAW (RENDERS PAUSE MENU)
-- ============================================================================

function PauseMenu.Draw()
    if not IsPaused() then
        return
    end

    -- Get framebuffer size for centering
    local fbWidth, fbHeight = GetFramebufferSize()

    local centerX = fbWidth * 0.5
    local centerY = fbHeight * 0.5

    -- ========================================================================
    -- DRAW TITLE
    -- ========================================================================
    local titleX = centerX - 120
    local titleY = centerY + 200
    local titleScale = 2.5

    DrawText("Sans48", "PAUSED", titleX, titleY, titleScale, 1.0, 1.0, 0.3)

    -- ========================================================================
    -- DRAW MENU OPTIONS
    -- ========================================================================
    local menuStartY = centerY + 80
    local menuSpacing = 70
    local textBaseX = centerX - 100

    local options = {"Resume", "Main Menu", "Exit Game"}

    for i = 0, 2 do
        local isSelected = (state.selectedOption == i)

        -- Add selection markers
        local text = options[i + 1]  -- Lua arrays start at 1
        if isSelected then
            text = "> " .. text .. " <"
        else
            text = "  " .. text
        end

        local scale = isSelected and 1.3 or 1.2
        local r, g, b = 1.0, 1.0, 0.3  -- Yellow for selected
        if not isSelected then
            r, g, b = 0.7, 0.7, 0.7  -- Gray for unselected
        end

        DrawText("Sans48", text, textBaseX, menuStartY - (i * menuSpacing), scale, r, g, b)
    end

    -- ========================================================================
    -- DRAW CONTROL HINTS
    -- ========================================================================
    local hint1X = centerX - 280
    local hint1Y = centerY - 150
    local hint2X = centerX - 380
    local hint2Y = centerY - 200
    local hintScale = 0.75

    DrawText("Sans48", "W/S or Arrow Keys to Navigate",
             hint1X, hint1Y, hintScale, 1.0, 1.0, 0.3)

    DrawText("Sans48", "Enter/Space to Select | 1-3 for Quick Select",
             hint2X, hint2Y, hintScale, 1.0, 1.0, 0.3)
end

-- ============================================================================
-- CALLBACKS
-- ============================================================================

function PauseMenu.OnResume()
    TogglePause()  -- Unpause
    SetMasterVolume(1.0)
    Log("Resume selected")
end

function PauseMenu.OnMainMenu()
    TogglePause()  -- Unpause before changing state
    SetMasterVolume(1.0)
    SetNextGameState("mainMenu")
    Log("Returning to main menu")
end

function PauseMenu.OnExit()
    SetNextGameState("GS_QUIT")
    Log("Exiting game")
end

-- ============================================================================
-- RETURN MODULE
-- ============================================================================

return PauseMenu
