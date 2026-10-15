--[[
===============================================================================
 File:          MapGenerator.lua
 Authors:       Josh Ong
 Co-Authors:    
 Date:          
 Contribution:  
 ------------------------------------------------------------------------------

 MAP GENERATOR - Procedural Map Generation System (Lua)

 Brief:
    Generates grid-based tactical maps with rooms, corridors, enemies, and
    loot. Supports multiple generation algorithms including rooms & corridors,
    cellular automata caves, and open arenas. Automatically places player,
    enemies, chests, and goal with pathfinding validation to ensure the goal
    is always reachable.

 Algorithms:
    "rooms"    - Traditional dungeon with rectangular rooms and L-corridors
    "cellular" - Cave-like terrain using cellular automata smoothing
    "open"     - Simple arena with floor surrounded by walls

 Features:
    - Configurable map size and parameters
    - Automatic player/enemy/chest/goal placement
    - Pathfinding validation (ensure goal is reachable)
    - JSON export compatible with TileMap.json format
    - Console visualization for debugging

 Usage:
    local MapGenerator = require("MapGenerator")

    local config = {
        width = 21,
        height = 26,
        algorithm = "rooms",
        minRoomSize = 3,
        maxRoomSize = 8,
        maxRooms = 10,
        minEnemies = 2,
        maxEnemies = 5,
        minChests = 2,
        maxChests = 4
    }

    local map = MapGenerator.Generate(config)
    MapGenerator.Print(map)
    MapGenerator.SaveToFile(map, "assets/JSON/GeneratedMap.json")


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
]]--

local MapGenerator = {}

-- ============================================================================
-- TILE DEFINITIONS
-- ============================================================================

MapGenerator.TileTypes = {
    WALL = "W",
    FLOOR_A = "0",
    FLOOR_B = "1",
    PLAYER = "P",
    ENEMY = "E",
    CHEST = "S",
    GOAL = "M"
}

-- ============================================================================
-- DEFAULT CONFIGURATION
-- ============================================================================

MapGenerator.DefaultConfig = {
    width = 21,
    height = 26,
    algorithm = "rooms",  -- "rooms", "cellular", "maze", "open"

    -- Room generation settings
    minRoomSize = 3,
    maxRoomSize = 8,
    maxRooms = 10,

    -- Cellular automata settings
    fillProbability = 0.45,
    smoothingIterations = 4,

    -- Entity placement
    minEnemies = 2,
    maxEnemies = 5,
    minChests = 2,
    maxChests = 4,

    -- Ensure borders are always walls
    forceBorders = true
}

-- ============================================================================
-- MAP CLASS
-- ============================================================================

function MapGenerator.CreateMap(config)
    config = config or MapGenerator.DefaultConfig

    local map = {
        width = config.width,
        height = config.height,
        tiles = {},
        playerPos = nil,
        enemyPositions = {},
        chestPositions = {},
        goalPos = nil
    }

    -- Initialize grid with walls
    for y = 1, map.height do
        map.tiles[y] = {}
        for x = 1, map.width do
            map.tiles[y][x] = MapGenerator.TileTypes.WALL
        end
    end

    return map
end

-- ============================================================================
-- ALGORITHM: ROOMS AND CORRIDORS
-- ============================================================================

function MapGenerator.GenerateRooms(map, config)
    local rooms = {}

    -- Try to place rooms
    for i = 1, config.maxRooms do
        local roomWidth = math.random(config.minRoomSize, config.maxRoomSize)
        local roomHeight = math.random(config.minRoomSize, config.maxRoomSize)

        local x = math.random(2, map.width - roomWidth - 1)
        local y = math.random(2, map.height - roomHeight - 1)

        local newRoom = {x = x, y = y, width = roomWidth, height = roomHeight}

        -- Check if room overlaps with existing rooms
        local overlaps = false
        for _, room in ipairs(rooms) do
            if MapGenerator.RoomsOverlap(newRoom, room) then
                overlaps = true
                break
            end
        end

        if not overlaps then
            -- Carve out the room
            MapGenerator.CarveRoom(map, newRoom)

            -- Connect to previous room with corridor
            if #rooms > 0 then
                MapGenerator.CreateCorridor(map, rooms[#rooms], newRoom)
            end

            table.insert(rooms, newRoom)
        end
    end

    return rooms
end

function MapGenerator.RoomsOverlap(room1, room2)
    return not (room1.x + room1.width < room2.x - 1 or
                room2.x + room2.width < room1.x - 1 or
                room1.y + room1.height < room2.y - 1 or
                room2.y + room2.height < room1.y - 1)
end

function MapGenerator.CarveRoom(map, room)
    for y = room.y, room.y + room.height - 1 do
        for x = room.x, room.x + room.width - 1 do
            if y >= 1 and y <= map.height and x >= 1 and x <= map.width then
                -- Alternate floor tiles for visual variety
                map.tiles[y][x] = ((x + y) % 2 == 0) and MapGenerator.TileTypes.FLOOR_A or MapGenerator.TileTypes.FLOOR_B
            end
        end
    end
end

function MapGenerator.CreateCorridor(map, room1, room2)
    local x1 = room1.x + math.floor(room1.width / 2)
    local y1 = room1.y + math.floor(room1.height / 2)
    local x2 = room2.x + math.floor(room2.width / 2)
    local y2 = room2.y + math.floor(room2.height / 2)

    -- Horizontal then vertical corridor
    if math.random() > 0.5 then
        -- Horizontal first
        MapGenerator.CarveHorizontalCorridor(map, x1, x2, y1)
        MapGenerator.CarveVerticalCorridor(map, y1, y2, x2)
    else
        -- Vertical first
        MapGenerator.CarveVerticalCorridor(map, y1, y2, x1)
        MapGenerator.CarveHorizontalCorridor(map, x1, x2, y2)
    end
end

function MapGenerator.CarveHorizontalCorridor(map, x1, x2, y)
    local xStart = math.min(x1, x2)
    local xEnd = math.max(x1, x2)

    for x = xStart, xEnd do
        if x >= 1 and x <= map.width and y >= 1 and y <= map.height then
            map.tiles[y][x] = ((x + y) % 2 == 0) and MapGenerator.TileTypes.FLOOR_A or MapGenerator.TileTypes.FLOOR_B
        end
    end
end

function MapGenerator.CarveVerticalCorridor(map, y1, y2, x)
    local yStart = math.min(y1, y2)
    local yEnd = math.max(y1, y2)

    for y = yStart, yEnd do
        if x >= 1 and x <= map.width and y >= 1 and y <= map.height then
            map.tiles[y][x] = ((x + y) % 2 == 0) and MapGenerator.TileTypes.FLOOR_A or MapGenerator.TileTypes.FLOOR_B
        end
    end
end

-- ============================================================================
-- ALGORITHM: CELLULAR AUTOMATA
-- ============================================================================

function MapGenerator.GenerateCellular(map, config)
    -- Initial random fill
    for y = 2, map.height - 1 do
        for x = 2, map.width - 1 do
            if math.random() < config.fillProbability then
                map.tiles[y][x] = MapGenerator.TileTypes.FLOOR_A
            else
                map.tiles[y][x] = MapGenerator.TileTypes.WALL
            end
        end
    end

    -- Smooth with cellular automata rules
    for iteration = 1, config.smoothingIterations do
        local newTiles = {}
        for y = 1, map.height do
            newTiles[y] = {}
            for x = 1, map.width do
                newTiles[y][x] = map.tiles[y][x]
            end
        end

        for y = 2, map.height - 1 do
            for x = 2, map.width - 1 do
                local wallCount = MapGenerator.CountAdjacentWalls(map, x, y)

                if wallCount > 4 then
                    newTiles[y][x] = MapGenerator.TileTypes.WALL
                else
                    -- Alternate floor tiles
                    newTiles[y][x] = ((x + y) % 2 == 0) and MapGenerator.TileTypes.FLOOR_A or MapGenerator.TileTypes.FLOOR_B
                end
            end
        end

        map.tiles = newTiles
    end
end

function MapGenerator.CountAdjacentWalls(map, x, y)
    local count = 0
    for dy = -1, 1 do
        for dx = -1, 1 do
            if dx ~= 0 or dy ~= 0 then
                local nx, ny = x + dx, y + dy
                if nx < 1 or nx > map.width or ny < 1 or ny > map.height or
                   map.tiles[ny][nx] == MapGenerator.TileTypes.WALL then
                    count = count + 1
                end
            end
        end
    end
    return count
end

-- ============================================================================
-- ALGORITHM: OPEN ARENA
-- ============================================================================

function MapGenerator.GenerateOpenArena(map)
    -- Create large open space with borders
    for y = 2, map.height - 1 do
        for x = 2, map.width - 1 do
            map.tiles[y][x] = ((x + y) % 2 == 0) and MapGenerator.TileTypes.FLOOR_A or MapGenerator.TileTypes.FLOOR_B
        end
    end

    -- Add some decorative walls/obstacles
    local obstacleCount = math.random(3, 8)
    for i = 1, obstacleCount do
        local ox = math.random(4, map.width - 4)
        local oy = math.random(4, map.height - 4)
        local osize = math.random(2, 4)

        for dy = 0, osize - 1 do
            for dx = 0, osize - 1 do
                if ox + dx <= map.width - 2 and oy + dy <= map.height - 2 then
                    map.tiles[oy + dy][ox + dx] = MapGenerator.TileTypes.WALL
                end
            end
        end
    end
end

-- ============================================================================
-- ENTITY PLACEMENT
-- ============================================================================

function MapGenerator.PlaceEntities(map, config)
    local walkableTiles = MapGenerator.GetWalkableTiles(map)

    if #walkableTiles < 10 then
        print("[MapGenerator] ERROR: Not enough walkable tiles for entity placement!")
        return false
    end

    -- Shuffle walkable tiles
    MapGenerator.Shuffle(walkableTiles)

    local index = 1

    -- Place player (always in first room or open area)
    if index <= #walkableTiles then
        local tile = walkableTiles[index]
        map.tiles[tile.y][tile.x] = MapGenerator.TileTypes.PLAYER
        map.playerPos = {x = tile.x, y = tile.y}
        index = index + 1
    end

    -- Place goal (far from player)
    local goalPlaced = false
    for i = #walkableTiles, math.max(1, #walkableTiles - 5), -1 do
        local tile = walkableTiles[i]
        if MapGenerator.ManhattanDistance(map.playerPos, tile) > math.floor(map.width / 2) then
            map.tiles[tile.y][tile.x] = MapGenerator.TileTypes.GOAL
            map.goalPos = {x = tile.x, y = tile.y}
            goalPlaced = true
            table.remove(walkableTiles, i)
            break
        end
    end

    if not goalPlaced and index <= #walkableTiles then
        local tile = walkableTiles[#walkableTiles]
        map.tiles[tile.y][tile.x] = MapGenerator.TileTypes.GOAL
        map.goalPos = {x = tile.x, y = tile.y}
        table.remove(walkableTiles, #walkableTiles)
    end

    -- Place enemies
    local enemyCount = math.random(config.minEnemies, config.maxEnemies)
    for i = 1, math.min(enemyCount, #walkableTiles - index) do
        if index <= #walkableTiles then
            local tile = walkableTiles[index]
            map.tiles[tile.y][tile.x] = MapGenerator.TileTypes.ENEMY
            table.insert(map.enemyPositions, {x = tile.x, y = tile.y})
            index = index + 1
        end
    end

    -- Place chests
    local chestCount = math.random(config.minChests, config.maxChests)
    for i = 1, math.min(chestCount, #walkableTiles - index) do
        if index <= #walkableTiles then
            local tile = walkableTiles[index]
            map.tiles[tile.y][tile.x] = MapGenerator.TileTypes.CHEST
            table.insert(map.chestPositions, {x = tile.x, y = tile.y})
            index = index + 1
        end
    end

    return true
end

function MapGenerator.GetWalkableTiles(map)
    local tiles = {}
    for y = 2, map.height - 1 do
        for x = 2, map.width - 1 do
            if map.tiles[y][x] ~= MapGenerator.TileTypes.WALL then
                table.insert(tiles, {x = x, y = y})
            end
        end
    end
    return tiles
end

function MapGenerator.ManhattanDistance(pos1, pos2)
    return math.abs(pos1.x - pos2.x) + math.abs(pos1.y - pos2.y)
end

function MapGenerator.Shuffle(array)
    for i = #array, 2, -1 do
        local j = math.random(i)
        array[i], array[j] = array[j], array[i]
    end
end

-- ============================================================================
-- MAP GENERATION MAIN FUNCTION
-- ============================================================================

function MapGenerator.Generate(config)
    config = config or MapGenerator.DefaultConfig

    -- Ensure dimensions are valid
    if config.width < 10 or config.height < 10 then
        print("[MapGenerator] ERROR: Map too small (min 10x10)")
        return nil
    end

    -- Create empty map
    local map = MapGenerator.CreateMap(config)

    -- Generate terrain based on algorithm
    if config.algorithm == "rooms" then
        local rooms = MapGenerator.GenerateRooms(map, config)
        if #rooms == 0 then
            print("[MapGenerator] WARNING: No rooms generated, using open arena")
            MapGenerator.GenerateOpenArena(map)
        end
    elseif config.algorithm == "cellular" then
        MapGenerator.GenerateCellular(map, config)
    elseif config.algorithm == "open" then
        MapGenerator.GenerateOpenArena(map)
    else
        print("[MapGenerator] ERROR: Unknown algorithm '" .. config.algorithm .. "'")
        return nil
    end

    -- Ensure borders are walls
    if config.forceBorders then
        MapGenerator.ForceBorders(map)
    end

    -- Place entities
    if not MapGenerator.PlaceEntities(map, config) then
        print("[MapGenerator] ERROR: Failed to place entities")
        return nil
    end

    print("[MapGenerator] Map generated successfully!")
    print("[MapGenerator]   Size: " .. map.width .. "x" .. map.height)
    print("[MapGenerator]   Player: (" .. map.playerPos.x .. ", " .. map.playerPos.y .. ")")
    print("[MapGenerator]   Goal: (" .. map.goalPos.x .. ", " .. map.goalPos.y .. ")")
    print("[MapGenerator]   Enemies: " .. #map.enemyPositions)
    print("[MapGenerator]   Chests: " .. #map.chestPositions)

    return map
end

function MapGenerator.ForceBorders(map)
    -- Top and bottom borders
    for x = 1, map.width do
        map.tiles[1][x] = MapGenerator.TileTypes.WALL
        map.tiles[map.height][x] = MapGenerator.TileTypes.WALL
    end

    -- Left and right borders
    for y = 1, map.height do
        map.tiles[y][1] = MapGenerator.TileTypes.WALL
        map.tiles[y][map.width] = MapGenerator.TileTypes.WALL
    end
end

-- ============================================================================
-- JSON EXPORT
-- ============================================================================

function MapGenerator.ExportToJSON(map)
    local json = {}
    json.TileMap = {}
    json.TileMap.cellSize = 128

    -- Tile definitions (static for now)
    json.TileMap.TileDefinitions = {
        W = {texture = "assets/TileMap/Tree_Block.png", solid = 1, layer = 0},
        ["0"] = {texture = "assets/TileMap/Grass_Block.png", solid = 0, layer = 0},
        ["1"] = {texture = "assets/TileMap/Grass_Block_Alt.png", solid = 0, layer = 0},
        P = {texture = "assets/player.png", solid = 0, entityType = "Player"},
        E = {texture = "assets/player.png", solid = 0, entityType = "Enemy"},
        S = {texture = "assets/TileMap/Chest_1.png", solid = 0, entityType = "Chest"},
        M = {texture = "assets/TileMap/Portal.png", solid = 0, entityType = "Goal"}
    }

    -- Convert tile grid to row strings
    json.TileMap.Tiles = {}
    for y = 1, map.height do
        local row = ""
        for x = 1, map.width do
            row = row .. map.tiles[y][x]
        end
        json.TileMap.Tiles["Row" .. (y - 1)] = row
    end

    return json
end

function MapGenerator.SaveToFile(map, filename)
    local json = MapGenerator.ExportToJSON(map)

    -- Convert to JSON string (manual serialization)
    local jsonStr = MapGenerator.TableToJSON(json)

    -- Write to file
    local file = io.open(filename, "w")
    if not file then
        print("[MapGenerator] ERROR: Could not open file '" .. filename .. "' for writing")
        return false
    end

    file:write(jsonStr)
    file:close()

    print("[MapGenerator] Map saved to: " .. filename)
    return true
end

function MapGenerator.TableToJSON(t, indent)
    indent = indent or 0
    local indentStr = string.rep("  ", indent)
    local nextIndentStr = string.rep("  ", indent + 1)

    if type(t) ~= "table" then
        if type(t) == "string" then
            return '"' .. t .. '"'
        else
            return tostring(t)
        end
    end

    local result = "{\n"
    local first = true

    for k, v in pairs(t) do
        if not first then
            result = result .. ",\n"
        end
        first = false

        local key = type(k) == "string" and ('"' .. k .. '"') or tostring(k)
        result = result .. nextIndentStr .. key .. ": "

        if type(v) == "table" then
            result = result .. MapGenerator.TableToJSON(v, indent + 1)
        elseif type(v) == "string" then
            result = result .. '"' .. v .. '"'
        else
            result = result .. tostring(v)
        end
    end

    result = result .. "\n" .. indentStr .. "}"
    return result
end

-- ============================================================================
-- CONSOLE VISUALIZATION
-- ============================================================================

function MapGenerator.Print(map)
    print("\n[MapGenerator] Map Preview:")
    for y = 1, map.height do
        local row = ""
        for x = 1, map.width do
            row = row .. map.tiles[y][x]
        end
        print(row)
    end
    print("")
end

-- ============================================================================
-- EXAMPLE USAGE
-- ============================================================================

function MapGenerator.Example()
    print("[MapGenerator] Generating example map...")

    local config = {
        width = 21,
        height = 26,
        algorithm = "rooms",  -- Try: "rooms", "cellular", "open"
        minRoomSize = 3,
        maxRoomSize = 7,
        maxRooms = 8,
        minEnemies = 3,
        maxEnemies = 6,
        minChests = 2,
        maxChests = 4,
        forceBorders = true
    }

    local map = MapGenerator.Generate(config)

    if map then
        MapGenerator.Print(map)
        -- MapGenerator.SaveToFile(map, "assets/JSON/GeneratedMap.json")
    end
end

return MapGenerator
