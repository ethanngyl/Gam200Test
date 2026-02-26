-- ===============================================================================
-- File:          EnemyTurnManager.lua
-- Author:        Ethan Ng Yong Le 
-- Email:         n.ethanyongle@digipen.edu
-- Date:          2026-02-01
-- ------------------------------------------------------------------------------
-- Enemy Turn Manager

-- Purpose:
--    Manages sequential enemy turns so enemies act one at a time instead of
--    all simultaneously. This allows for:
--    - Camera panning to each enemy as they act
--    - Visual feedback of each enemy's movement
--    - Better game flow and readability

-- Turn Flow:
--    Enemy 1 -> Enemy 2 -> Enemy 3 -> ... -> Back to Player

-- Features:
--    - Sequential enemy turns (one at a time)
--    - Camera panning to active enemy
--    - Delay between enemy actions for visibility
--    - Tracks which enemy is currently acting

--===============================================================================


-- ============================================================================
-- ENEMY TURN STATE
-- ============================================================================

-- Enemy turn configuration
ActiveEnemyIndex = 0        -- Index in enemy list (0 = none)
EnemyTurnActive = false     -- True when in enemy turn phase
EnemyActionDelay = 0.5      -- Delay in seconds between enemy actions
EnemyActionTimer = 0.0      -- Current action timer

-- Safety timeout: auto-advance if an enemy gets stuck (e.g. no path, script error)
EnemyStuckTimeout = 10.0    -- Max seconds an enemy can be active before auto-advancing
EnemyStuckTimer = 0.0       -- Tracks how long current enemy has been active

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function InitializeEnemyTurn()
    print("[EnemyTurnManager] Initializing enemy turn")

    local enemies = GetAllEnemies()
    if not enemies or #enemies == 0 then
        print("[EnemyTurnManager] No enemies found!")
        EnemyTurnActive = false
        return false
    end

    print("[EnemyTurnManager] Found " .. #enemies .. " enemies")

    ActiveEnemyIndex = 1
    EnemyTurnActive = true
    EnemyActionTimer = 0.0  -- Start immediately
    EnemyStuckTimer = 0.0   -- Reset stuck timer

    local firstEnemy = enemies[1]
    print("[EnemyTurnManager] Starting with enemy " .. firstEnemy)

    -- CRITICAL: Pan camera to first enemy
    SetCameraFollowTarget(firstEnemy)
    print("[EnemyTurnManager] Camera now following Enemy " .. firstEnemy)

    return true
end

-- ============================================================================
-- QUERY FUNCTIONS
-- ============================================================================

function GetActiveEnemy()
    if not EnemyTurnActive or ActiveEnemyIndex == 0 then
        print("[EnemyTurnManager] GetActiveEnemy(): 0 (EnemyTurnActive=" .. tostring(EnemyTurnActive) .. ", ActiveEnemyIndex=" .. ActiveEnemyIndex .. ")")
        return 0
    end

    local enemies = GetAllEnemies()
    if not enemies or ActiveEnemyIndex > #enemies then
        print("[EnemyTurnManager] GetActiveEnemy(): 0 (invalid index " .. ActiveEnemyIndex .. ", enemies=" .. tostring(enemies and #enemies or "nil") .. ")")
        return 0
    end

    local activeID = enemies[ActiveEnemyIndex]
    print("[EnemyTurnManager] GetActiveEnemy(): " .. activeID .. " (index " .. ActiveEnemyIndex .. "/" .. #enemies .. ")")
    return activeID
end

function IsActiveEnemy(entityID)
    -- If not in Enemy turn phase, no enemy is active
    local currentTurn = GetCurrentTurn()
    if currentTurn ~= "Enemy" then
        print("[EnemyTurnManager] IsActiveEnemy(" .. entityID .. "): false (turn is " .. tostring(currentTurn) .. ")")
        return false
    end

    -- Check if this enemy is the currently active one
    local activeEnemyID = GetActiveEnemy()
    local isActive = (entityID == activeEnemyID)
    print("[EnemyTurnManager] IsActiveEnemy(" .. entityID .. "): " .. tostring(isActive) .. " (active=" .. tostring(activeEnemyID) .. ", index=" .. ActiveEnemyIndex .. ")")
    return isActive
end

function GetEnemyTurnProgress()
    if not EnemyTurnActive then
        return 0, 0
    end

    local enemies = GetAllEnemies()
    if not enemies then
        return 0, 0
    end

    return ActiveEnemyIndex, #enemies
end

-- ============================================================================
-- TURN ADVANCEMENT
-- ============================================================================

function NextEnemyTurn()
    if not EnemyTurnActive then
        print("[EnemyTurnManager] ERROR: NextEnemyTurn called but turn not active")
        return
    end

    local enemies = GetAllEnemies()
    if not enemies or #enemies == 0 then
        print("[EnemyTurnManager] No enemies found, ending enemy turn")
        EndAllEnemyTurns()
        return
    end

    print("[EnemyTurnManager] ========== ENEMY TURN ADVANCEMENT ==========")
    print(string.format("[EnemyTurnManager] Current: Enemy %d (Index %d/%d)",
        enemies[ActiveEnemyIndex],
        ActiveEnemyIndex,
        #enemies))

    -- Move to next enemy
    ActiveEnemyIndex = ActiveEnemyIndex + 1

    -- Check if all enemies have acted
    if ActiveEnemyIndex > #enemies then
        print("[EnemyTurnManager] ======================================")
        print("[EnemyTurnManager] ALL ENEMIES HAVE ACTED!")
        print("[EnemyTurnManager] ======================================")
        EndAllEnemyTurns()
        return
    end

    -- Switch to new active enemy
    local newActiveEnemy = enemies[ActiveEnemyIndex]

    print(string.format("[EnemyTurnManager] Switching to: Enemy %d (Index %d/%d)",
        newActiveEnemy,
        ActiveEnemyIndex,
        #enemies))

    -- Pan camera to new active enemy
    SetCameraFollowTarget(newActiveEnemy)
    print("[EnemyTurnManager] Camera now following Enemy " .. newActiveEnemy)

    -- Reset action timer for next enemy
    EnemyActionTimer = EnemyActionDelay
    EnemyStuckTimer = 0.0  -- Reset stuck timer for new enemy

    print("[EnemyTurnManager] ======================================")
end

function EndAllEnemyTurns()
    print("============================================================")
    print("[EnemyTurnManager] !!! EndAllEnemyTurns() CALLED !!!")
    print("============================================================")

    EnemyTurnActive = false
    ActiveEnemyIndex = 0

    -- Pan camera back to active player
    local activePlayerID = GetActiveCharacter()
    if activePlayerID and activePlayerID > 0 then
        SetCameraFollowTarget(activePlayerID)
        print("[EnemyTurnManager] Camera now following Player " .. activePlayerID)
    end

    -- Switch back to player turn
    EndEnemyTurn()

    -- CRITICAL: Reset party turn immediately
    -- Normally OnEnemyTurnEnded() would be called by the level script on next frame,
    -- but we need to reset the party NOW to avoid being stuck in a stale state
    if ResetPartyTurn then
        print("[EnemyTurnManager] Immediately resetting party turn...")
        ResetPartyTurn()
    else
        print("[EnemyTurnManager] WARNING: ResetPartyTurn not found!")
    end

    print("[EnemyTurnManager] EndAllEnemyTurns() COMPLETE")
    print("============================================================")
end

-- ============================================================================
-- UPDATE / TIMER MANAGEMENT
-- ============================================================================

function UpdateEnemyTurnManager(dt)
    if not EnemyTurnActive then
        return
    end

    -- Check if all enemies are dead - automatically end enemy turn
    local enemies = GetAllEnemies()
    if not enemies or #enemies == 0 then
        print("[EnemyTurnManager] ========================================")
        print("[EnemyTurnManager] ALL ENEMIES DEAD - Auto-ending enemy turn!")
        print("[EnemyTurnManager] ========================================")
        SetNextGameState("LEVEL_END")
        --EndAllEnemyTurns()
        return
    end

    -- Update action timer
    if EnemyActionTimer > 0 then
        local oldTimer = EnemyActionTimer
        EnemyActionTimer = EnemyActionTimer - dt
        if EnemyActionTimer < 0 then
            EnemyActionTimer = 0
        end
        if EnemyActionTimer == 0 then
            print("[EnemyTurnManager] Action timer ready! (was " .. string.format("%.3f", oldTimer) .. ", now 0.0)")
        end
    end

    -- Safety timeout: auto-advance if an enemy is stuck too long
    EnemyStuckTimer = EnemyStuckTimer + dt
    if EnemyStuckTimer >= EnemyStuckTimeout then
        local activeEnemy = GetActiveEnemy()
        print("[EnemyTurnManager] ========================================")
        print("[EnemyTurnManager] SAFETY TIMEOUT: Enemy " .. tostring(activeEnemy) .. " stuck for " .. string.format("%.1f", EnemyStuckTimer) .. "s!")
        print("[EnemyTurnManager] Auto-advancing to next enemy...")
        print("[EnemyTurnManager] ========================================")
        EnemyStuckTimer = 0.0
        NextEnemyTurn()
    end
end

function IsEnemyActionReady()
    local ready = EnemyTurnActive and EnemyActionTimer <= 0
    print("[EnemyTurnManager] IsEnemyActionReady(): " .. tostring(ready) .. " (active=" .. tostring(EnemyTurnActive) .. ", timer=" .. EnemyActionTimer .. ")")
    return ready
end

function MarkEnemyActionComplete()
    print("[EnemyTurnManager] Enemy action complete, advancing to next enemy")
    NextEnemyTurn()
end

-- ============================================================================
-- GLOBAL EXPORTS
-- ============================================================================

_G.InitializeEnemyTurn = InitializeEnemyTurn
_G.GetActiveEnemy = GetActiveEnemy
_G.IsActiveEnemy = IsActiveEnemy
_G.GetEnemyTurnProgress = GetEnemyTurnProgress
_G.NextEnemyTurn = NextEnemyTurn
_G.EndAllEnemyTurns = EndAllEnemyTurns
_G.UpdateEnemyTurnManager = UpdateEnemyTurnManager
_G.IsEnemyActionReady = IsEnemyActionReady
_G.MarkEnemyActionComplete = MarkEnemyActionComplete

print("============================================================")
print("========== EnemyTurnManager.lua LOADED SUCCESSFULLY ==========")
print("============================================================")

Log("[EnemyTurnManager] Loaded successfully")
