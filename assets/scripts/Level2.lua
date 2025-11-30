-- ============================================================================
-- Level2.lua - ImGui Editor Showcase 
-- Author:        Padilla Carl Jameson
-- Date:          11/30/2025
-- Contribution:  100%
--
-- Description:
-- A test level that demonstrates the ImGui editor functionality. Starts in
-- editor mode by default with camera controls enabled. Allows toggling between
-- editor and play modes using F1. Implements safe level transitions that
-- properly exit editor mode before switching scenes to prevent state conflicts.
-- Supports quick navigation to main menu (key 5) and Level 3 (key 3).
-- ============================================================================

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local initialized = false
local editorToggleCooldown = 0

-- ============================================================================
-- HELPER FUNCTIONS
-- ============================================================================

--  Helper function: Safe level transition
-- Ensures editor mode is exited before switching levels
local function SafeSwitchLevel(targetLevel, levelName)
    Log(string.format("Switching to %s...", levelName))
    
    -- Exit editor mode if active
    if IsEditorMode() then
        Log("⚠ Currently in editor mode - exiting before level switch")
        ToggleEditorMode()          -- Exit editor mode
        SetEnginePlayState(true)    -- Restore game state
        Log(" Editor mode exited")
    end
    
    -- Switch to target level
    SetNextGameState(targetLevel)
    Log(string.format(" Switching to %s", levelName))
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

--- Initializes Level 2 with ImGui editor enabled.
function OnInit()
    Log("========================================")
    Log("LEVEL 2: ImGui Editor Showcase")
    Log("========================================")
    
    -- Set camera to default position
    SetCameraPosition(0.0, 0.0, 0.0)
    SetCameraZoom(1.0)
    Log("Camera initialized: pos(0,0,0), zoom=1.0")
    
    --  Enable ImGui first
    EnableImGui()
    Log(" ImGui enabled")
    
    --  Enter editor mode
    if not IsEditorMode() then
        ToggleEditorMode()
        Log(" Editor mode toggled ON")
    end
    
    --  Set engine to non-playing state (editor camera)
    SetEnginePlayState(false)
    Log(" Engine play state: false (editor camera)")
    
    -- Load animations
    LoadAnimationConfig("assets/animations.json")
    Log("Loaded animations.json for editor")
    
    initialized = true
    Log("Level 2 initialization complete")
    Log("Editor mode active - Press F1 to toggle")
    Log("Press 5 for Main Menu | Press 3 for Level 3")
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnUpdate
-- ============================================================================

--- Handles input and updates level state each frame.
function OnUpdate(dt)
    -- Update audio system
    UpdateAudio(dt)
    
    -- F1: Toggle editor mode
    if IsKeyDown("F1") then
        if editorToggleCooldown <= 0 then
            ToggleEditorMode()
            
            if IsEditorMode() then
                SetEnginePlayState(false)
                Log("EDITOR MODE ON")
            else
                SetEnginePlayState(true)
                Log("EDITOR MODE OFF")
            end
            
            editorToggleCooldown = 0.3
        end
    end
    
    if editorToggleCooldown > 0 then
        editorToggleCooldown = editorToggleCooldown - dt
    end
    
    -- ========================================================================
    --  KEY_5: Return to main menu (safe transition)
    -- ========================================================================
    if IsKeyDown("5") then
        SafeSwitchLevel("mainMenu", "Main Menu")
    end
    
    -- ========================================================================
    --  KEY_3: Go to Level 3 (safe transition)
    -- ========================================================================
    if IsKeyDown("3") then
        SafeSwitchLevel("LEVEL_3", "Level 3")
    end
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDraw
-- ============================================================================

--- Renders UI text and editor mode indicator.
function OnDraw()
    -- Display editor mode indicator
    if IsEditorMode() then
        DrawText("Sans48", "EDITOR MODE", 50, 50, 0.8, 1.0, 0.3, 0.3)
    end
    
    -- Display helper text
    DrawText("Sans32", "ImGui Editor Showcase", 50, 100, 0.6, 0.8, 0.8, 0.8)
    DrawText("Sans24", "Press F1 to toggle editor", 50, 140, 0.5, 0.6, 0.6, 0.6)
    DrawText("Sans24", "Press 5 for Main Menu", 50, 170, 0.5, 0.6, 0.6, 0.6)
    DrawText("Sans24", "Press 3 for Level 3", 50, 200, 0.5, 0.6, 0.6, 0.6)
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDestroy
-- ============================================================================

--- Cleans up resources and exits editor mode.
function OnDestroy()
    Log("========================================")
    Log("Level 2 cleanup...")
    Log("========================================")
    
    -- Reset state
    editorToggleCooldown = 0
    
    -- Exit editor mode if active
    if IsEditorMode() then
        SetEditorMode(false)
        Log("Exited editor mode")
    end
    
    -- Disable ImGui
    DisableImGui()
    Log("ImGui disabled")
    ClearAllEntities()

    initialized = false
    Log("Level 2 cleanup complete")
end