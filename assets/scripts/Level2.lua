-- ============================================================================
-- Level2.lua - ImGui Editor Showcase (FIXED v2)
-- ============================================================================
-- Purpose:
-- - Showcase ImGui editor functionality
-- - Demonstrate editor mode with F1 toggle
-- - Simple level for testing ImGui features
-- 
-- FIX: Properly initialize editor mode on startup
-- ============================================================================

-- ============================================================================
-- LEVEL STATE VARIABLES
-- ============================================================================

local initialized = false
local editorToggleCooldown = 0

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("========================================")
    Log("LEVEL 2: ImGui Editor Showcase")
    Log("========================================")
    
    -- Set camera to default position
    SetCameraPosition(0.0, 0.0, 0.0)
    SetCameraZoom(1.0)
    Log("Camera initialized: pos(0,0,0), zoom=1.0")
    
    -- ✅ FIX 1: Enable ImGui first
    EnableImGui()
    Log("✓ ImGui enabled")
    
    -- ✅ FIX 2: Set editor mode BEFORE SetEnginePlayState
    -- This ensures the engine knows we're in editor mode
    if not IsEditorMode() then
        ToggleEditorMode()  -- Use toggle to ensure proper state
        Log("✓ Editor mode toggled ON")
    end
    
    -- ✅ FIX 3: Set engine to non-playing state (editor camera)
    SetEnginePlayState(false)  -- false = Editor mode (not playing)
    Log("✓ Engine play state: false (editor camera)")
    
    -- Load animations (for editor animation testing)
    LoadAnimationConfig("assets/animations.json")
    Log("Loaded animations.json for editor")
    
    initialized = true
    Log("Level 2 initialization complete")
    Log("Editor mode should be active now")
    Log("Press F1 to toggle editor mode")
    Log("Press 5 to return to main menu")
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    -- Update audio system
    UpdateAudio(dt)
    
    -- ✅ F1 toggles editor mode (same as other levels)
    if IsKeyDown("F1") then
        if editorToggleCooldown <= 0 then
            ToggleEditorMode()
            
            if IsEditorMode() then
                SetEnginePlayState(false)
                Log("EDITOR MODE ON - ImGui enabled")
            else
                SetEnginePlayState(true)
                Log("EDITOR MODE OFF - ImGui disabled")
            end
            
            editorToggleCooldown = 0.3
        end
    end
    
    if editorToggleCooldown > 0 then
        editorToggleCooldown = editorToggleCooldown - dt
    end
    
    -- Return to main menu with KEY_5
    if IsKeyDown("5") then
        Log("KEY_5 pressed - returning to main menu")
        SetNextGameState("mainMenu")
    end
    
    -- Go to Level 3 with KEY_3
    if IsKeyDown("3") then
        Log("KEY_3 pressed - going to Level 3")
        SetNextGameState("LEVEL_3")
    end
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDraw
-- ============================================================================

function OnDraw()
    -- ✅ Display editor mode indicator (same as other levels)
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

function OnDestroy()
    Log("========================================")
    Log("Level 2 cleanup...")
    Log("========================================")
    
    -- Reset editor mode state
    editorToggleCooldown = 0
    
    -- Exit editor mode if active
    if IsEditorMode() then
        SetEditorMode(false)
        Log("Exited editor mode")
    end
    
    -- Disable ImGui
    DisableImGui()
    Log("ImGui disabled")
    
    -- Reset state
    initialized = false
    
    Log("Level 2 cleanup complete")
end