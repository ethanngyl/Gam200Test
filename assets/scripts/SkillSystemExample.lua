--[[
===============================================================================
File:        SkillSystemExample.lua
Author:      AI Assistant
Date:        2026-02-07
------------------------------------------------------------------------------
Skill System - Integration Example

This file demonstrates how to integrate the SkillSystem into your player
scripts or combat system.

QUICK START GUIDE:
=================================================================================

1. LOAD THE SKILL SYSTEM (in your level script, e.g., Level3Clean.lua)
   -------------------------------------------------------------------------
   dofile("assets/scripts/SkillSystem.lua")
   SkillSystem.LoadSkills()


2. INTEGRATE INTO PLAYERSCRIPT.LUA
   -------------------------------------------------------------------------
   Add to OnInit():

   -- Load skill system
   if not SkillSystem then
       dofile("assets/scripts/SkillSystem.lua")
       SkillSystem.LoadSkills()
   end

   -- Set default skill
   currentSkill = "BasicAttack"


3. ADD SKILL SELECTION (in OnUpdate())
   -------------------------------------------------------------------------
   -- Cycle through skills with number keys
   if IsKeyPressed(49) then  -- KEY_1
       currentSkill = "BasicAttack"
       print("[Player] Selected skill: Basic Attack")
   elseif IsKeyPressed(50) then  -- KEY_2
       currentSkill = "Slash"
       print("[Player] Selected skill: Slash")
   elseif IsKeyPressed(51) then  -- KEY_3
       currentSkill = "CrossSlash"
       print("[Player] Selected skill: Cross Slash")
   elseif IsKeyPressed(52) then  -- KEY_4
       currentSkill = "AreaBlast"
       print("[Player] Selected skill: Area Blast")
   elseif IsKeyPressed(53) then  -- KEY_5
       currentSkill = "Fireball"
       print("[Player] Selected skill: Fireball")
   end


4. REPLACE ATTACK EXECUTION
   -------------------------------------------------------------------------
   OLD CODE:

   if dKeyPressed and currentAttackAP >= 3 then
       -- Old attack logic...
       DamageEntity(targetEntityID, 1)
       ConsumeEntityAttackAP(entityID, 3)
   end

   NEW CODE:

   if dKeyPressed then
       -- Preview skill range
       SkillSystem.PreviewSkill(entityID, currentSkill)

       -- Execute skill
       local success = SkillSystem.ExecuteSkill(entityID, currentSkill)

       if success then
           print("[Player] Skill executed successfully!")
       else
           print("[Player] Failed to use skill (not enough AP?)")
       end
   end


5. OPTIONAL: SHOW SKILL PREVIEW ON HOVER
   -------------------------------------------------------------------------
   -- Show skill range when hovering over attack button
   if IsKeyDown(68) then  -- KEY_D held down
       SkillSystem.PreviewSkill(entityID, currentSkill)
   else
       SkillSystem.ClearPreview()
   end


CREATING NEW SKILLS:
=================================================================================

Edit assets/JSON/Skills.json and add a new entry:

{
  "skills": {
    "YourNewSkill": {
      "name": "Your Skill Name",
      "description": "What the skill does",
      "apCost": 4,              -- How much attack AP it costs
      "damage": 2,              -- Damage dealt
      "range": 3,               -- How far it reaches
      "pattern": "line",        -- Attack pattern (see SkillPatterns.lua)
      "animationName": "Attack",
      "soundEffect": "explosion"
    }
  }
}

AVAILABLE PATTERNS:
- "single"    : Single target tile
- "adjacent"  : 4 adjacent tiles (cross pattern)
- "line"      : Straight line in facing direction
- "pierce"    : Line that hits all enemies (doesn't stop)
- "cone"      : 3 tiles in front (90-degree arc)
- "area3x3"   : 3x3 grid centered on target
- "area5x5"   : 5x5 grid centered on target
- "diagonal"  : 4 diagonal tiles
- "knight"    : L-shaped (like chess knight)


ADDING CUSTOM PATTERNS:
=================================================================================

Edit assets/scripts/SkillPatterns.lua and add a new function:

function SkillPatterns.YourPattern(direction, range, flipX)
    local tiles = {}

    -- Example: Attack in a plus shape (+)
    table.insert(tiles, {x = 0, y = -1})  -- Up
    table.insert(tiles, {x = 0, y = 1})   -- Down
    table.insert(tiles, {x = -1, y = 0})  -- Left
    table.insert(tiles, {x = 1, y = 0})   -- Right
    table.insert(tiles, {x = 0, y = 0})   -- Center

    return tiles
end

Then add it to GetPattern():

elseif patternType == "yourpattern" then
    return SkillPatterns.YourPattern(direction, range, flipX)


FUTURE ENHANCEMENTS (Placeholder for later):
=================================================================================

1. ANIMATIONS:
   - Skills.json already has "animationName" field
   - Can trigger different attack animations per skill
   - Example: Fireball could play a casting animation

2. STATUS EFFECTS:
   - Add "statusEffect" field to skill
   - Apply burn, freeze, stun, etc.

3. COOLDOWNS:
   - Add "cooldown" field to track skill reuse
   - Store cooldown timers per entity

4. SKILL TREES:
   - Add "requiredLevel" or "prerequisiteSkill" fields
   - Lock/unlock skills based on progression

5. PARTICLE EFFECTS:
   - Add "particleEffect" field
   - Spawn visual effects on skill use

===============================================================================
]]--

-- This is just a documentation file - no actual code to run
print("[SkillSystemExample] This is a documentation/example file")
print("[SkillSystemExample] See the comments above for usage instructions")
