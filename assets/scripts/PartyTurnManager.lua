print("============================================================")
print("========== PartyTurnManager.lua LOADING START ==========")
print("============================================================")

--[[
===============================================================================
 File:           PartyTurnManager.lua
 Author:         Auto-generated for party system implementation
 Date:           2026-01-16
 ------------------------------------------------------------------------------
 Party Turn Manager

 Purpose:
    Manages turn sequencing for a 3-character party system where each
    character acts individually before the enemy turn begins.

 Turn Flow:
    Character 1 -> Character 2 -> Character 3 -> Enemy Phase -> Repeat

 Features:
    - Sequential character turns (not simultaneous)
    - Tracks which character is currently active
    - Prevents all characters from responding to input at once
    - Integrates with existing turn system (Player/Enemy phases)
    - Camera switching between characters (optional)

 Usage:
    -- In level initialization:
    InitializeParty({warrior, mage, rogue})

    -- In character scripts:
    if not IsActiveCharacter(entityID) then
        return  -- Not my turn
    end

    -- To advance turns:
    NextCharacterTurn()  -- Go to next party member

    -- When character finishes turn:
    EndCharacterTurn()  -- Marks character done, advances if last

===============================================================================
]]--

-- ============================================================================
-- PARTY STATE
-- ============================================================================

-- Party configuration
PartyMembers = {}           -- Array of {entityID, name, hasActed}
ActiveCharacterIndex = 1    -- Index in PartyMembers (1-3)
PartyTurnComplete = false   -- True when all 3 characters have acted

-- Turn transition cooldown (prevents input carry-over between characters)
TurnTransitionCooldown = 0.0         -- Current cooldown timer
TurnTransitionCooldownTime = 0.0     -- Delay in seconds after turn switch (0 = instant with per-key blocking)

-- Character definitions (can be customized)
CharacterConfig = {
    {
        name = "Warrior",
        maxHP = 10,
        maxAP = 3,
        role = "Tank"
    },
    {
        name = "Mage",
        maxHP = 5,
        maxAP = 5,
        role = "DPS"
    },
    {
        name = "Rogue",
        maxHP = 7,
        maxAP = 4,
        role = "Balanced"
    }
}

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

--[[
    InitializeParty(entityIDs)
    Sets up the party with the given entity IDs

    @param entityIDs Table of entity IDs {warrior, mage, rogue}
]]--
function InitializeParty(entityIDs)
    print("[InitializeParty] Called with entityIDs table")

    if not entityIDs then
        print("[InitializeParty] ERROR: entityIDs is nil!")
        return false
    end

    print("[InitializeParty] entityIDs has " .. #entityIDs .. " entries")

    if #entityIDs ~= 3 then
        print("[InitializeParty] ERROR: Need exactly 3 entity IDs, got " .. #entityIDs)
        return false
    end

    print("[InitializeParty] Step 1: Clearing PartyMembers...")
    PartyMembers = {}
    print("[InitializeParty] Step 1: DONE")

    print("[InitializeParty] Step 2: Populating PartyMembers...")
    for i = 1, 3 do
        print("[InitializeParty]   Creating member " .. i .. " for Entity " .. entityIDs[i])
        PartyMembers[i] = {
            entityID = entityIDs[i],
            name = CharacterConfig[i].name,
            role = CharacterConfig[i].role,
            hasActed = false,
            turnIndex = i
        }
        print("[InitializeParty]   Member " .. i .. " created: " .. PartyMembers[i].name .. " (" .. PartyMembers[i].role .. ")")
    end
    print("[InitializeParty] Step 2: DONE - PartyMembers has " .. #PartyMembers .. " entries")

    print("[InitializeParty] Step 3: Setting initial state...")
    ActiveCharacterIndex = 1
    PartyTurnComplete = false
    print("[InitializeParty] Step 3: DONE")

    print("[InitializeParty] Step 4: Calling SetActiveCharacter(" .. PartyMembers[1].entityID .. ")...")
    SetActiveCharacter(PartyMembers[1].entityID)
    print("[InitializeParty] Step 4: DONE")

    print("[InitializeParty] Step 5: Refilling AP for all characters...")
    for i = 1, 3 do
        print("[InitializeParty]   Refilling AP for character " .. i .. " (Entity " .. PartyMembers[i].entityID .. ")...")
        RefillEntityAP(PartyMembers[i].entityID)
        print("[InitializeParty]   Done")
    end
    print("[InitializeParty] Step 5: DONE")

    print("[InitializeParty] Step 6: Checking AP/HP for all characters...")
    for i = 1, 3 do
        local currentAP, maxAP = GetEntityAP(PartyMembers[i].entityID)
        print("[InitializeParty]   " .. i .. ". " .. PartyMembers[i].name .. " (Entity " .. PartyMembers[i].entityID .. ") - AP: " .. tostring(currentAP) .. "/" .. tostring(maxAP))
    end
    print("[InitializeParty] Step 6: DONE")

    print("[InitializeParty] COMPLETED SUCCESSFULLY - Active character: " .. PartyMembers[ActiveCharacterIndex].name)
    return true
end

-- ============================================================================
-- QUERY FUNCTIONS
-- ============================================================================

--[[
    GetActiveCharacter()
    Returns the entity ID of the currently active character

    @return Entity ID of active character, or 0 if no party
]]--
function GetActiveCharacter()
    if #PartyMembers == 0 then
        return 0
    end

    if ActiveCharacterIndex < 1 or ActiveCharacterIndex > #PartyMembers then
        Log("[PartyTurnManager] ERROR: Invalid ActiveCharacterIndex: " .. ActiveCharacterIndex)
        return 0
    end

    return PartyMembers[ActiveCharacterIndex].entityID
end

--[[
    GetActiveCharacterName()
    Returns the name of the currently active character

    @return Character name string, or "Unknown"
]]--
function GetActiveCharacterName()
    if #PartyMembers == 0 then
        return "Unknown"
    end

    if ActiveCharacterIndex < 1 or ActiveCharacterIndex > #PartyMembers then
        return "Unknown"
    end

    return PartyMembers[ActiveCharacterIndex].name
end

--[[
    IsActiveCharacter(entityID)
    Checks if the given entity is currently the active character

    @param entityID Entity ID to check
    @return true if this entity should process input, false otherwise
]]--
function IsActiveCharacter(entityID)
    -- If not in Player turn phase, no character is active
    local currentTurn = GetCurrentTurn()
    if currentTurn ~= "Player" then
        return false
    end

    -- Check if this entity matches the active character
    local activeID = GetActiveCharacter()
    return entityID == activeID
end

--[[
    GetPartyMembers()
    Returns array of all party member entity IDs

    @return {entityID1, entityID2, entityID3}
]]--
function GetPartyMembers()
    -- DEBUG: Log when this function is called
    Log("[PartyTurnManager DEBUG] GetPartyMembers() called - PartyMembers has " .. #PartyMembers .. " entries")

    local ids = {}
    for i = 1, #PartyMembers do
        ids[i] = PartyMembers[i].entityID
        Log("[PartyTurnManager DEBUG]   ids[" .. i .. "] = " .. ids[i])
    end

    Log("[PartyTurnManager DEBUG] GetPartyMembers() returning " .. #ids .. " IDs")
    return ids
end

--[[
    GetPartyMemberInfo(index)
    Returns detailed info about a party member

    @param index Party member index (1-3)
    @return {entityID, name, role, hasActed} or nil
]]--
function GetPartyMemberInfo(index)
    if index < 1 or index > #PartyMembers then
        return nil
    end
    return PartyMembers[index]
end

--[[
    IsPartyTurnComplete()
    Checks if all 3 characters have acted this turn

    @return true if party turn is done, false otherwise
]]--
function IsPartyTurnComplete()
    Log(string.format("[PartyTurnManager DEBUG] IsPartyTurnComplete() called - returning %s (ActiveCharacterIndex=%d, #PartyMembers=%d)",
        tostring(PartyTurnComplete), ActiveCharacterIndex, #PartyMembers))
    return PartyTurnComplete
end

-- ============================================================================
-- TURN ADVANCEMENT
-- ============================================================================

--[[
    NextCharacterTurn()
    Advances to the next party member

    If all 3 have acted, sets PartyTurnComplete flag
    Call EndPartyTurn() to actually switch to enemy phase
]]--
function NextCharacterTurn()
    if #PartyMembers == 0 then
        print("[PartyTurnManager] ERROR: No party initialized")
        return
    end

    print("[PartyTurnManager] ========== TURN ADVANCEMENT ==========")
    print(string.format("[PartyTurnManager] Current: %s (Entity %d, Index %d)",
        PartyMembers[ActiveCharacterIndex].name,
        PartyMembers[ActiveCharacterIndex].entityID,
        ActiveCharacterIndex))

    -- Mark current character as having acted
    PartyMembers[ActiveCharacterIndex].hasActed = true

    print(string.format("[PartyTurnManager] %s marked as ACTED",
        PartyMembers[ActiveCharacterIndex].name))

    -- Move to next character, skipping dead ones
    print(string.format("[PartyTurnManager DEBUG] BEFORE increment: ActiveCharacterIndex = %d, #PartyMembers = %d",
        ActiveCharacterIndex, #PartyMembers))

    ActiveCharacterIndex = ActiveCharacterIndex + 1

    print(string.format("[PartyTurnManager DEBUG] AFTER increment: ActiveCharacterIndex = %d, #PartyMembers = %d",
        ActiveCharacterIndex, #PartyMembers))

    -- Skip dead characters
    local skippedDead = 0
    while ActiveCharacterIndex <= #PartyMembers do
        local checkEntity = PartyMembers[ActiveCharacterIndex].entityID
        local currentHP, maxHP = GetEntityHP(checkEntity)

        if currentHP and currentHP > 0 then
            -- This character is alive, use them
            break
        else
            -- This character is dead, skip to next
            print(string.format("[PartyTurnManager] %s is DEAD (HP: %s), skipping...",
                PartyMembers[ActiveCharacterIndex].name, tostring(currentHP)))
            PartyMembers[ActiveCharacterIndex].hasActed = true  -- Mark as acted so they don't block
            ActiveCharacterIndex = ActiveCharacterIndex + 1
            skippedDead = skippedDead + 1
        end
    end

    if skippedDead > 0 then
        print(string.format("[PartyTurnManager] Skipped %d dead character(s)", skippedDead))
    end

    print(string.format("[PartyTurnManager DEBUG] Check: %d > %d = %s",
        ActiveCharacterIndex, #PartyMembers, tostring(ActiveCharacterIndex > #PartyMembers)))

    -- Check if all characters have acted (or are dead)
    if ActiveCharacterIndex > #PartyMembers then
        PartyTurnComplete = true
        print("[PartyTurnManager] ======================================")
        print("[PartyTurnManager] ALL PARTY MEMBERS HAVE ACTED (or are dead)!")
        print("[PartyTurnManager] PartyTurnComplete = true")
        print("[PartyTurnManager] Waiting for EndPartyTurn() to switch to enemy phase")
        print("[PartyTurnManager] ======================================")
        return
    end

    -- Switch to new active character (guaranteed to be alive at this point)
    local newActiveEntity = PartyMembers[ActiveCharacterIndex].entityID

    print(string.format("[PartyTurnManager] Switching to: %s (Entity %d, Index %d)",
        PartyMembers[ActiveCharacterIndex].name,
        newActiveEntity,
        ActiveCharacterIndex))

    -- Notify C++ about active character change
    SetActiveCharacter(newActiveEntity)

    -- Refill AP for the new active character
    RefillEntityAP(newActiveEntity)
    local currentAP, maxAP = GetEntityAP(newActiveEntity)
    print(string.format("[PartyTurnManager] %s AP refilled to %d/%d",
        PartyMembers[ActiveCharacterIndex].name,
        currentAP,
        maxAP))

    -- Start turn transition cooldown to prevent input carry-over
    TurnTransitionCooldown = TurnTransitionCooldownTime
    print(string.format("[PartyTurnManager] Turn transition cooldown started: %.2fs", TurnTransitionCooldown))

    print("[PartyTurnManager] ======================================")

    -- Optional: Trigger camera switch
    OnCharacterSwitched(newActiveEntity)
end

--[[
    EndCharacterTurn()
    Called when active character finishes their actions
    Automatically advances to next character
]]--
function EndCharacterTurn()
    print("[PartyTurnManager] ==========================================")
    print(string.format("[PartyTurnManager] EndCharacterTurn() called for %s (Entity %d)",
        GetActiveCharacterName(), GetActiveCharacter()))
    print(string.format("[PartyTurnManager DEBUG] Before advancing - ActiveCharacterIndex=%d, PartyTurnComplete=%s",
        ActiveCharacterIndex, tostring(PartyTurnComplete)))
    print("[PartyTurnManager] ==========================================")

    NextCharacterTurn()

    Log("[PartyTurnManager] ==========================================")
    Log(string.format("[PartyTurnManager DEBUG] After advancing - ActiveCharacterIndex=%d, PartyTurnComplete=%s",
        ActiveCharacterIndex, tostring(PartyTurnComplete)))
    Log("[PartyTurnManager] ==========================================")
end

--[[
    EndPartyTurn()
    Called when all 3 characters have acted
    Switches to enemy turn phase
]]--
function EndPartyTurn()
    print("============================================================")
    print("[PartyTurnManager] !!! EndPartyTurn() CALLED !!!")
    print(string.format("[PartyTurnManager DEBUG] PartyTurnComplete=%s, ActiveCharacterIndex=%d, #PartyMembers=%d",
        tostring(PartyTurnComplete), ActiveCharacterIndex, #PartyMembers))

    -- Log which characters have acted
    for i = 1, #PartyMembers do
        print(string.format("[PartyTurnManager DEBUG] %s: hasActed=%s",
            PartyMembers[i].name, tostring(PartyMembers[i].hasActed)))
    end

    if not PartyTurnComplete then
        print("[PartyTurnManager] WARNING: EndPartyTurn() called but not all characters have acted")
        -- Force complete anyway
        PartyTurnComplete = true
    end

    print("[PartyTurnManager] Calling EndPlayerTurn() to switch to Enemy phase...")

    -- Switch to enemy turn using existing API
    EndPlayerTurn()

    -- Check what turn it is now
    local currentTurn = GetCurrentTurn()
    print("[PartyTurnManager] After EndPlayerTurn(), GetCurrentTurn() = " .. tostring(currentTurn))
    print("[PartyTurnManager] Turn should now be 'Enemy'")

    if currentTurn ~= "Enemy" then
        print("[PartyTurnManager] ERROR: Turn is NOT 'Enemy' after EndPlayerTurn()!")
        print("[PartyTurnManager] This means enemies won't act!")
    else
        print("[PartyTurnManager] SUCCESS: Turn is now 'Enemy' - enemies should start acting")

        -- CRITICAL: Refill AP for all enemies at the start of enemy turn
        print("[PartyTurnManager] Refilling AP for all enemies...")
        local enemies = GetAllEnemies()
        if enemies and #enemies > 0 then
            print("[PartyTurnManager] Found " .. #enemies .. " enemies to refill")
            for i, enemyID in ipairs(enemies) do
                print("[PartyTurnManager]   Refilling AP for Enemy " .. enemyID .. "...")
                RefillEntityAP(enemyID)
                local currentAP, maxAP = GetEntityAP(enemyID)
                print("[PartyTurnManager]   Enemy " .. enemyID .. " AP: " .. tostring(currentAP) .. "/" .. tostring(maxAP))
            end
        else
            print("[PartyTurnManager] WARNING: No enemies found to refill AP")
        end

        -- Initialize sequential enemy turn system
        print("[PartyTurnManager] Initializing sequential enemy turn system...")
        if InitializeEnemyTurn then
            InitializeEnemyTurn()
        else
            print("[PartyTurnManager] WARNING: EnemyTurnManager not loaded!")
        end
    end

    -- NOTE: Do NOT call ResetPartyTurn() here!
    -- It will be called in OnEnemyTurnEnded() when the enemy turn actually ends
    -- Calling it here would override the enemy camera we just set

    print("[PartyTurnManager] EndPartyTurn() COMPLETE")
    print("============================================================")
end

--[[
    ResetPartyTurn()
    Resets party state for a new turn cycle
    Called automatically when enemy turn ends
]]--
function ResetPartyTurn()
    -- Safety check: Don't reset if party not initialized
    if #PartyMembers == 0 then
        Log("[PartyTurnManager] WARNING: ResetPartyTurn called but party not initialized yet")
        return
    end

    ActiveCharacterIndex = 1
    PartyTurnComplete = false

    -- Notify C++ about active character reset
    SetActiveCharacter(PartyMembers[1].entityID)

    -- Reset hasActed flags and refill AP for all party members
    for i = 1, #PartyMembers do
        PartyMembers[i].hasActed = false
        RefillEntityAP(PartyMembers[i].entityID)
    end

    Log("[PartyTurnManager] Party turn reset - back to " .. PartyMembers[1].name)

    -- Log AP status for all characters
    for i = 1, #PartyMembers do
        local currentAP, maxAP = GetEntityAP(PartyMembers[i].entityID)
        Log(string.format("[PartyTurnManager] %s AP: %d/%d",
            PartyMembers[i].name,
            currentAP,
            maxAP))
    end

    -- Optional: Trigger camera switch to first character
    if #PartyMembers > 0 then
        OnCharacterSwitched(PartyMembers[1].entityID)
    end
end

-- ============================================================================
-- CAMERA INTEGRATION (Optional)
-- ============================================================================

--[[
    OnCharacterSwitched(newCharID)
    Called when active character changes
    Can be used to trigger camera movement, UI updates, etc.

    @param newCharID Entity ID of new active character
]]--
function OnCharacterSwitched(newCharID)
    print("[PartyTurnManager] ==========================================")
    print("[PartyTurnManager] OnCharacterSwitched() called for entity " .. newCharID)
    print("[PartyTurnManager] ==========================================")

    -- Update the graphics system's camera follow target
    -- This is more robust than SetCameraPosition because the camera
    -- will continuously follow the entity every frame
    SetCameraFollowTarget(newCharID)

    print("[PartyTurnManager] Camera now following Entity " .. newCharID)
    print("[PartyTurnManager] ==========================================")

    -- Optional: Play sound effect for character switch
    -- PlaySound("character_switch.wav", false)
end

-- ============================================================================
-- DEBUG FUNCTIONS
-- ============================================================================

--[[
    DebugPrintPartyState()
    Prints current party state for debugging
]]--
function DebugPrintPartyState()
    Log("========================================")
    Log("PARTY STATE DEBUG")
    Log("========================================")
    Log("Active Character Index: " .. ActiveCharacterIndex)
    Log("Party Turn Complete: " .. tostring(PartyTurnComplete))
    Log("Current Game Turn: " .. GetCurrentTurn())
    Log("")
    Log("Party Members:")
    for i = 1, #PartyMembers do
        local member = PartyMembers[i]
        local activeMarker = (i == ActiveCharacterIndex) and " <-- ACTIVE" or ""
        local actedMarker = member.hasActed and " [ACTED]" or " [NOT ACTED]"
        Log(string.format("  %d. %s (Entity %d)%s%s",
            i,
            member.name,
            member.entityID,
            actedMarker,
            activeMarker))
    end
    Log("========================================")
end

-- ============================================================================
-- INTEGRATION WITH EXISTING TURN SYSTEM
-- ============================================================================

--[[
    OnEnemyTurnEnded()
    Called when enemy turn ends
    Resets party for new player turn

    Should be called from your existing turn system
]]--
function OnEnemyTurnEnded()
    Log("[PartyTurnManager] Enemy turn ended - resetting party")
    ResetPartyTurn()
end

-- ============================================================================
-- UPDATE / COOLDOWN MANAGEMENT
-- ============================================================================

--[[
    UpdatePartyTurnManager(dt)
    Updates turn transition cooldown timer

    Must be called every frame from level's Update() function

    @param dt Delta time in seconds
]]--
function UpdatePartyTurnManager(dt)
    if TurnTransitionCooldown > 0 then
        TurnTransitionCooldown = TurnTransitionCooldown - dt
        if TurnTransitionCooldown < 0 then
            TurnTransitionCooldown = 0
        end
    end
end

--[[
    IsInTurnTransition()
    Checks if we're currently in turn transition cooldown

    Used by PlayerScript to block input during character transitions

    @return true if in cooldown, false otherwise
]]--
function IsInTurnTransition()
    return TurnTransitionCooldown > 0
end

-- ============================================================================
-- GLOBAL EXPORTS
-- ============================================================================

-- Export functions to global scope for use in other scripts
_G.InitializeParty = InitializeParty
_G.GetActiveCharacter = GetActiveCharacter
_G.GetActiveCharacterName = GetActiveCharacterName
_G.IsActiveCharacter = IsActiveCharacter
_G.GetPartyMembers = GetPartyMembers
_G.GetPartyMemberInfo = GetPartyMemberInfo
_G.IsPartyTurnComplete = IsPartyTurnComplete
_G.NextCharacterTurn = NextCharacterTurn
_G.EndCharacterTurn = EndCharacterTurn
_G.EndPartyTurn = EndPartyTurn
_G.ResetPartyTurn = ResetPartyTurn
_G.OnCharacterSwitched = OnCharacterSwitched
_G.OnEnemyTurnEnded = OnEnemyTurnEnded
_G.DebugPrintPartyState = DebugPrintPartyState
_G.UpdatePartyTurnManager = UpdatePartyTurnManager
_G.IsInTurnTransition = IsInTurnTransition

print("============================================================")
print("========== PartyTurnManager.lua LOADED SUCCESSFULLY ==========")
print("============================================================")

Log("[PartyTurnManager] Loaded successfully")
