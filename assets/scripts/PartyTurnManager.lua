--[[
===============================================================================
 File:           PartyTurnManager.lua
 Author:         Ethan Ng Yong Le
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

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
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
-- PROJECTILE DAMAGE (Soul Rend, Soul Merge - called from C++ ProjectileSystem)
-- ============================================================================
--[[
    ApplyProjectileDamage(enemyID, damage, attackerID)
    Called when a player projectile hits an enemy. Uses full damage pipeline
    (DamageEntity) and triggers Soul Rend heal + Soul Merge kill heal.
]]
function ApplyProjectileDamage(enemyID, damage, attackerID)
    local hadSoulRend = HasStatusEffect and HasStatusEffect(enemyID, "soulRend")
    local success = DamageEntity(enemyID, damage, attackerID or 0)
    if success and hadSoulRend then
        local allPlayers = GetAllPlayers()
        if allPlayers then
            for _, pid in ipairs(allPlayers) do
                local hp, maxHP = GetEntityHP(pid)
                if hp and maxHP and hp > 0 and hp < maxHP then
                    SetEntityHP(pid, math.min(hp + 1, maxHP))
                end
            end
        end
    end
    if success and attackerID and attackerID > 0 and HasStatusEffect and HasStatusEffect(attackerID, "soulMergeBuff") then
        local hp = GetEntityHP(enemyID)
        if hp == nil or hp <= 0 then
            local ahp, amax = GetEntityHP(attackerID)
            if ahp and amax and ahp > 0 and ahp < amax then
                SetEntityHP(attackerID, math.min(ahp + 1, amax))
            end
        end
    end
    return success
end

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
        RefillEntityAttackAP(PartyMembers[i].entityID)
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

    -- Guard against double-call when turn is already complete
    if PartyTurnComplete then
        print("[PartyTurnManager] WARNING: NextCharacterTurn() called but PartyTurnComplete=true, ignoring")
        return
    end

    -- Guard against out-of-bounds index
    if ActiveCharacterIndex < 1 or ActiveCharacterIndex > #PartyMembers then
        print(string.format("[PartyTurnManager] ERROR: Invalid ActiveCharacterIndex=%d (must be 1-%d), ignoring",
            ActiveCharacterIndex, #PartyMembers))
        return
    end

    print("[PartyTurnManager] ========== TURN ADVANCEMENT ==========")
    print(string.format("[PartyTurnManager] Current: %s (Entity %d, Index %d)",
        PartyMembers[ActiveCharacterIndex].name,
        PartyMembers[ActiveCharacterIndex].entityID,
        ActiveCharacterIndex))

    -- Mark current character as having acted
    PartyMembers[ActiveCharacterIndex].hasActed = true

    -- Dark Omens Triggered: kill this character at end of their turn
    local currentEntity = PartyMembers[ActiveCharacterIndex].entityID
    if HasStatusEffect and HasStatusEffect(currentEntity, "darkOmensTriggered") then
        print(string.format("[PartyTurnManager] %s has DARK OMENS TRIGGERED - dying at end of turn!",
            PartyMembers[ActiveCharacterIndex].name))
        RemoveStatusEffect(currentEntity, "darkOmensTriggered")
        SetEntityHP(currentEntity, 0)
        DamageEntity(currentEntity, 0)  -- trigger death cleanup
    end

    print(string.format("[PartyTurnManager] %s marked as ACTED",
        PartyMembers[ActiveCharacterIndex].name))

    -- Move to next character, skipping dead ones
    print(string.format("[PartyTurnManager DEBUG] BEFORE increment: ActiveCharacterIndex = %d, #PartyMembers = %d",
        ActiveCharacterIndex, #PartyMembers))

    ActiveCharacterIndex = ActiveCharacterIndex + 1

    print(string.format("[PartyTurnManager DEBUG] AFTER increment: ActiveCharacterIndex = %d, #PartyMembers = %d",
        ActiveCharacterIndex, #PartyMembers))

    -- Skip dead, Soul Merge, and already-acted (e.g. stunned) characters
    local skippedDead = 0
    while ActiveCharacterIndex <= #PartyMembers do
        local checkEntity = PartyMembers[ActiveCharacterIndex].entityID
        local currentHP, maxHP = GetEntityHP(checkEntity)

        -- Skip dead characters
        if not currentHP or currentHP <= 0 then
            print(string.format("[PartyTurnManager] %s is DEAD (HP: %s), skipping...",
                PartyMembers[ActiveCharacterIndex].name, tostring(currentHP)))
            PartyMembers[ActiveCharacterIndex].hasActed = true
            ActiveCharacterIndex = ActiveCharacterIndex + 1
            skippedDead = skippedDead + 1
        -- Skip Soul Merge sacrificed characters (turn permanently skipped)
        elseif HasStatusEffect and HasStatusEffect(checkEntity, "soulMerge") then
            print(string.format("[PartyTurnManager] %s has SOUL MERGE - turn skipped",
                PartyMembers[ActiveCharacterIndex].name))
            PartyMembers[ActiveCharacterIndex].hasActed = true
            ActiveCharacterIndex = ActiveCharacterIndex + 1
            skippedDead = skippedDead + 1
        -- Skip characters already marked as acted (e.g. stunned from Groundshatter)
        elseif PartyMembers[ActiveCharacterIndex].hasActed then
            print(string.format("[PartyTurnManager] %s already acted (stunned?) - skipping",
                PartyMembers[ActiveCharacterIndex].name))
            ActiveCharacterIndex = ActiveCharacterIndex + 1
            skippedDead = skippedDead + 1
        else
            -- This character is alive and active, use them
            break
        end
    end

    if skippedDead > 0 then
        print(string.format("[PartyTurnManager] Skipped %d dead/merged character(s)", skippedDead))
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
        print("[PartyTurnManager] AUTO-CALLING EndPartyTurn() to switch to enemy phase...")
        EndPartyTurn()
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

    -- Check if stunned (stun skips the entire turn)
    -- Note: stun is already handled in ResetPartyTurn (marks hasActed=true),
    -- but check here as a safety net for mid-round stun effects
    local isStunned = HasStatusEffect and HasStatusEffect(newActiveEntity, "stun")

    -- If stunned, skip this character's turn entirely
    if isStunned then
        print(string.format("[PartyTurnManager] %s is STUNNED - skipping turn!",
            PartyMembers[ActiveCharacterIndex].name))
        PartyMembers[ActiveCharacterIndex].hasActed = true
        NextCharacterTurn()
        return
    end

    -- AP refill, status effect decrementing, and Soul Merge Buff bonuses are all
    -- handled by ResetPartyTurn at the start of each round for ALL characters.
    -- Do NOT refill AP or apply bonuses here, as it would override the overload
    -- AP-skip and double-apply Soul Merge Buff bonuses from ResetPartyTurn.

    local currentAP, maxAP = GetEntityAP(newActiveEntity)
    local currentAttackAP, maxAttackAP = GetEntityAttackAP(newActiveEntity)
    print(string.format("[PartyTurnManager] %s AP: %d/%d, AttackAP: %d/%d",
        PartyMembers[ActiveCharacterIndex].name,
        currentAP,
        maxAP,
        currentAttackAP,
        maxAttackAP))

    -- Restore attack AP crystal visuals
    if UIManager and UIManager.GetComponent then
        local attackAPIndicator = UIManager.GetComponent("attackAP")
        if attackAPIndicator and attackAPIndicator.RestoreAllAP then
            attackAPIndicator:RestoreAllAP()
            print("[PartyTurnManager] Restored all attack AP crystals")
        end
        
        -- Restore movement AP visuals (force immediate update)
        local movementAPIndicator = UIManager.GetComponent("movementAP")
        if movementAPIndicator and movementAPIndicator.ForceUpdate then
            movementAPIndicator:ForceUpdate()
            print("[PartyTurnManager] Force updated movement AP indicator")
        elseif movementAPIndicator and movementAPIndicator.RestoreAllAP then
            movementAPIndicator:RestoreAllAP()
            print("[PartyTurnManager] Restored all movement AP crystals")
        end
    end

    -- Start turn transition cooldown to prevent input carry-over
    TurnTransitionCooldown = TurnTransitionCooldownTime
    print(string.format("[PartyTurnManager] Turn transition cooldown started: %.2fs", TurnTransitionCooldown))

    print("[PartyTurnManager] ======================================")
    PlaySound("turnstart", false, 0.5)
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
    PlaySound("turnend", false, 0.5)
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

                -- Mana Drain: target starts with 3 less AP next turn
                if HasStatusEffect and HasStatusEffect(enemyID, "manaDrain") then
                    ConsumeEnemyAP(enemyID, 3)
                    print("[PartyTurnManager]   Enemy " .. enemyID .. " has MANA DRAIN - AP reduced by 3")
                end

                local currentAP, maxAP = GetEntityAP(enemyID)
                print("[PartyTurnManager]   Enemy " .. enemyID .. " AP: " .. tostring(currentAP) .. "/" .. tostring(maxAP))
            end
        else
            print("[PartyTurnManager] WARNING: No enemies found to refill AP")
        end

        -- Initialize sequential enemy turn system
        print("[PartyTurnManager] Initializing sequential enemy turn system...")
        if InitializeEnemyTurn then
            local enemyTurnStarted = InitializeEnemyTurn()
            if not enemyTurnStarted then
                -- No enemies found - immediately end enemy turn and return to player
                print("[PartyTurnManager] No enemies to act - skipping enemy turn!")
                print("[PartyTurnManager] Calling EndEnemyTurn() to return to player...")
                EndEnemyTurn()

                -- CRITICAL: Reset party turn immediately since we're skipping enemy turn
                -- Normally this would be called by OnEnemyTurnEnded() on the next frame,
                -- but we need it NOW to avoid the party being stuck in a stale state
                print("[PartyTurnManager] Immediately resetting party turn (no enemies)...")
                ResetPartyTurn()
            end
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

    -- Reset hasActed flags and refill AP for all party members
    -- ORDER: Check flags -> Refill AP/MP -> Decrement status effects
    -- (Status effects trigger AFTER AP/MP refill)
    for i = 1, #PartyMembers do
        PartyMembers[i].hasActed = false
        local eid = PartyMembers[i].entityID

        -- Check for stun and Overload BEFORE refilling or decrementing
        local isStunned = HasStatusEffect and HasStatusEffect(eid, "stun")
        local hasOverload = HasStatusEffect and HasStatusEffect(eid, "overload")

        -- If stunned, mark as acted so their turn is skipped
        if isStunned then
            PartyMembers[i].hasActed = true
            Log(string.format("[PartyTurnManager] ResetPartyTurn: %s is STUNNED - turn skipped!",
                PartyMembers[i].name))
        end

        -- 1) Refill AP: always refill movement AP (MP); skip Attack AP only if overloaded
        RefillEntityAP(eid)
        if hasOverload then
            Log(string.format("[PartyTurnManager] ResetPartyTurn: %s has OVERLOAD - Attack AP refill skipped (MP still refilled)",
                PartyMembers[i].name))
        else
            RefillEntityAttackAP(eid)
        end

        -- Soul Merge Buff: grant +1 movement AP and +1 attack AP
        if HasStatusEffect and HasStatusEffect(eid, "soulMergeBuff") then
            ConsumeEntityAP(eid, -1)
            ConsumeEntityAttackAP(eid, -1)
            Log(string.format("[PartyTurnManager] ResetPartyTurn: %s has SOUL MERGE BUFF - +1 AP bonus",
                PartyMembers[i].name))
        end

        -- Bloody Warcry: convert pending to active at round start
        -- (pending is applied during the casting round to prevent same-round consumption)
        if HasStatusEffect and HasStatusEffect(eid, "bloodyWarcryPending") then
            RemoveStatusEffect(eid, "bloodyWarcryPending")
            ApplyStatusEffect(eid, "bloodyWarcry", -1, 0)
            Log(string.format("[PartyTurnManager] ResetPartyTurn: %s BLOODY WARCRY activated for this round",
                PartyMembers[i].name))
        end

        -- 2) Decrement status effects AFTER AP refill
        if DecrementStatusEffects then
            DecrementStatusEffects(eid)
        end
    end

    -- 3) Decrement status effects for ALL enemies (centralized, once per turn cycle)
    -- This ensures debuffs with duration 2 last "current turn + next turn"
    if DecrementStatusEffects then
        local enemies = GetAllEnemies()
        if enemies then
            for _, enemyID in ipairs(enemies) do
                DecrementStatusEffects(enemyID)
            end
        end
    end

    -- Find first character who is alive and not Soul Merged (may be stunned - we'll auto-skip like pressing P)
    while ActiveCharacterIndex <= #PartyMembers do
        local checkEntity = PartyMembers[ActiveCharacterIndex].entityID
        local currentHP, maxHP = GetEntityHP(checkEntity)

        if not currentHP or currentHP <= 0 then
            -- This character is dead, skip
            Log(string.format("[PartyTurnManager] ResetPartyTurn: %s is DEAD (HP: %s), skipping...",
                PartyMembers[ActiveCharacterIndex].name, tostring(currentHP)))
            PartyMembers[ActiveCharacterIndex].hasActed = true
            ActiveCharacterIndex = ActiveCharacterIndex + 1
        elseif HasStatusEffect and HasStatusEffect(checkEntity, "soulMerge") then
            -- This character is Soul Merged, skip permanently
            Log(string.format("[PartyTurnManager] ResetPartyTurn: %s has SOUL MERGE - skipping",
                PartyMembers[ActiveCharacterIndex].name))
            PartyMembers[ActiveCharacterIndex].hasActed = true
            ActiveCharacterIndex = ActiveCharacterIndex + 1
        else
            -- This character is alive (may be stunned - hasActed=true, we'll auto EndCharacterTurn below)
            break
        end
    end

    -- Check if all characters are dead
    if ActiveCharacterIndex > #PartyMembers then
        Log("[PartyTurnManager] ERROR: All party members are dead in ResetPartyTurn!")
        PartyTurnComplete = true
        return
    end

    -- Notify C++ about active character (may be stunned - we enter their turn then auto-skip like pressing P)
    SetActiveCharacter(PartyMembers[ActiveCharacterIndex].entityID)

    -- If this character is stunned, auto-skip their turn (simulate pressing P to end turn)
    if PartyMembers[ActiveCharacterIndex].hasActed then
        Log(string.format("[PartyTurnManager] %s is STUNNED - auto-skipping turn (like pressing P)",
            PartyMembers[ActiveCharacterIndex].name))
        EndCharacterTurn()
        return  -- EndCharacterTurn already advanced to next character
    end

    -- Restore attack AP crystal visuals for the active character
    if UIManager and UIManager.GetComponent then
        local attackAPIndicator = UIManager.GetComponent("attackAP")
        if attackAPIndicator and attackAPIndicator.RestoreAllAP then
            attackAPIndicator:RestoreAllAP()
            Log("[PartyTurnManager] Restored all attack AP crystals for " .. PartyMembers[ActiveCharacterIndex].name)
        end
        
        -- Restore movement AP visuals (force immediate update)
        local movementAPIndicator = UIManager.GetComponent("movementAP")
        if movementAPIndicator and movementAPIndicator.ForceUpdate then
            movementAPIndicator:ForceUpdate()
            Log("[PartyTurnManager] Force updated movement AP indicator")
        elseif movementAPIndicator and movementAPIndicator.RestoreAllAP then
            movementAPIndicator:RestoreAllAP()
            Log("[PartyTurnManager] Restored all movement AP crystals")
        end
    end

    Log("[PartyTurnManager] Party turn reset - back to " .. PartyMembers[ActiveCharacterIndex].name)

    -- Log AP status for all characters
    for i = 1, #PartyMembers do
        local currentAP, maxAP = GetEntityAP(PartyMembers[i].entityID)
        Log(string.format("[PartyTurnManager] %s AP: %d/%d",
            PartyMembers[i].name,
            currentAP,
            maxAP))
    end

    -- Optional: Trigger camera switch to active character (guaranteed to be alive)
    if ActiveCharacterIndex <= #PartyMembers then
        OnCharacterSwitched(PartyMembers[ActiveCharacterIndex].entityID)
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

--[[
    ApplyGroundshatterStun(entityID)
    Called by PlayerScript (via CallLevelFunction) when casting Groundshatter.
    Applies stun from level Lua state so HasStatusEffect sees it in ResetPartyTurn.
]]--
function ApplyGroundshatterStun(entityID)
    if ApplyStatusEffect and entityID then
        ApplyStatusEffect(entityID, "stun", 1, entityID)
        Log(string.format("[PartyTurnManager] ApplyGroundshatterStun: entity %s stunned for next turn", tostring(entityID)))
    end
end

-- Export functions to global scope for use in other scripts
_G.InitializeParty = InitializeParty
_G.ApplyGroundshatterStun = ApplyGroundshatterStun
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