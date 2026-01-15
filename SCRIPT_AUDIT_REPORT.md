# Script Usage Audit Report
Generated: 2026-01-14

## Executive Summary
✅ **All critical systems are properly scripted**
⚠️ **One unused script detected** (Level3.lua)
✅ **No C++ duplication** - Systems properly segregated

---

## 1. Active Scripts (Currently Used in Game)

### Level Scripts (Loaded by GameStateManager.cpp)
| Script | Status | Used By | Purpose |
|--------|--------|---------|---------|
| **MainMenuLevel.lua** | ✅ Active | GSM Line 82 | Main menu with buttons |
| **LevelSelectLevel.lua** | ✅ Active | GSM Line 178 | Level selection screen |
| **TutorialLevel.lua** | ✅ Active | GSM Line 509 | Tutorial level |
| **Level2.lua** | ✅ Active | GSM Line 268 | Level 2 gameplay |
| **Level3Clean.lua** | ✅ Active | GSM Line 300 | Level 3 gameplay (CURRENT) |
| **EndLevel.lua** | ✅ Active | Transitions | End game screen |

### Entity Component Scripts (Attached via AddScriptComponentToEntity)
| Script | Status | Attached To | Purpose |
|--------|--------|-------------|---------|
| **PlayerScript.lua** | ✅ Active | Player Entity | Grid-based player movement |
| **EnemyScript.lua** | ✅ Active | Enemy Entities | Turn-based AI behavior |

### UI System Scripts (Required by UIManager)
| Script | Status | Used By | Purpose |
|--------|--------|---------|---------|
| **UIManager.lua** | ✅ Active | Level3Clean | Central UI coordinator |
| **UIComponent.lua** | ✅ Active | All UI scripts | Base UI class |
| **APIndicatorUI.lua** | ✅ Active | UIManager | Movement AP crystals |
| **HealthUI.lua** | ✅ Active | UIManager | HP heart display |
| **TurnIndicatorUI.lua** | ✅ Active | UIManager | Player/Enemy turn indicator |

### Utility Scripts
| Script | Status | Used By | Purpose |
|--------|--------|---------|---------|
| **PauseMenu.lua** | ✅ Active | Level3/Level3Clean | Pause menu system |
| **ButtonManager.lua** | ✅ Active | All menu levels | Button creation/handling |
| **MapGenerator.lua** | ✅ Active | MapGeneratorDemo | Procedural map generation |
| **MapGeneratorDemo.lua** | ✅ Active | Manual testing | Map gen testing interface |

---

## 2. Unused Scripts (Can Be Deleted)

### ⚠️ Level3.lua (1050 lines)
**Status:** UNUSED - Replaced by Level3Clean.lua
**Action Required:** DELETE or ARCHIVE

**Evidence:**
```cpp
// GameStateManager.cpp:300
bool success = loader.LoadLevel("assets/scripts/Level3Clean.lua", g_loadAsEditorMode);
```

**Reason:**
- Level3Clean.lua (311 lines) replaced Level3.lua (1050 lines)
- 70% code reduction through modular UI architecture
- All functionality moved to UIManager + component scripts
- Level3.lua is outdated and unmaintained

**Recommendation:**
```bash
# Archive for reference
mkdir -p assets/scripts/archive
mv assets/scripts/Level3.lua assets/scripts/archive/

# Or delete if not needed
rm assets/scripts/Level3.lua
```

---

### ℹ️ TestScript.lua
**Status:** TEST ONLY - Not loaded in production
**Action Required:** KEEP (useful for debugging)

**Purpose:**
- Simple test script for Script Browser feature
- Useful for debugging script attachment
- Safe to keep in `assets/scripts/` for development

---

## 3. C++ Systems Analysis (No Duplication Detected)

### ✅ MovementSystem (src/Movement/MovementSystem.cpp)
**Status:** ACTIVE - Not redundant
**Reason:** Handles non-scripted entities only

**Evidence:**
```cpp
// Line 92-94: Skips scripted entities
if (entityManager->HasComponent<ScriptComponent>(entity)) {
    continue;  // PlayerScript.lua handles movement
}
```

**Purpose:**
- Free-form WASD movement for non-grid entities
- Animation flipping, scaling, rotation
- Only processes entities WITHOUT ScriptComponent
- **Conclusion:** NOT redundant with PlayerScript.lua

---

### ✅ PlayerControllerSystem (src/Player/PlayerManager.cpp)
**Status:** ACTIVE - Controlled by Lua
**Reason:** Grid movement disabled when PlayerScript attached

**Evidence:**
```lua
-- Level3Clean.lua:232
SetGridMovementEnabled(false)  -- Disable C++ grid movement
AddScriptComponentToEntity(playerID, "assets/scripts/PlayerScript.lua")
```

**Purpose:**
- Grid-based click-to-move (disabled for Level3)
- Shooting/attack mechanics
- Tile interaction handling
- **Conclusion:** Complementary to PlayerScript, not duplicate

---

### ✅ Other Core Systems (No Action Needed)
| System | Status | Reason |
|--------|--------|--------|
| CollisionSystem | Keep | Core physics, not scriptable |
| AudioSystem | Keep | Low-level audio, exposed via Lua API |
| GraphicsSystemV2 | Keep | Rendering pipeline, not game logic |
| PathfindingSystem | Keep | Algorithm backend, called from Lua |
| EventSystem | Keep | Message bus, used by all systems |
| AnimationSystem | Keep | Frame timing, not game logic |

---

## 4. Script Architecture Quality Report

### ✅ Excellent Separation of Concerns
```
Level Scripts (Level3Clean.lua - 311 lines)
    ↓ requires
UIManager.lua (202 lines)
    ↓ requires
UI Components (APIndicatorUI, HealthUI, TurnIndicatorUI)
    ↓ inherits from
UIComponent.lua (base class)
```

**Benefits:**
- **Single Responsibility Principle** ✅
- **Modularity** ✅
- **Reusability** ✅
- **Maintainability** ✅

---

### ✅ Entity Component Scripts Pattern
```
Entities (Player, Enemies)
    ↓ AttachScript
Component Scripts (PlayerScript.lua, EnemyScript.lua)
    ↓ uses
Lua API (MoveEntityToTile, DamageEntity, etc.)
    ↓ calls
C++ Systems (Grid, Combat, Pathfinding)
```

**Benefits:**
- Hot-reloadable gameplay logic
- Per-entity behavior customization
- No C++ recompilation for balance changes

---

## 5. Recommended Actions

### Immediate Actions
1. **Delete Level3.lua** (or archive it)
   ```bash
   mv assets/scripts/Level3.lua assets/scripts/archive/
   ```

2. **Verify Level3Clean.lua is working correctly**
   - Test all UI elements render
   - Test player movement
   - Test enemy AI
   - Test pause menu

### Future Enhancements (Optional)
1. **Combat System Script** - Extract combat logic into CombatSystem.lua
2. **Inventory Manager Script** - Handle item pickups, chest opening
3. **Ability System Script** - Special attacks, cooldowns
4. **Turn Manager Script** - Centralized turn logic

### Code Quality (Already Excellent)
- ✅ All scripts follow consistent naming conventions
- ✅ Proper use of `require()` for module dependencies
- ✅ Clean separation between level, UI, and entity scripts
- ✅ C++ systems skip scripted entities (no duplication)

---

## 6. Script Loading Flow

```
Game Start
    ↓
GameStateManager.cpp loads MainMenuLevel.lua
    ↓ (Player clicks "Play")
Loads LevelSelectLevel.lua
    ↓ (Player selects Level 3)
Loads Level3Clean.lua
    ↓ OnInit()
    ├─ LoadTileMap()
    ├─ AddScriptComponentToEntity(player, PlayerScript.lua)
    ├─ AddScriptComponentToEntity(enemies, EnemyScript.lua)
    └─ UIManager.Init() → loads UI components
        ↓
    Level3Clean OnUpdate() called every frame
        ├─ UIManager.Update()
        ├─ PauseMenu.Update()
        └─ Level logic
```

---

## 7. Conclusion

### ✅ All Scripts Are Properly Used
- Every script except Level3.lua is actively loaded
- No dead code in production scripts
- Clean module dependency tree

### ✅ No C++ Duplication
- MovementSystem skips scripted entities
- PlayerControllerSystem grid movement disabled via Lua
- Core systems remain in C++ (physics, rendering, audio)
- Game logic properly in Lua scripts

### ✅ Architecture Quality: EXCELLENT
- Modular UI component system
- Entity component scripts pattern
- Clean separation of concerns
- Hot-reloadable gameplay

### 🎯 Final Recommendation
**DELETE** `Level3.lua` and you'll have a perfectly clean, efficient script architecture with zero redundancy!

---

## Appendix: Full Script Dependency Graph

```
MainMenuLevel.lua
    └─ ButtonManager.lua

LevelSelectLevel.lua
    └─ ButtonManager.lua

TutorialLevel.lua
    └─ ButtonManager.lua

Level2.lua
    └─ (standalone)

Level3Clean.lua
    ├─ PauseMenu.lua
    └─ UIManager.lua
        ├─ APIndicatorUI.lua
        │   └─ UIComponent.lua
        ├─ HealthUI.lua
        │   └─ UIComponent.lua
        └─ TurnIndicatorUI.lua
            └─ UIComponent.lua

EndLevel.lua
    └─ ButtonManager.lua

MapGeneratorDemo.lua
    ├─ MapGenerator.lua
    └─ ButtonManager.lua

PlayerScript.lua (entity component)
    └─ (uses C++ Lua API)

EnemyScript.lua (entity component)
    └─ (uses C++ Lua API)

TestScript.lua (debug only)
    └─ (standalone)
```

