--[[
===============================================================================
File:        SkillPatterns.lua
Author:      AI Assistant
Date:        2026-02-07
------------------------------------------------------------------------------
Skill Attack Pattern Definitions

Purpose:
    Defines reusable attack range patterns for the skill system.
    Each pattern returns a list of tile offsets relative to the caster.

Usage:
    local pattern = SkillPatterns.GetPattern("adjacent", direction)
    for _, offset in ipairs(pattern) do
        local targetX = casterX + offset.x
        local targetY = casterY + offset.y
        -- Attack this tile
    end

Supported Patterns:
    - single: Single target tile
    - adjacent: 4 adjacent tiles (up/down/left/right)
    - cross: Same as adjacent (all 4 directions)
    - line: Straight line in facing direction
    - pierce: Line that hits all enemies (doesn't stop)
    - cone: 3 tiles in front (90-degree arc)
    - area3x3: 3x3 grid centered on target
    - area5x5: 5x5 grid centered on target
    - diagonal: 4 diagonal tiles
    - knight: L-shaped (like chess knight)

Copyright (C) 2026 DigiPen Institute of Technology.
===============================================================================
]]--

SkillPatterns = {}

-- Direction enums (matching AnimDirection)
local Direction = {
    Front = 0,  -- Down
    Back = 1,   -- Up
    Side = 2,   -- Left/Right (use flipX)
    None = 3
}

--[[
    GetPattern(patternType, direction, range, flipX)

    @param patternType string - Pattern name (e.g., "adjacent", "line", "cone")
    @param direction number - Facing direction (0=Front, 1=Back, 2=Side)
    @param range number - Range of the skill (default 1)
    @param flipX boolean - For Side direction, true=left, false=right
    @return table - Array of {x, y} offsets
]]--
function SkillPatterns.GetPattern(patternType, direction, range, flipX)
    if range == nil then range = 1 end
    flipX = flipX or false
    direction = direction or Direction.Front

    if patternType == "single" then
        return SkillPatterns.Single(direction, range, flipX)

    elseif patternType == "adjacent" or patternType == "cross" then
        return SkillPatterns.Adjacent()

    elseif patternType == "line" then
        return SkillPatterns.Line(direction, range, flipX)

    elseif patternType == "pierce" then
        return SkillPatterns.Pierce(direction, range, flipX)

    elseif patternType == "cone" then
        return SkillPatterns.Cone(direction, range, flipX)

    elseif patternType == "area3x3" then
        return SkillPatterns.Area3x3(direction, range, flipX)

    elseif patternType == "area5x5" then
        return SkillPatterns.Area5x5(direction, range, flipX)

    elseif patternType == "area5x5_self" then
        return SkillPatterns.Area5x5Self()

    elseif patternType == "diagonal" then
        return SkillPatterns.Diagonal()

    elseif patternType == "knight" then
        return SkillPatterns.Knight()

    else
        print("[SkillPatterns] Unknown pattern: " .. tostring(patternType) .. ", using 'adjacent'")
        return SkillPatterns.Adjacent()
    end
end

-- Single target in facing direction
function SkillPatterns.Single(direction, range, flipX)
    local dx, dy = SkillPatterns.GetDirectionVector(direction, flipX)
    return {{x = dx * range, y = dy * range}}
end

-- 4 adjacent tiles (cardinal directions)
function SkillPatterns.Adjacent()
    return {
        {x = 0, y = -1},  -- Up
        {x = 0, y = 1},   -- Down
        {x = -1, y = 0},  -- Left
        {x = 1, y = 0}    -- Right
    }
end

-- Line in facing direction
function SkillPatterns.Line(direction, range, flipX)
    local tiles = {}
    local dx, dy = SkillPatterns.GetDirectionVector(direction, flipX)

    for i = 1, range do
        table.insert(tiles, {x = dx * i, y = dy * i})
    end

    return tiles
end

-- Pierce (line that hits all tiles, doesn't stop at first enemy)
function SkillPatterns.Pierce(direction, range, flipX)
    return SkillPatterns.Line(direction, range, flipX)
end

-- Cone (3 tiles in front, 90-degree arc)
function SkillPatterns.Cone(direction, range, flipX)
    local tiles = {}

    if direction == Direction.Front then  -- Facing down (-Y)
        table.insert(tiles, {x = 0, y = -range})    -- Center
        table.insert(tiles, {x = -1, y = -range})   -- Left
        table.insert(tiles, {x = 1, y = -range})    -- Right

    elseif direction == Direction.Back then  -- Facing up (+Y)
        table.insert(tiles, {x = 0, y = range})     -- Center
        table.insert(tiles, {x = -1, y = range})    -- Left
        table.insert(tiles, {x = 1, y = range})     -- Right

    elseif direction == Direction.Side then  -- Facing left/right
        if flipX then  -- Facing right (+X)
            table.insert(tiles, {x = range, y = 0})    -- Center
            table.insert(tiles, {x = range, y = -1})   -- Up
            table.insert(tiles, {x = range, y = 1})     -- Down
        else  -- Facing left (-X)
            table.insert(tiles, {x = -range, y = 0})   -- Center
            table.insert(tiles, {x = -range, y = -1})  -- Up
            table.insert(tiles, {x = -range, y = 1})    -- Down
        end
    end

    return tiles
end

-- 3x3 area: range=0 = centered on caster (self), range>0 = centered on tile in facing direction
function SkillPatterns.Area3x3(direction, range, flipX)
    local centerX, centerY
    if range == 0 or range == nil then
        centerX, centerY = 0, 0  -- Centered on self
    else
        local dx, dy = SkillPatterns.GetDirectionVector(direction, flipX)
        centerX = dx * range
        centerY = dy * range
    end

    local tiles = {}
    for offsetY = -1, 1 do
        for offsetX = -1, 1 do
            table.insert(tiles, {x = centerX + offsetX, y = centerY + offsetY})
        end
    end

    return tiles
end

-- 5x5 area centered on target
function SkillPatterns.Area5x5(direction, range, flipX)
    local dx, dy = SkillPatterns.GetDirectionVector(direction, flipX)
    local centerX = dx * range
    local centerY = dy * range

    local tiles = {}
    for offsetY = -2, 2 do
        for offsetX = -2, 2 do
            table.insert(tiles, {x = centerX + offsetX, y = centerY + offsetY})
        end
    end

    return tiles
end

-- 5x5 area centered on caster (self)
function SkillPatterns.Area5x5Self()
    local tiles = {}
    for offsetY = -2, 2 do
        for offsetX = -2, 2 do
            if offsetX ~= 0 or offsetY ~= 0 then  -- exclude caster's own tile
                table.insert(tiles, {x = offsetX, y = offsetY})
            end
        end
    end
    return tiles
end

-- 4 diagonal tiles
function SkillPatterns.Diagonal()
    return {
        {x = -1, y = -1},  -- Top-left
        {x = 1, y = -1},   -- Top-right
        {x = -1, y = 1},   -- Bottom-left
        {x = 1, y = 1}     -- Bottom-right
    }
end

-- Knight move (L-shaped like chess)
function SkillPatterns.Knight()
    return {
        {x = -2, y = -1}, {x = -2, y = 1},  -- Left 2, up/down 1
        {x = 2, y = -1}, {x = 2, y = 1},    -- Right 2, up/down 1
        {x = -1, y = -2}, {x = 1, y = -2},  -- Up 2, left/right 1
        {x = -1, y = 2}, {x = 1, y = 2}     -- Down 2, left/right 1
    }
end

-- Helper: Convert direction enum to vector
function SkillPatterns.GetDirectionVector(direction, flipX)
    if direction == Direction.Front then
        return 0, -1  -- Down (Front = facing camera = -Y)
    elseif direction == Direction.Back then
        return 0, 1   -- Up (Back = facing away = +Y)
    elseif direction == Direction.Side then
        if flipX then
            return 1, 0   -- Right (flipX=true = facing right)
        else
            return -1, 0  -- Left (flipX=false = facing left)
        end
    else
        return 0, -1  -- Default to down
    end
end

return SkillPatterns
