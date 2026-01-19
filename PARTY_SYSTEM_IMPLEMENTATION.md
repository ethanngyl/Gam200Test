# Party System Implementation Guide
**Phase 1: Core Party System - COMPLETE**
**Date:** 2026-01-16

---

## 🎯 Overview

The party system allows 3 controllable characters to act sequentially during the player turn phase.

**Turn Flow:**
```
Character 1 acts → Character 2 acts → Character 3 acts → Enemy Turn → Repeat
```

---

## ✅ What's Been Implemented

### 1. **PartyTurnManager.lua** (Core System)
- Manages turn queue for 3 characters
- Tracks which character is currently active
- Prevents simultaneous input from all characters
- Provides `IsActiveCharacter()` function for input routing

**Key Functions:**
```lua
InitializeParty({warrior, mage, rogue})  -- Setup party with 3 entity IDs
GetActiveCharacter()                     -- Returns active character's entity ID
IsActiveCharacter(entityID)              -- Check if entity should process input
NextCharacterTurn()                      -- Advance to next party member
EndCharacterTurn()                       -- Finish current character's turn
EndPartyTurn()                           -- All 3 done, switch to enemy turn
```

### 2. **Entity-Based AP/HP APIs** (C++ Extensions)
New Lua APIs that work with any entity ID (not just "the player"):

```lua
-- AP Management
currentAP, maxAP = GetEntityAP(entityID)
ConsumeEntityAP(entityID, amount)
RefillEntityAP(entityID)

-- HP Management
currentHP, maxHP = GetEntityHP(entityID)
SetEntityHP(entityID, newHP)
```

### 3. **Updated PlayerScript.lua**
Now supports party system:
- Checks `IsActiveCharacter()` before processing input
- Uses entity-based APIs (`GetEntityAP`, `ConsumeEntityAP`)
- Auto-advances turn when out of AP (optional)

### 4. **PartyStatusUI.lua** (Multi-Character UI)
Displays all 3 characters' status:
- HP hearts (♥♥♥♥♥)
- AP crystals (◆◆◆)
- Highlights active character
- Grays out inactive characters

---

## 📋 Integration Guide

### Step 1: Load PartyTurnManager

Add to your level script's `OnInit()`:

```lua
-- Load party turn manager FIRST (provides IsActiveCharacter function)
dofile("assets/scripts/PartyTurnManager.lua")
```

### Step 2: Spawn 3 Party Members

Example implementation:

```lua
function SetupParty()
    Log("========================================")
    Log("Setting up 3-character party...")
    Log("========================================")

    -- Spawn 3 characters at starting positions
    local warrior = SpawnEntity("Warrior", startX, startY)
    local mage = SpawnEntity("Mage", startX + 1, startY)
    local rogue = SpawnEntity("Rogue", startX + 2, startY)

    -- Configure each character's stats
    ConfigureCharacter(warrior, {
        maxHP = 10,
        maxAP = 3,
        name = "Warrior"
    })

    ConfigureCharacter(mage, {
        maxHP = 5,
        maxAP = 5,
        name = "Mage"
    })

    ConfigureCharacter(rogue, {
        maxHP = 7,
        maxAP = 4,
        name = "Rogue"
    })

    -- Attach PlayerScript to each character
    AddScriptComponentToEntity(warrior, "assets/scripts/PlayerScript.lua")
    AddScriptComponentToEntity(mage, "assets/scripts/PlayerScript.lua")
    AddScriptComponentToEntity(rogue, "assets/scripts/PlayerScript.lua")

    -- Attach AnimationController (optional)
    AddScriptComponentToEntity(warrior, "assets/scripts/AnimationController.lua")
    AddScriptComponentToEntity(mage, "assets/scripts/AnimationController.lua")
    AddScriptComponentToEntity(rogue, "assets/scripts/AnimationController.lua")

    -- Initialize party system
    local partyInitialized = InitializeParty({warrior, mage, rogue})

    if partyInitialized then
        Log("✓ Party initialized successfully")
        Log("  - Warrior (Entity " .. warrior .. ")")
        Log("  - Mage (Entity " .. mage .. ")")
        Log("  - Rogue (Entity " .. rogue .. ")")
    else
        Log("✗ FAILED to initialize party")
    end

    return warrior, mage, rogue
end
```

### Step 3: Create Party UI (Optional)

```lua
function SetupPartyUI()
    -- Load PartyStatusUI
    dofile("assets/scripts/UI/PartyStatusUI.lua")

    -- Create UI instance
    partyUI = PartyStatusUI:new(0)
    partyUI:OnInit()

    Log("✓ Party status UI created")

    return partyUI
end
```

### Step 4: Update Game Loop

In your `OnUpdate(dt)`:

```lua
function OnUpdate(dt)
    -- Update party UI
    if partyUI then
        partyUI:OnUpdate(dt)
    end

    -- Character scripts automatically handle input routing via IsActiveCharacter()
    -- No additional logic needed here!
end
```

### Step 5: Handle Turn Transitions

When enemy turn ends, reset party:

```lua
function OnEnemyTurnEnded()
    -- Existing enemy turn logic
    EndEnemyTurn()

    -- Reset party for new player turn
    ResetPartyTurn()

    Log("Party turn reset - back to Character 1")
end
```

---

## 🎮 Usage Examples

### Switching Characters Manually

```lua
-- When player presses SPACE to end turn
if IsKeyPressed(32) then  -- SPACE key
    local activeChar = GetActiveCharacter()
    Log("Character " .. activeChar .. " ended turn")

    -- Advance to next character
    NextCharacterTurn()

    -- Check if all 3 have acted
    if IsPartyTurnComplete() then
        Log("All party members acted - ending party turn")
        EndPartyTurn()
    end
end
```

### Checking Party State

```lua
-- Get active character
local activeID = GetActiveCharacter()
local activeName = GetActiveCharacterName()
Log("Active: " .. activeName .. " (Entity " .. activeID .. ")")

-- Get all party members
local party = GetPartyMembers()
for i = 1, #party do
    local entityID = party[i]
    local currentAP, maxAP = GetEntityAP(entityID)
    Log("Character " .. i .. ": " .. currentAP .. "/" .. maxAP .. " AP")
end

-- Debug party state
DebugPrintPartyState()
```

---

## 🔧 Configuration

### Character Stats

Customize in `PartyTurnManager.lua`:

```lua
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
```

### Auto-Advance on AP Depletion

In `PlayerScript.lua`, uncomment:

```lua
if currentAP == 0 then
    Log("[PlayerScript] Character out of AP - ending turn")
    EndCharacterTurn()  -- Auto-advance to next character
end
```

---

## 🐛 Troubleshooting

### Issue: All 3 characters move simultaneously

**Cause:** PartyTurnManager not loaded before PlayerScript
**Fix:** Load PartyTurnManager first in OnInit()

### Issue: IsActiveCharacter() not found

**Cause:** PartyTurnManager.lua not loaded
**Fix:** Add `dofile("assets/scripts/PartyTurnManager.lua")` to your level script

### Issue: Party members don't have HP/AP

**Cause:** Entities missing AP or Health components
**Fix:** Add components when spawning entities:

```lua
local function ConfigureCharacter(entityID, config)
    -- Add AP component
    -- AddComponent<AP>(entityID, config.maxAP, config.maxAP)

    -- Add Health component
    -- AddComponent<Health>(entityID, config.maxHP, config.maxHP)
end
```

### Issue: UI shows wrong characters

**Cause:** Party initialized before UI created
**Fix:** Create UI AFTER calling InitializeParty()

---

## 📊 Testing Checklist

- [ ] Load PartyTurnManager successfully
- [ ] Spawn 3 characters
- [ ] Initialize party with InitializeParty()
- [ ] Verify only active character responds to input
- [ ] Test turn advancement (Character 1 → 2 → 3)
- [ ] Verify EndPartyTurn() switches to enemy turn
- [ ] Test AP consumption per character
- [ ] Verify HP display for all 3 characters
- [ ] Test active character highlighting
- [ ] Verify ResetPartyTurn() on new player turn

---

## 📁 Files Modified/Created

### New Files:
- `assets/scripts/PartyTurnManager.lua` - Core turn management
- `assets/scripts/UI/PartyStatusUI.lua` - Multi-character UI

### Modified Files:
- `assets/scripts/PlayerScript.lua` - Added IsActiveCharacter() check
- `src/Script/LevelLoader.h` - Added entity-based API declarations
- `src/Script/LevelLoader.cpp` - Implemented entity-based APIs

### C++ API Functions Added:
- `GetEntityAP(entityID)` - Get any entity's AP
- `ConsumeEntityAP(entityID, amount)` - Consume any entity's AP
- `RefillEntityAP(entityID)` - Refill any entity's AP
- `GetEntityHP(entityID)` - Get any entity's HP
- `SetEntityHP(entityID, newHP)` - Set any entity's HP

---

## 🚀 Next Steps (Phase 2)

### Remaining Features:
1. **Camera Auto-Follow** - Smooth pan to active character
2. **Character Portraits** - Visual icons for each character
3. **Turn Indicator UI** - "Character 1's Turn" text display
4. **Death Handling** - Skip dead characters in turn queue
5. **Character Selection Keys** - Press 1/2/3 to switch manually

---

## 💡 Design Notes

### Why Sequential Turns?

Traditional party-based RPGs (Final Fantasy, Fire Emblem) use sequential character turns because it:
- Prevents input confusion (which character am I controlling?)
- Creates tactical decision points (who should act first?)
- Allows camera focus on active character
- Simplifies animation and visual feedback

### Why Entity-Based APIs?

Using `GetEntityAP(entityID)` instead of `GetPlayerAP()` provides:
- **Flexibility:** Same API works for party members and enemies
- **Scalability:** Easy to add/remove party members
- **Reusability:** Scripts can be attached to any entity type
- **Future-proofing:** Supports 1-character mode, 3-character mode, or even 5-character mode

### Turn Flow Diagram

```
┌─────────────────────────────────────────┐
│         PLAYER TURN BEGINS              │
└────────────────┬────────────────────────┘
                 │
    ┌────────────▼──────────┐
    │ Character 1's Turn    │
    │ - Process WASD input  │
    │ - Consume AP          │
    │ - Camera focused here │
    └────────────┬──────────┘
                 │ (EndCharacterTurn)
    ┌────────────▼──────────┐
    │ Character 2's Turn    │
    │ - Process WASD input  │
    │ - Consume AP          │
    │ - Camera pans here    │
    └────────────┬──────────┘
                 │ (EndCharacterTurn)
    ┌────────────▼──────────┐
    │ Character 3's Turn    │
    │ - Process WASD input  │
    │ - Consume AP          │
    │ - Camera pans here    │
    └────────────┬──────────┘
                 │ (EndPartyTurn)
    ┌────────────▼──────────┐
    │ ENEMY TURN            │
    │ - All enemies act     │
    │ - Existing AI logic   │
    └────────────┬──────────┘
                 │ (OnEnemyTurnEnded)
                 └──────────► PLAYER TURN BEGINS (loop)
```

---

## ✅ Success Criteria

The party system is working correctly when:

1. ✅ Only 1 character responds to input at a time
2. ✅ Turns advance: Char1 → Char2 → Char3 → Enemies → Repeat
3. ✅ Each character has individual AP tracking
4. ✅ Each character has individual HP tracking
5. ✅ UI shows all 3 characters' status
6. ✅ Active character is visually highlighted
7. ✅ Player turn doesn't end until all 3 have acted
8. ✅ Existing enemy AI still works

---

**Status:** Phase 1 Complete ✅
**Next Phase:** Camera system & advanced UI
