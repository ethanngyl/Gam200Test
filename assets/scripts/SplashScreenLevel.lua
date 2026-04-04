-- ============================================================================
-- SplashScreenLevel.lua
-- Author:        GE YONGQI
-- Email:         yongqi.ge@digipen.edu
-- Date:          2026-04-04
-- Contribution:  100%
-- ----------------------------------------------------------------------------
--  DigiPen Splash Screen
--
--  Purpose:
--  Displays the DigiPen logo for ~3 seconds on a black background before
--  transitioning to the main menu. The logo fades in, holds at full opacity,
--  then fades out.
--
--  Timing:
--    0.0 - 0.6s  : Fade in
--    0.6 - 2.3s  : Hold at full opacity
--    2.3 - 3.0s  : Fade out
--    3.0s+       : Transition to mainMenu
--
--  Level Lifecycle:
--    OnInit()      : Spawn black background + DigiPen logo (alpha=0)
--    OnUpdate(dt)  : Drive fade timer and set logo alpha each frame
--    OnDraw()      : (empty)
--    OnDestroy()   : Destroy spawned entities
-- ============================================================================

-- ============================================================================
-- STATE VARIABLES
-- ============================================================================

local logoID        = 0
local timer         = 0.0
local transitioned  = false

-- Timing constants (seconds)
local FADE_IN_TIME  = 0.6
local HOLD_TIME     = 1.7
local FADE_OUT_TIME = 0.7
local TOTAL_TIME    = FADE_IN_TIME + HOLD_TIME + FADE_OUT_TIME   -- 3.0 s

-- ============================================================================
-- LEVEL LIFECYCLE: OnInit
-- ============================================================================

function OnInit()
    Log("[SplashScreen] Initializing DigiPen splash screen")

    -- Disable editor overlay and enter play state
    if DisableImGui then DisableImGui() end
    if SetEnginePlayState then SetEnginePlayState(true) end

    -- Centre camera, neutral zoom so UI coords match world coords
    if SetCameraPosition then SetCameraPosition(0.0, 0.0, 0.0) end
    if SetCameraZoom then SetCameraZoom(1.0) end

    -- -------------------------------------------------------------------------
    -- DigiPen logo — fit-by-width, starts fully transparent.
    -- Image native size: 1525x445 px → aspect ratio 3.427:1
    -- Viewport at zoom=1, camera_viewport_height=10: width ≈ 17.78 units
    -- Scale width to fill the full viewport width; height preserves aspect ratio
    -- so the complete logo is always visible, centered vertically.
    -- -------------------------------------------------------------------------
    logoID = SpawnSprite(
        "assets/DigiPen_BLACK.png",
        0.0, 0.0,
        2.5,1.5,
        1
    )
    if logoID and logoID > 0 then
        SetSpriteColor(logoID, 1.0, 1.0, 1.0, 0.0)   -- invisible at start
        Log("[SplashScreen] Logo sprite spawned (ID: " .. logoID .. ")")
    else
        Log("[SplashScreen] WARNING: Failed to spawn logo sprite")
    end

    timer        = 0.0
    transitioned = false
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    if transitioned then return end

    timer = timer + dt

    -- Compute alpha based on current phase
    local alpha
    if timer < FADE_IN_TIME then
        -- Phase 1: fade in
        alpha = timer / FADE_IN_TIME
    elseif timer < FADE_IN_TIME + HOLD_TIME then
        -- Phase 2: hold
        alpha = 1.0
    elseif timer < TOTAL_TIME then
        -- Phase 3: fade out
        local fadeProgress = (timer - FADE_IN_TIME - HOLD_TIME) / FADE_OUT_TIME
        alpha = 1.0 - fadeProgress
    else
        alpha = 0.0
    end

    alpha = math.max(0.0, math.min(1.0, alpha))

    if logoID and logoID > 0 then
        SetSpriteColor(logoID, 1.0, 1.0, 1.0, alpha)
    end

    -- Transition once the full sequence is complete
    if timer >= TOTAL_TIME and not transitioned then
        transitioned = true
        Log("[SplashScreen] Splash complete - transitioning to main menu")
        SetNextGameState("mainmenu")
    end
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDraw
-- ============================================================================

function OnDraw()
    -- Nothing to draw manually; sprites are rendered by the engine.
end

-- ============================================================================
-- LEVEL LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    Log("[SplashScreen] Cleaning up splash screen entities")

    if logoID and logoID > 0 then
        DestroyEntity(logoID)
        logoID = 0
    end
end
