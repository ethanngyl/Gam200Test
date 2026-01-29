# Animation Setup Guide for Party System

## Overview
This guide explains how to add character-specific animations for your 3-character party (Warrior, Mage, Rogue) with unique animations for movement, attacks, and other actions.

## Current System Architecture

The animation system works in 3 layers:

1. **JSON Config** (`animations.json`) - Defines all available animations
2. **C++ Animation System** - Loads animations and manages sprite rendering
3. **Lua AnimationController** - Switches between animations based on game state

## Two Approaches for Character-Specific Animations

### Approach 1: Separate Animation Config Files (Simplest)

Create separate JSON files for each character:

```
assets/JSON/warrior_animations.json
assets/JSON/mage_animations.json
assets/JSON/rogue_animations.json
```

**Pros:**
- Clean separation
- Easy to manage
- No C++ changes needed

**Cons:**
- Need to load different configs for each entity
- Requires LoadAnimationConfig() to support per-entity configs

#### Setup Code:
```lua
-- In SetupParty() in Level3Clean.lua
function SetupParty()
    local allPlayers = GetAllPlayers()

    local player1 = allPlayers[1]  -- Warrior
    local player2 = allPlayers[2]  -- Mage
    local player3 = allPlayers[3]  -- Rogue

    -- Load animations for each character
    LoadAnimationConfigForEntity(player1, "assets/JSON/warrior_animations.json")
    LoadAnimationConfigForEntity(player2, "assets/JSON/mage_animations.json")
    LoadAnimationConfigForEntity(player3, "assets/JSON/rogue_animations.json")

    -- Attach controllers
    AddScriptComponentToEntity(player1, "assets/scripts/AnimationController.lua")
    AddScriptComponentToEntity(player2, "assets/scripts/AnimationController.lua")
    AddScriptComponentToEntity(player3, "assets/scripts/AnimationController.lua")
end
```

### Approach 2: Single JSON with Character Prefixes (Current Setup)

All animations in one `animations.json` with naming convention:
- `{Character}_{Group}_{Direction}`
- Example: `Warrior_Walk_front`, `Mage_Attack_side`, `Rogue_Idle_back`

**Pros:**
- Single file to manage
- Can share common animations

**Cons:**
- Need C++ support for animation prefixes
- JSON file can become large

## Required Animations Per Character

Each character needs **15 animations** minimum:

| Group | Front | Back | Side |
|-------|-------|------|------|
| Idle | ✓ | ✓ | ✓ |
| Walk | ✓ | ✓ | ✓ |
| Attack | ✓ | ✓ | ✓ |
| Injured | ✓ | ✓ | ✓ |
| Death | ✓ | ✓ | ✓ |

## Animation JSON Format

```json
{
  "animations": {
    "Walk_front": {
      "sprite": "assets/Warrior/FrontView/WarriorWalkFront.png",
      "group": "Walk",
      "direction": "Front",
      "rows": 4,
      "columns": 4,
      "frameCount": 8,
      "frameTime": 0.1,
      "loop": true
    }
  }
}
```

### Properties Explained:

- **sprite**: Path to sprite sheet image
- **group**: Animation type - `"Idle"`, `"Walk"`, `"Attack"`, `"Injured"`, `"Death"`
- **direction**: Facing direction - `"Front"`, `"Back"`, `"Side"`
- **rows**: Number of rows in sprite sheet grid
- **columns**: Number of columns in sprite sheet grid
- **frameCount**: Total frames in animation
- **frameTime**: Seconds per frame (0.1 = 10 FPS, 0.05 = 20 FPS)
- **loop**: `true` for repeating animations (Idle, Walk), `false` for one-shot (Attack, Death)

## Step-by-Step: Adding New Animations

### 1. Prepare Your Sprite Sheets

Organize sprite sheets by character and direction:
```
assets/
  Warrior/
    FrontView/
      WarriorIdle.png
      WarriorWalk.png
      WarriorAttack.png
    BackView/
      WarriorIdleBack.png
      WarriorWalkBack.png
    SideView/
      WarriorIdleSide.png
      WarriorWalkSide.png
  Mage/
    FrontView/
      MageIdle.png
      MageSpellcast.png
    ...
  Rogue/
    ...
```

### 2. Add Animations to JSON

```json
{
  "animations": {
    "Mage_Idle_front": {
      "sprite": "assets/Mage/FrontView/MageIdle.png",
      "group": "Idle",
      "direction": "Front",
      "rows": 2,
      "columns": 2,
      "frameCount": 4,
      "frameTime": 0.6,
      "loop": true
    },
    "Mage_Walk_front": {
      "sprite": "assets/Mage/FrontView/MageWalk.png",
      "group": "Walk",
      "direction": "Front",
      "rows": 3,
      "columns": 3,
      "frameCount": 8,
      "frameTime": 0.12,
      "loop": true
    }
  }
}
```

### 3. Trigger Attack Animations in Combat

To play attack animations when characters attack enemies, modify PlayerScript.lua:

```lua
-- In PlayerScript.lua, when attacking an enemy:
function AttackEnemy(targetX, targetY)
    -- Switch to Attack animation
    SetAnimationGroup(entityID, 2)  -- 2 = Attack
    SetAnimationLoop(entityID, false)  -- One-shot

    -- Wait for animation to complete, then deal damage
    -- (You'll need to implement animation completion callback)
    DealDamageToTile(targetX, targetY, attackDamage)

    -- Return to Idle after attack
    -- SetAnimationGroup(entityID, 0)  -- 0 = Idle
end
```

## Animation Group Enum Reference

```lua
AnimGroup = {
    Idle = 0,
    Walk = 1,
    Attack = 2,
    Injured = 3,
    Death = 4
}

AnimDirection = {
    Front = 0,
    Back = 1,
    Side = 2,
    None = 3
}
```

## C++ API Functions

Your C++ engine provides these functions to Lua:

```cpp
// Animation configuration
LoadAnimationConfig("path/to/animations.json")
LoadAnimationConfigForEntity(entityID, "path/to/animations.json")  // If supported

// Animation control
SetAnimationGroup(entityID, groupEnum)      // 0-4 for Idle/Walk/Attack/Injured/Death
SetAnimationDirection(entityID, dirEnum)    // 0-3 for Front/Back/Side/None
SetAnimationFlipX(entityID, boolean)        // true = flip horizontally
SetAnimationLoop(entityID, boolean)         // true = repeat, false = play once
SetAnimationPlaying(entityID, boolean)      // true = play, false = pause

// Query
GetAnimationGroup(entityID)                 // Returns current group enum
```

## Testing Your Animations

### Manual Testing Keys (in AnimationController.lua):
- **K** - Trigger Attack animation
- **J** - Trigger Injured animation
- **L** - Trigger Death animation
- **WASD** - Automatically triggers Walk animations

### Debug Tips:

1. **Check sprite sheet dimensions:**
   - rows × columns should accommodate frameCount
   - Example: 4 rows × 4 columns = 16 spaces (can fit 8-16 frames)

2. **Frame timing:**
   - Walk animations: 0.08 - 0.12 seconds per frame
   - Idle animations: 0.4 - 0.6 seconds per frame
   - Attack animations: 0.05 - 0.08 seconds per frame

3. **Verify files exist:**
   ```bash
   ls -la assets/Warrior/FrontView/
   ls -la assets/Mage/FrontView/
   ls -la assets/Rogue/FrontView/
   ```

## Example: Complete Warrior Animation Set

```json
{
  "animations": {
    "Warrior_Idle_front": {...},
    "Warrior_Idle_back": {...},
    "Warrior_Idle_side": {...},

    "Warrior_Walk_front": {...},
    "Warrior_Walk_back": {...},
    "Warrior_Walk_side": {...},

    "Warrior_Attack_front": {...},
    "Warrior_Attack_back": {...},
    "Warrior_Attack_side": {...},

    "Warrior_Injured_front": {...},
    "Warrior_Injured_back": {...},
    "Warrior_Injured_side": {...},

    "Warrior_Death_front": {...},
    "Warrior_Death_back": {...},
    "Warrior_Death_side": {...}
  }
}
```

## Next Steps

1. **Organize your sprite sheets** into character/direction folders
2. **Add Walk animations** for all 3 characters (most important for WASD movement)
3. **Add Attack animations** for combat
4. **Test each character** individually
5. **Fine-tune timing** (frameTime values) for smooth animations

## Common Issues

**Problem:** Animation not playing
- Check console for "Animation not found" errors
- Verify sprite path exists
- Check group/direction values match enum

**Problem:** Wrong frames showing
- Verify rows/columns/frameCount values
- Check if sprite sheet layout matches config

**Problem:** Animation too fast/slow
- Adjust frameTime value
- Typical range: 0.05 (fast) to 0.6 (slow)

**Problem:** All characters have same animation
- Need C++ support for character-specific animation sets OR
- Use separate config files per character
