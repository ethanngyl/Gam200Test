--[[
===============================================================================
File:        SkillSystem.lua
Author:      AI Assistant
Date:        2026-02-07
------------------------------------------------------------------------------
Skill System - Combat Skill Manager

Purpose:
    Centralized system for loading, managing, and executing combat skills.
    Skills are defined in Skills.json with configurable damage, range, and patterns.

Features:
    - Load skills from JSON configuration
    - Execute skills with automatic range calculation
    - Visual attack preview (tile highlighting)
    - Damage calculation and application
    - Animation triggering (future)
    - Sound effects (future)

Usage in PlayerScript:
    -- Load the system
    dofile("assets/scripts/SkillSystem.lua")
    SkillSystem.LoadSkills()

    -- Get skill data
    local skill = SkillSystem.GetSkill("Fireball")

    -- Preview attack range
    SkillSystem.PreviewSkill(entityID, "Fireball")

    -- Execute skill
    SkillSystem.ExecuteSkill(entityID, "Fireball")

Copyright (C) 2026 DigiPen Institute of Technology.
===============================================================================
]]--

-- Load pattern system
dofile("assets/scripts/SkillPatterns.lua")

SkillSystem = {}
SkillSystem.skills = {}
SkillSystem.previewTiles = {}

--[[
    LoadSkills()
    Loads all skill definitions from Skills.json
]]--
function SkillSystem.LoadSkills()
    print("[SkillSystem] Loading skills from JSON...")

    local jsonPath = "assets/JSON/Skills.json"
    local data = LoadJSONFile(jsonPath)

    if not data or not data.skills then
        print("[SkillSystem] ERROR: Failed to load Skills.json or missing 'skills' field")
        return false
    end

    SkillSystem.skills = data.skills
    local count = 0
    for skillId, skillData in pairs(SkillSystem.skills) do
        count = count + 1
        print("[SkillSystem]   Loaded skill: " .. skillId .. " - " .. (skillData.name or "Unnamed"))
    end

    print("[SkillSystem] Successfully loaded " .. count .. " skills")
    return true
end

--[[
    GetSkill(skillId)
    Returns skill data for the given skill ID

    @param skillId string - Skill identifier (e.g., "Fireball")
    @return table - Skill data or nil
]]--
function SkillSystem.GetSkill(skillId)
    return SkillSystem.skills[skillId]
end

--[[
    GetSkillRange(entityID, skillId)
    Calculates and returns the attack tiles for a skill

    @param entityID number - Entity performing the skill
    @param skillId string - Skill identifier
    @return table - Array of {x, y} grid coordinates
]]--
function SkillSystem.GetSkillRange(entityID, skillId)
    local skill = SkillSystem.GetSkill(skillId)
    if not skill then
        print("[SkillSystem] ERROR: Skill not found: " .. tostring(skillId))
        return {}
    end

    -- Get caster position
    local casterX, casterY = GetEntityGridPosition(entityID)
    if not casterX or not casterY then
        print("[SkillSystem] ERROR: Could not get entity position")
        return {}
    end

    -- Get caster facing direction (use animation direction)
    local direction = 0  -- Default to Front
    local flipX = false

    if GetAnimationDirection then
        direction = GetAnimationDirection(entityID)
    end

    if GetAnimationFlipX then
        flipX = GetAnimationFlipX(entityID)
    end

    -- Get pattern offsets
    local pattern = SkillPatterns.GetPattern(skill.pattern, direction, skill.range, flipX)

    -- Convert offsets to absolute coordinates
    local targetTiles = {}
    for _, offset in ipairs(pattern) do
        local targetX = casterX + offset.x
        local targetY = casterY + offset.y
        table.insert(targetTiles, {x = targetX, y = targetY})
    end

    return targetTiles
end

--[[
    PreviewSkill(entityID, skillId)
    Shows visual preview of skill range (highlights tiles)

    @param entityID number - Entity performing the skill
    @param skillId string - Skill identifier
]]--
function SkillSystem.PreviewSkill(entityID, skillId)
    -- Clear previous preview
    SkillSystem.ClearPreview()

    local targetTiles = SkillSystem.GetSkillRange(entityID, skillId)

    -- Highlight each tile
    for _, tile in ipairs(targetTiles) do
        if PulseTile then
            -- Red highlight for attack range
            PulseTile(tile.x, tile.y, 0.3, 1.0, 0.0, 0.0)
            table.insert(SkillSystem.previewTiles, tile)
        end
    end

    print("[SkillSystem] Previewing skill: " .. skillId .. " (" .. #targetTiles .. " tiles)")
end

--[[
    ClearPreview()
    Clears skill range preview
]]--
function SkillSystem.ClearPreview()
    -- Preview tiles automatically fade out due to PulseTile duration
    SkillSystem.previewTiles = {}
end

--[[
    ExecuteSkill(entityID, skillId)
    Executes the skill, dealing damage to enemies in range

    @param entityID number - Entity performing the skill
    @param skillId string - Skill identifier
    @return boolean - True if skill was executed successfully
]]--
function SkillSystem.ExecuteSkill(entityID, skillId)
    local skill = SkillSystem.GetSkill(skillId)
    if not skill then
        print("[SkillSystem] ERROR: Skill not found: " .. tostring(skillId))
        return false
    end

    print("[SkillSystem] ========================================")
    print("[SkillSystem] Executing skill: " .. skill.name)
    print("[SkillSystem] ========================================")

    -- Check AP cost
    local currentAP, maxAP = GetEntityAttackAP(entityID)
    if not currentAP or currentAP < skill.apCost then
        print("[SkillSystem] ERROR: Insufficient AP. Need " .. skill.apCost .. ", have " .. (currentAP or 0))
        return false
    end

    -- Get target tiles
    local targetTiles = SkillSystem.GetSkillRange(entityID, skillId)

    -- Find and damage enemies in target tiles
    local enemiesHit = 0
    for _, tile in ipairs(targetTiles) do
        local targetEntityID = GetTileOccupant(tile.x, tile.y)

        if targetEntityID and targetEntityID ~= 0 then
            -- Check if it's an enemy (has EnemyAI or BoxCollider)
            local isEnemy = false

            -- Try to get HP to confirm it's a damageable entity
            local hp, maxHP = GetEntityHP(targetEntityID)
            if hp and maxHP then
                isEnemy = true
            end

            if isEnemy then
                -- Deal damage
                print("[SkillSystem]   Hitting enemy " .. targetEntityID .. " at (" .. tile.x .. "," .. tile.y .. ")")
                print("[SkillSystem]   Damage: " .. skill.damage)

                DamageEntity(targetEntityID, skill.damage)

                -- Visual feedback
                if PulseTile then
                    PulseTile(tile.x, tile.y, 0.5, 1.0, 0.0, 0.0)  -- Red flash
                end

                enemiesHit = enemiesHit + 1
            end
        end
    end

    -- Consume AP
    ConsumeEntityAttackAP(entityID, skill.apCost)
    print("[SkillSystem]   AP consumed: " .. skill.apCost)
    print("[SkillSystem]   Enemies hit: " .. enemiesHit)

    -- Play animation (future)
    if skill.animationName and SetAnimationGroup then
        SetAnimationGroup(entityID, 2)  -- 2 = Attack
        SetAnimationLoop(entityID, false)
    end

    -- Play sound effect (future)
    if skill.soundEffect and PlaySound then
        -- PlaySound(skill.soundEffect, false, 0.7)
    end

    print("[SkillSystem] Skill execution complete")
    print("[SkillSystem] ========================================")

    return true
end

--[[
    GetSkillList()
    Returns array of all loaded skill IDs

    @return table - Array of skill ID strings
]]--
function SkillSystem.GetSkillList()
    local skillList = {}
    for skillId, _ in pairs(SkillSystem.skills) do
        table.insert(skillList, skillId)
    end
    return skillList
end

--[[
    CanUseSkill(entityID, skillId)
    Checks if entity has enough AP to use skill

    @param entityID number - Entity to check
    @param skillId string - Skill identifier
    @return boolean - True if entity can use skill
]]--
function SkillSystem.CanUseSkill(entityID, skillId)
    local skill = SkillSystem.GetSkill(skillId)
    if not skill then
        return false
    end

    local currentAP, maxAP = GetEntityAttackAP(entityID)
    return currentAP and currentAP >= skill.apCost
end

print("[SkillSystem] Module loaded successfully")
return SkillSystem
