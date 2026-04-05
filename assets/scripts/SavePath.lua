-- SavePath.lua
-- Provides a writable save directory path using %APPDATA% on Windows.
-- Falls back to "assets/JSON/" if APPDATA is unavailable.

local SavePath = {}

local _dir = nil

function SavePath.GetDir()
    if _dir then return _dir end

    local appdata = os.getenv("APPDATA")
    if appdata then
        _dir = appdata .. "/StructSquad/"
    else
        -- Fallback: write next to the exe (works in dev, may fail in Program Files)
        _dir = "assets/JSON/"
    end

    -- Ensure the directory exists
    os.execute('mkdir "' .. _dir:gsub("/", "\\") .. '" 2>nul')

    return _dir
end

function SavePath.GetLevelProgressPath()
    return SavePath.GetDir() .. "LevelProgress.json"
end

function SavePath.GetSkillLoadoutPath()
    return SavePath.GetDir() .. "SkillLoadout.json"
end

return SavePath
