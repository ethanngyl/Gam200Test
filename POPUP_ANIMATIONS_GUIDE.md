# Pop-Up & One-Time Animation Guide

This guide explains all the ways you can create one-time animations (pop-ups) in your game.

## Types of One-Time Animations Supported

### 1. **UI Pop-ups** (NEW - PopupManager)
Floating text and numbers that appear temporarily
- ✅ Damage numbers
- ✅ Heal numbers
- ✅ Status effect notifications
- ✅ Custom text messages
- ✅ Tutorial hints

### 2. **Sprite Animations** (Existing - AnimationController)
One-shot character/enemy animations
- ✅ Attack slashes
- ✅ Death animations
- ✅ Hit reactions
- ✅ Spell effects

### 3. **Tile Effects** (Existing - C++ API)
Temporary tile highlights and pulses
- ✅ Tile flashes (damage, movement)
- ✅ Tile borders (selection, targeting)

---

## 1. UI Pop-Ups with PopupManager

### Setup (Already Done in Level3Clean.lua)

```lua
local PopupManager = require("UI/PopupManager")

function OnInit()
    PopupManager.Init()
end

function OnUpdate(dt)
    PopupManager.Update(dt)
end

function OnDraw()
    PopupManager.Draw()
end

function OnDestroy()
    PopupManager.Clear()
end
```

### Usage Examples

#### Show Damage Number
```lua
-- When player takes damage from enemy
local worldX, worldY = GetEntityWorldPosition(playerID)
PopupManager.ShowDamageNumber(worldX, worldY + 0.2, 5)  -- Shows "5" in red
```

**Result:** Red number "5" floats upward for 1 second then fades out

#### Show Healing Number
```lua
-- When player uses health potion
local worldX, worldY = GetEntityWorldPosition(playerID)
PopupManager.ShowHealNumber(worldX, worldY + 0.2, 10)  -- Shows "+10" in green
```

**Result:** Green "+10" floats upward

#### Show Custom Text
```lua
-- Critical hit message
local color = {r = 1.0, g = 0.8, b = 0.0}  -- Gold color
PopupManager.ShowText(x, y, "Critical Hit!", color, 1.5)  -- Shows for 1.5 seconds
```

#### Show Status Effect
```lua
-- When enemy is poisoned
PopupManager.ShowStatusEffect(enemyX, enemyY, "Poisoned")  -- Shows "Poisoned" in green
PopupManager.ShowStatusEffect(enemyX, enemyY, "Stunned")   -- Shows "Stunned" in yellow
PopupManager.ShowStatusEffect(enemyX, enemyY, "Burning")   -- Shows "Burning" in orange
PopupManager.ShowStatusEffect(enemyX, enemyY, "Frozen")    // Shows "Frozen" in blue
```

#### Tutorial Hints
```lua
-- Show hint when player first moves
PopupManager.ShowText(playerX, playerY - 0.5, "Use WASD to move!",
    {r = 1.0, g = 1.0, b = 1.0}, 3.0)  -- White text for 3 seconds
```

### Customization

#### Adjust Float Speed & Lifetime
Edit `PopupManager.lua` defaults:

```lua
local popup = {
    lifetime = 1.0,      -- How long popup exists (seconds)
    floatSpeed = 0.3,    -- How fast it moves upward (units/second)
    fadeDelay = 0.3,     -- When to start fading (seconds)
    color = {r = 1.0, g = 0.2, b = 0.2}
}
```

#### Predefined Colors
```lua
local RED = {r = 1.0, g = 0.0, b = 0.0}
local GREEN = {r = 0.0, g = 1.0, b = 0.0}
local BLUE = {r = 0.0, g = 0.5, b = 1.0}
local YELLOW = {r = 1.0, g = 1.0, b = 0.0}
local ORANGE = {r = 1.0, g = 0.5, b = 0.0}
local PURPLE = {r = 0.8, g = 0.2, b = 1.0}
local WHITE = {r = 1.0, g = 1.0, b = 1.0}
```

---

## 2. One-Shot Sprite Animations

### In AnimationController

The AnimationController automatically handles sprite animations. For one-shot animations:

```json
// In animations.json
{
  "Attack_front": {
    "sprite": "assets/Warrior/FrontView/WarriorAttackFront.png",
    "group": "Attack",
    "direction": "Front",
    "rows": 3,
    "columns": 3,
    "frameCount": 6,
    "frameTime": 0.08,
    "loop": false  // ← ONE-TIME ANIMATION
  }
}
```

### Triggering One-Shot Animations

```lua
-- In PlayerScript.lua or EnemyScript.lua
function AttackEnemy(targetX, targetY)
    -- Play attack animation
    SetAnimationGroup(entityID, 2)      -- 2 = Attack enum
    SetAnimationLoop(entityID, false)   // Play once

    -- Animation plays automatically, then returns to Idle
    DealDamage(targetX, targetY)
end
```

### Animation Completion Callbacks

To run code when animation finishes, you'd need to poll the animation state:

```lua
local isPlayingAttack = false

function OnUpdate(dt)
    if isPlayingAttack then
        local currentGroup = GetAnimationGroup(entityID)
        if currentGroup ~= AnimGroup.Attack then
            -- Attack animation finished
            isPlayingAttack = false
            OnAttackComplete()
        end
    end
end

function TriggerAttack()
    SetAnimationGroup(entityID, AnimGroup.Attack)
    SetAnimationLoop(entityID, false)
    isPlayingAttack = true
end
```

---

## 3. Tile Effects (Temporary Visual Feedback)

### PulseTile - Flash a Tile

```lua
PulseTile(x, y, duration, r, g, b)
```

**Examples:**
```lua
-- Red flash when taking damage
PulseTile(playerX, playerY, 0.3, 1.0, 0.0, 0.0)

-- Green flash when healing
PulseTile(playerX, playerY, 0.5, 0.0, 1.0, 0.0)

-- Blue flash for mana restore
PulseTile(playerX, playerY, 0.4, 0.2, 0.5, 1.0)

-- Yellow flash for item pickup
PulseTile(itemX, itemY, 0.6, 1.0, 1.0, 0.0)

-- Orange flash for enemy movement
PulseTile(nextTile.x, nextTile.y, 0.2, 1.0, 0.5, 0.0)
```

### ShowTileBorder - Highlight Tile Edge

```lua
ShowTileBorder(x, y, duration)
```

**Examples:**
```lua
-- Highlight movement target
ShowTileBorder(targetX, targetY, 0.5)

// Show attack range
for x = -2, 2 do
    for y = -2, 2 do
        if IsInAttackRange(playerX + x, playerY + y) then
            ShowTileBorder(playerX + x, playerY + y, 1.0)
        end
    end
end
```

---

## Complete Integration Examples

### Example 1: Player Attack with Full Feedback

```lua
-- In PlayerScript.lua
function ExecuteAttack(targetX, targetY)
    -- 1. Play attack animation (one-shot sprite animation)
    SetAnimationGroup(entityID, AnimGroup.Attack)
    SetAnimationLoop(entityID, false)

    -- 2. Highlight target tile
    ShowTileBorder(targetX, targetY, 0.5)

    -- 3. Deal damage
    local damage = CalculateDamage()
    DamageEntity(targetEntity, damage)

    -- 4. Show damage number popup
    local worldX, worldY = GetEntityWorldPosition(targetEntity)
    PopupManager.ShowDamageNumber(worldX, worldY + 0.2, damage)

    -- 5. Flash target tile red
    PulseTile(targetX, targetY, 0.3, 1.0, 0.0, 0.0)
end
```

### Example 2: Healing Spell with Effects

```lua
function CastHealSpell(targetPlayerID)
    local casterX, casterY = GetEntityGridPosition(entityID)
    local targetX, targetY = GetEntityGridPosition(targetPlayerID)

    -- 1. Play spell cast animation
    SetAnimationGroup(entityID, AnimGroup.Attack)  // Or create "Cast" group
    SetAnimationLoop(entityID, false)

    -- 2. Show casting effect
    local worldX, worldY = GetEntityWorldPosition(entityID)
    PopupManager.ShowText(worldX, worldY + 0.3, "Casting...",
        {r = 0.5, g = 0.8, b = 1.0}, 0.5)

    -- 3. Heal target
    local healAmount = 10
    HealEntity(targetPlayerID, healAmount)

    -- 4. Show heal number
    local targetWorldX, targetWorldY = GetEntityWorldPosition(targetPlayerID)
    PopupManager.ShowHealNumber(targetWorldX, targetWorldY + 0.2, healAmount)

    -- 5. Green pulse on target
    PulseTile(targetX, targetY, 0.5, 0.2, 1.0, 0.2)
end
```

### Example 3: Level Up Notification

```lua
function OnLevelUp(newLevel)
    local worldX, worldY = GetEntityWorldPosition(entityID)

    -- Big level up message
    PopupManager.ShowText(worldX, worldY + 0.4, "LEVEL UP!",
        {r = 1.0, g = 0.8, b = 0.0}, 2.0)  -- Gold color, 2 seconds

    -- Show new level
    PopupManager.ShowText(worldX, worldY + 0.2, "Level " .. newLevel,
        {r = 1.0, g = 1.0, b = 1.0}, 2.0)  -- White, 2 seconds

    -- Golden tile flash
    local gridX, gridY = GetEntityGridPosition(entityID)
    PulseTile(gridX, gridY, 1.0, 1.0, 0.8, 0.0)
end
```

### Example 4: Status Effect Application

```lua
function ApplyPoison(targetID, duration)
    local targetX, targetY = GetEntityGridPosition(targetID)
    local worldX, worldY = GetEntityWorldPosition(targetID)

    -- Show status effect popup
    PopupManager.ShowStatusEffect(worldX, worldY + 0.2, "Poisoned")

    -- Green/purple tile pulse
    PulseTile(targetX, targetY, 0.8, 0.5, 1.0, 0.2)

    -- Store status effect data
    ApplyStatusToEntity(targetID, "poison", duration)
end
```

---

## Advanced: Animation Sequencing

For complex sequences, create a coroutine-style system:

```lua
-- Example: Multi-step ability animation
function ExecuteMultiHitAbility(targetID)
    local targetX, targetY = GetEntityGridPosition(targetID)
    local worldX, worldY = GetEntityWorldPosition(targetID)

    -- Hit 1
    PulseTile(targetX, targetY, 0.2, 1.0, 0.5, 0.0)
    PopupManager.ShowDamageNumber(worldX, worldY + 0.2, 3)

    -- Wait 0.3 seconds, Hit 2
    -- (You'd need a timer system for this)
    -- PulseTile(targetX, targetY, 0.2, 1.0, 0.3, 0.0)
    // PopupManager.ShowDamageNumber(worldX, worldY + 0.3, 3)

    // Final hit
    // PulseTile(targetX, targetY, 0.4, 1.0, 0.0, 0.0)
    // PopupManager.ShowDamageNumber(worldX, worldY + 0.4, 5)
    // PopupManager.ShowText(worldX, worldY + 0.6, "COMBO!", {r=1,g=1,b=0}, 1.0)
end
```

---

## Performance Considerations

### PopupManager Limits
- Each popup is lightweight (just text rendering)
- No hard limit, but keep under 50 active popups for performance
- Automatically removes expired popups

### Best Practices
1. **Damage Numbers**: One per hit (not per frame)
2. **Status Effects**: One when applied, not continuously
3. **Text Messages**: Use sparingly for important events
4. **Tile Effects**: Short duration (0.2-0.5 seconds typically)

---

## Debugging

### Check Active Popups
```lua
local count = PopupManager.GetActiveCount()
print("Active popups: " .. count)
```

### Clear All Popups
```lua
PopupManager.Clear()  -- Useful for testing or level transitions
```

### Test Popup Positioning
```lua
-- Test at player position
local x, y = GetEntityWorldPosition(playerID)
print("Player world position: " .. x .. ", " .. y)
PopupManager.ShowText(x, y, "TEST", {r=1, g=1, b=1}, 3.0)
```

---

## Summary

| Animation Type | Use Case | Duration | Example |
|----------------|----------|----------|---------|
| **Damage Numbers** | Combat feedback | 1.0s | `ShowDamageNumber(x, y, 5)` |
| **Heal Numbers** | Healing feedback | 1.0s | `ShowHealNumber(x, y, 10)` |
| **Status Text** | Status effects | 2.0s | `ShowStatusEffect(x, y, "Poisoned")` |
| **Custom Text** | Any message | Variable | `ShowText(x, y, "Level Up!", color, 2.0)` |
| **Sprite Anims** | Character actions | Variable | `SetAnimationGroup(id, Attack)` |
| **Tile Pulse** | Quick feedback | 0.2-0.5s | `PulseTile(x, y, 0.3, 1, 0, 0)` |
| **Tile Border** | Selection/targeting | 0.5-1.0s | `ShowTileBorder(x, y, 0.5)` |

All these systems work together to create rich visual feedback for your turn-based combat system!
