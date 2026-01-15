# Multi-Character Party System Feasibility Analysis
**Feature Request:** 3 controllable characters with individual AP, camera switching between turns

---

## Executive Summary

### ✅ Current Foundation: **GOOD** (70% Ready)
Your architecture is **well-designed** and can support a party system with moderate extensions.

### 🔧 Required Changes: **MODERATE** (Manageable scope)
- Need new turn management system
- Need entity-aware APIs (not player-specific)
- Need input routing to active character
- Need party UI components

### ⏱️ Estimated Effort: **2-3 days of work**
- 40% scripting (Lua)
- 30% API extensions (C++)
- 30% UI/camera work

---

## ✅ What's Already Good (Robust Architecture)

### 1. **Entity Component System** ✅
**Status:** PERFECT for multi-character support

```cpp
// Each entity has own components
struct AP : public Component<AP> {
    int actionPoints = 3;
    int maxActionPoints = 3;
};

struct Health : public Component<Health> {
    int hp = 5;
    int maxHP = 5;
};
```

**Why This is Good:**
- Each character entity has its own AP component
- Each character entity has its own Health component
- No global state - fully independent characters
- **Score: 10/10** - Zero changes needed

---

### 2. **PlayerScript.lua Architecture** ✅
**Status:** EXCELLENT - Already entity-aware

```lua
-- Current code (already good!)
function OnInit()
    entityID = self  -- Uses unique entity ID
end

function OnUpdate(dt)
    -- Processes input for THIS entity
    if IsKeyDown("W") then
        -- Move THIS character
    end
end
```

**Why This is Good:**
- Script is attached per-entity (not global)
- Each character gets own script instance
- State is per-entity (moveCooldown, entityID)
- **Score: 10/10** - Just attach to 3 entities!

**Example Usage:**
```lua
-- Already works!
AddScriptComponentToEntity(character1, "assets/scripts/PlayerScript.lua")
AddScriptComponentToEntity(character2, "assets/scripts/PlayerScript.lua")
AddScriptComponentToEntity(character3, "assets/scripts/PlayerScript.lua")
```

---

### 3. **Grid Movement System** ✅
**Status:** GOOD - Entity-based APIs

```lua
-- Already entity-aware!
MoveEntityToTile(entityID, x, y)  -- Works for any entity
GetEntityGridPosition(entityID)   -- Works for any entity
```

**Why This is Good:**
- Movement APIs accept entity IDs
- No hardcoded "player" in movement logic
- **Score: 9/10** - Minor API additions needed

---

### 4. **Modular UI Architecture** ✅
**Status:** VERY GOOD - Can be extended

```lua
UIManager.lua
    ├─ APIndicatorUI.lua
    ├─ HealthUI.lua
    └─ TurnIndicatorUI.lua
```

**Why This is Good:**
- Component-based UI (easy to add CharacterSelectUI)
- Already supports multiple instances
- Clean separation of concerns
- **Score: 8/10** - Need multi-character variants

---

### 5. **Camera System** ✅
**Status:** FUNCTIONAL - Manual control works

```lua
SetCameraPosition(x, y, z)  -- Can manually position camera
```

**Why This is Good:**
- Can position camera anywhere
- Can track entity positions
- **Score: 7/10** - Need smooth follow/transition

---

## ❌ What Needs Work (Required Extensions)

### 1. **Turn Management System** ❌
**Current:** Binary (Player → Enemy)
**Needed:** Queue-based (Char1 → Char2 → Char3 → Enemies)

**Current Code:**
```lua
GetCurrentTurn()  -- Returns "Player" or "Enemy"
EndPlayerTurn()   -- Ends all player actions
```

**Problem:**
- No concept of "active character"
- No turn queue system
- All 3 characters act simultaneously (not sequentially)

**Solution Needed:** TurnManager.lua
```lua
-- New turn queue system
TurnQueue = {
    { entity = char1, type = "player" },
    { entity = char2, type = "player" },
    { entity = char3, type = "player" },
    { entity = enemy1, type = "enemy" },
    { entity = enemy2, type = "enemy" },
}

function GetActiveEntity() -- Returns entityID of current actor
function NextTurn()        -- Advances to next in queue
function GetTurnPhase()    -- Returns "PlayerChar1", "PlayerChar2", etc.
```

**Impact:** HIGH - Core gameplay mechanic
**Effort:** MEDIUM - New Lua script + API functions

---

### 2. **Player-Specific APIs** ❌
**Current:** Hardcoded to single player
**Needed:** Entity-based variants

**Problem APIs:**
```lua
GetPlayerAP()           -- Only works for "the player"
ConsumePlayerAP(amount) -- Only works for "the player"
RefillPlayerAP()        -- Only works for "the player"
FindPlayer()            -- Returns ONE player entity
```

**Solution Needed:** Generic entity APIs
```lua
-- New entity-based APIs
GetEntityAP(entityID)
ConsumeEntityAP(entityID, amount)
RefillEntityAP(entityID)
GetPartyMembers()  -- Returns array of character entity IDs
```

**Implementation:**
```cpp
// C++ API additions (LevelLoader_API.cpp)
int LevelLoader::Lua_GetEntityAP(lua_State* L) {
    lua_Integer entityID = luaL_checkinteger(L, 1);
    Entity e(static_cast<uint32_t>(entityID));

    if (em->HasComponent<AP>(e)) {
        auto& ap = em->GetComponent<AP>(e);
        lua_pushinteger(L, ap.actionPoints);
        lua_pushinteger(L, ap.maxActionPoints);
        return 2;
    }
    return 0;
}
```

**Impact:** HIGH - Used everywhere
**Effort:** LOW - Straightforward C++ functions

---

### 3. **Input Routing System** ❌
**Current:** All characters respond to input
**Needed:** Only active character responds

**Current Problem:**
```lua
-- PlayerScript.lua - ALL 3 characters process input simultaneously!
function OnUpdate(dt)
    if IsKeyDown("W") then
        -- ALL 3 characters try to move at once!
    end
end
```

**Solution Needed:** Active character check
```lua
-- Option 1: Check in PlayerScript
function OnUpdate(dt)
    if not IsActiveCharacter(entityID) then
        return  -- Not my turn, do nothing
    end

    -- Process input only for active character
    if IsKeyDown("W") then
        -- Move
    end
end

-- Option 2: Disable/enable scripts
function ActivateCharacter(entityID)
    -- Enable script for active character
    -- Disable scripts for others
end
```

**Impact:** HIGH - Prevents chaos
**Effort:** MEDIUM - Need active character tracking

---

### 4. **UI Extensions** ❌
**Current:** Shows single player status
**Needed:** Shows all 3 characters + highlights active

**Current UI:**
- 1 HP bar (bottom left)
- 1 AP indicator (bottom left)
- Turn indicator (Player/Enemy)

**Needed UI:**
```
Bottom of screen:
[Char1: ♥♥♥♥♥ ◆◆◆] [Char2: ♥♥♥♥♥ ◆◆◆] [Char3: ♥♥♥♥♥ ◆◆◆]
     ^ACTIVE^         ^grayed out^       ^grayed out^

Character portraits showing:
- Health hearts
- AP crystals
- Active highlight/border
- Character name/icon
```

**Solution Needed:** PartyStatusUI.lua
```lua
-- New UI component
PartyStatusUI.lua
    ├─ Shows all 3 characters
    ├─ Highlights active character
    ├─ Grays out inactive characters
    └─ Uses GetPartyMembers() + GetEntityAP()/GetEntityHP()
```

**Impact:** MEDIUM - Visual clarity
**Effort:** MEDIUM - New UI component

---

### 5. **Camera Follow System** ❌
**Current:** Manual camera positioning
**Needed:** Auto-follow active character with smooth transitions

**Solution Needed:**
```lua
-- New camera controller
function SetCameraFollowTarget(entityID)
function UpdateCameraFollow(dt)  -- Smooth lerp to target
function OnCharacterSwitched(newCharID)
    SetCameraFollowTarget(newCharID)
    -- Smooth camera transition
end
```

**Alternative:** C++ camera follow system
```cpp
// Add to camera system
void Camera::SetFollowTarget(Entity target);
void Camera::Update(float dt); // Lerp to target position
```

**Impact:** MEDIUM - Nice-to-have
**Effort:** LOW-MEDIUM - Can use lerp for smoothness

---

### 6. **Party Manager System** ❌
**Current:** No concept of "party"
**Needed:** Central party state management

**Solution Needed:** PartyManager.lua
```lua
local PartyManager = {
    members = {},        -- { char1ID, char2ID, char3ID }
    activeIndex = 1,     -- Which character is currently acting
    activeEntityID = 0   -- Current active character
}

function PartyManager.Init(characterIDs)
    -- Store party members
end

function PartyManager.GetActiveCharacter()
    return PartyManager.activeEntityID
end

function PartyManager.NextCharacter()
    -- Switch to next character
    -- Update camera
    -- Update UI highlighting
end

function PartyManager.GetAllMembers()
    return PartyManager.members
end

function PartyManager.IsActive(entityID)
    return entityID == PartyManager.activeEntityID
end
```

**Impact:** HIGH - Core coordination
**Effort:** MEDIUM - New Lua script

---

## 📊 Detailed Readiness Matrix

| System | Current State | Needed For Party | Effort | Priority |
|--------|---------------|------------------|--------|----------|
| **Entity Components** | ✅ Perfect | None | None | - |
| **PlayerScript** | ✅ Entity-aware | Input filtering | Low | High |
| **Grid Movement** | ✅ Entity-based | Minor API additions | Low | High |
| **Turn System** | ❌ Binary | Complete rewrite | Medium | Critical |
| **Player APIs** | ❌ Single player | Entity-based variants | Low | Critical |
| **Input Routing** | ❌ All respond | Active character check | Medium | Critical |
| **UI System** | ⚠️ Single player | Multi-character variant | Medium | High |
| **Camera System** | ⚠️ Manual | Follow + transitions | Medium | Medium |
| **Party Management** | ❌ None | Complete system | Medium | Critical |

**Legend:**
- ✅ Ready to use as-is
- ⚠️ Partially ready, needs extension
- ❌ Requires new implementation

---

## 🎯 Implementation Roadmap

### Phase 1: Core Party Support (Critical - 1 day)
**Goal:** Make 3 characters work with sequential turns

1. **Create PartyManager.lua** (2 hours)
   - Track 3 party member IDs
   - Track active character index
   - Provide IsActive(entityID) check

2. **Add Entity-Based APIs** (2 hours)
   ```cpp
   Lua_GetEntityAP(entityID)
   Lua_ConsumeEntityAP(entityID, amount)
   Lua_RefillEntityAP(entityID)
   Lua_GetEntityHP(entityID)
   Lua_GetPartyMembers()
   ```

3. **Create TurnManager.lua** (3 hours)
   - Turn queue system
   - Character1 → Character2 → Character3 → Enemies
   - Integration with PartyManager

4. **Update PlayerScript.lua** (1 hour)
   - Add `IsActive()` check in OnUpdate
   - Only process input if active
   ```lua
   function OnUpdate(dt)
       if not PartyManager.IsActive(entityID) then
           return
       end
       -- Normal movement code...
   end
   ```

**Outcome:** Basic party system works, characters take sequential turns

---

### Phase 2: UI Extensions (High Priority - 0.5 day)
**Goal:** Show all 3 characters' status

1. **Create PartyStatusUI.lua** (2 hours)
   - Show 3 character portraits
   - Display HP/AP for each
   - Highlight active character
   - Gray out inactive characters

2. **Update UIManager.lua** (1 hour)
   - Integrate PartyStatusUI component
   - Update based on active character changes

**Outcome:** Player sees all 3 characters' status at once

---

### Phase 3: Camera System (Medium Priority - 0.5 day)
**Goal:** Auto-follow active character

1. **Add Camera Follow API** (1 hour)
   ```cpp
   Lua_SetCameraFollowTarget(entityID)
   Lua_EnableCameraFollow(bool enable)
   ```

2. **Create CameraController.lua** (2 hours)
   - Smooth follow current character
   - Transition when character switches
   - Configurable follow speed

**Outcome:** Camera smoothly follows active character

---

### Phase 4: Polish (Optional - 0.5 day)
**Goal:** Nice-to-have features

1. **Manual Character Switching** (1 hour)
   - Tab key to switch characters mid-turn
   - Only if character has AP remaining

2. **Character Selection UI** (1 hour)
   - Number keys (1/2/3) to switch
   - Visual feedback

3. **Turn Transition Effects** (1 hour)
   - "Character 2's Turn!" text
   - Character portrait zoom
   - Sound effects

**Outcome:** Polished party experience

---

## 📝 Implementation Example

### Minimal Party System (Phase 1 Only)

**Level3Clean.lua changes:**
```lua
local PartyManager = require("PartyManager")
local TurnManager = require("TurnManager")

function OnInit()
    -- Spawn 3 characters
    local char1 = SpawnCharacter(5, 5, "Warrior")
    local char2 = SpawnCharacter(6, 5, "Mage")
    local char3 = SpawnCharacter(7, 5, "Rogue")

    -- Attach scripts to all
    AddScriptComponentToEntity(char1, "assets/scripts/PlayerScript.lua")
    AddScriptComponentToEntity(char2, "assets/scripts/PlayerScript.lua")
    AddScriptComponentToEntity(char3, "assets/scripts/PlayerScript.lua")

    -- Initialize party manager
    PartyManager.Init({char1, char2, char3})

    -- Initialize turn manager
    TurnManager.Init({char1, char2, char3}, enemies)
end

function OnUpdate(dt)
    TurnManager.Update(dt)
    -- When character ends turn, automatically switch to next
end
```

**PartyManager.lua:**
```lua
local PartyManager = {
    members = {},
    activeIndex = 1
}

function PartyManager.Init(characterIDs)
    PartyManager.members = characterIDs
    PartyManager.activeIndex = 1
end

function PartyManager.GetActiveCharacter()
    return PartyManager.members[PartyManager.activeIndex]
end

function PartyManager.IsActive(entityID)
    return entityID == PartyManager.GetActiveCharacter()
end

function PartyManager.NextCharacter()
    PartyManager.activeIndex = PartyManager.activeIndex + 1
    if PartyManager.activeIndex > #PartyManager.members then
        return false  -- All characters done, switch to enemies
    end
    return true
end

return PartyManager
```

**PlayerScript.lua changes:**
```lua
local PartyManager = require("PartyManager")

function OnUpdate(dt)
    -- CRITICAL: Only process input if this is the active character
    if not PartyManager.IsActive(entityID) then
        return
    end

    -- Rest of movement code unchanged...
    if moveCooldown > 0 then
        moveCooldown = moveCooldown - dt
        return
    end

    -- Normal WASD handling...
end
```

---

## 🚨 Potential Pitfalls

### 1. **Input Handling Race Condition**
**Problem:** All 3 PlayerScripts run simultaneously
**Solution:** Active character check at start of OnUpdate

### 2. **AP Consumption Confusion**
**Problem:** Consuming wrong character's AP
**Solution:** Use entity-based APIs, not "player" APIs

### 3. **Camera Jitter**
**Problem:** Instant camera switches jarring
**Solution:** Smooth lerp transitions (0.3-0.5s)

### 4. **UI Overlap**
**Problem:** 3 character UIs take up too much space
**Solution:** Compact horizontal layout, shared resources

### 5. **Turn Order Confusion**
**Problem:** Player doesn't know whose turn it is
**Solution:** Large "CHARACTER 2'S TURN" text, portrait highlight

---

## 💡 Design Recommendations

### 1. **Turn Flow**
```
Character 1 acts → Ends turn (Space/Enter)
    ↓
Camera smoothly moves to Character 2
    ↓
"CHARACTER 2'S TURN" text appears
    ↓
Character 2 acts → Ends turn
    ↓
Camera smoothly moves to Character 3
    ↓
"CHARACTER 3'S TURN" text appears
    ↓
Character 3 acts → Ends turn
    ↓
"ENEMY TURN" text appears
    ↓
All enemies act (using existing EnemyScript.lua)
    ↓
Back to Character 1
```

### 2. **UI Layout**
```
                        [ENEMY TURN]
                             ▲
┌──────────────────────────────────────────────────┐
│                                                  │
│            [Game World View]                     │
│                                                  │
└──────────────────────────────────────────────────┘
[Char1: ♥♥♥♥♥ ◆◆◆] [Char2: ♥♥♥♥♥ ◆◆◆] [Char3: ♥♥♥♥♥ ◆◆◆]
   ^HIGHLIGHTED^      ^GRAYED OUT^      ^GRAYED OUT^
```

### 3. **Character Differentiation**
- **Character 1 (Warrior):** High HP, low AP
- **Character 2 (Mage):** Low HP, high AP
- **Character 3 (Rogue):** Medium HP, medium AP, extra range

### 4. **AP System Per Character**
```lua
-- Each character has own AP config
warrior.maxAP = 3   -- Slower but tankier
mage.maxAP = 5      -- Fast, fragile
rogue.maxAP = 4     -- Balanced
```

---

## 📦 Required New Files

### Lua Scripts (4 files)
1. **PartyManager.lua** - Party state management
2. **TurnManager.lua** - Turn queue system
3. **PartyStatusUI.lua** - Multi-character UI
4. **CameraController.lua** - Smooth camera follow

### C++ API Extensions (1 file)
Update **LevelLoader_API.cpp** with:
- `Lua_GetEntityAP(entityID)`
- `Lua_ConsumeEntityAP(entityID, amount)`
- `Lua_RefillEntityAP(entityID)`
- `Lua_GetEntityHP(entityID)`
- `Lua_DamageEntity(entityID, damage)` (already exists?)
- `Lua_GetPartyMembers()`
- `Lua_SetCameraFollowTarget(entityID)`

---

## ✅ Final Assessment

### Is Your System Robust Enough?

**Short Answer: YES, with moderate work**

**Why:**
- ✅ **Entity Component Architecture** is perfect
- ✅ **PlayerScript per-entity design** is exactly what you need
- ✅ **Modular UI system** can be extended easily
- ✅ **Grid movement** already entity-aware
- ⚠️ **Turn system** needs overhaul (but doable)
- ⚠️ **APIs** need entity-based variants (straightforward)
- ⚠️ **UI** needs multi-character variant (medium effort)

**Architectural Score: 8/10**
- Core systems are solid
- Missing party-specific layer
- No major architectural changes needed
- Extensions fit naturally into existing design

### Estimated Timeline

| Phase | Duration | Can Skip? |
|-------|----------|-----------|
| **Phase 1: Core Party** | 1 day | ❌ Critical |
| **Phase 2: UI Extensions** | 0.5 day | ⚠️ Important |
| **Phase 3: Camera System** | 0.5 day | ✅ Nice-to-have |
| **Phase 4: Polish** | 0.5 day | ✅ Optional |
| **Total** | 2-3 days | - |

### Risk Assessment: **LOW**
- No major refactoring required
- Fits existing architecture
- Incremental implementation possible
- Can ship Phase 1 alone if needed

---

## 🎯 Recommendation

**YES, proceed with party system!**

Your architecture is well-designed and ready for this feature. The required changes are:
- ✅ Logical extensions (not hacks)
- ✅ Moderate effort (2-3 days)
- ✅ Low risk (no major refactoring)
- ✅ Natural fit (existing patterns support it)

**Next Steps:**
1. Start with Phase 1 (Core Party Support)
2. Test with 3 characters taking sequential turns
3. Add Phase 2 (UI) once basic flow works
4. Add Phase 3 (Camera) for polish

**Would you like me to:**
1. Create PartyManager.lua first?
2. Add the entity-based API functions?
3. Update PlayerScript.lua to respect active character?
4. Create a complete TurnManager.lua system?

Let me know which part you want to tackle first! 🚀
