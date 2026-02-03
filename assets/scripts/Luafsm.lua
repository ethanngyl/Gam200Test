--[[
===============================================================================
 File:           LuaFSM.lua
 Date:           2025-01-27
 ------------------------------------------------------------------------------

 FINITE STATE MACHINE - Lua Implementation
 
 A simple, reusable FSM for Lua scripts.
 Load this BEFORE any script that uses it.

 ============================================================================
 USAGE EXAMPLE:
 ============================================================================

    -- Create FSM
    local myFSM = FSM:new("EnemyFSM")
    myFSM:setDebugEnabled(true)

    -- Add states
    myFSM:addState("Idle", {
        enter = function(self) end,
        update = function(self, dt) end,
        exit = function(self) end
    })

    -- Start and update
    myFSM:start("Idle")
    myFSM:update(dt)

    -- Change state
    myFSM:changeState("Chase")

    -- Query state
    if myFSM:isInState("Idle") then ... end

===============================================================================
]]--

-- ============================================================================
-- FSM CLASS
-- ============================================================================

FSM = {}
FSM.__index = FSM

--- Creates a new FSM instance
-- @param name (string) Name for debug logging
-- @return FSM instance
function FSM:new(name)
    local instance = {
        name = name or "FSM",
        states = {},
        currentState = nil,
        currentStateName = "",
        owner = nil,
        data = {},  -- Shared data between states
        debugEnabled = false
    }
    setmetatable(instance, FSM)
    return instance
end

--- Adds a state to the FSM
-- @param name (string) Unique state identifier
-- @param stateTable (table) Table with enter/update/exit functions
function FSM:addState(name, stateTable)
    if not name or not stateTable then
        print("[" .. self.name .. "] ERROR: Invalid state")
        return
    end

    -- Default empty functions if not provided
    stateTable.enter = stateTable.enter or function() end
    stateTable.update = stateTable.update or function() end
    stateTable.exit = stateTable.exit or function() end
    
    -- Give state access to FSM
    stateTable.fsm = self
    stateTable.name = name

    self.states[name] = stateTable
end

--- Starts the FSM with initial state
-- @param stateName (string) Name of starting state
function FSM:start(stateName)
    if not self.states[stateName] then
        print("[" .. self.name .. "] ERROR: State '" .. stateName .. "' not found")
        return
    end

    self.currentStateName = stateName
    self.currentState = self.states[stateName]

    if self.debugEnabled then
        print("[" .. self.name .. "] Started: " .. stateName)
    end

    self.currentState.enter(self.currentState)
end

--- Changes to a new state
-- @param newStateName (string) Name of state to change to
function FSM:changeState(newStateName)
    if not self.states[newStateName] then
        print("[" .. self.name .. "] ERROR: State '" .. newStateName .. "' not found")
        return
    end

    -- Don't transition to same state
    if newStateName == self.currentStateName then
        return
    end

    local oldName = self.currentStateName

    -- Exit current state
    if self.currentState then
        self.currentState.exit(self.currentState)
    end

    -- Enter new state
    self.currentStateName = newStateName
    self.currentState = self.states[newStateName]

    if self.debugEnabled then
        print("[" .. self.name .. "] " .. oldName .. " -> " .. newStateName)
    end

    self.currentState.enter(self.currentState)
end

--- Updates current state (call every frame)
-- @param dt (number) Delta time in seconds
function FSM:update(dt)
    if self.currentState then
        self.currentState.update(self.currentState, dt)
    end
end

--- Gets current state name
-- @return (string) Current state name
function FSM:getCurrentState()
    return self.currentStateName
end

--- Checks if in a specific state
-- @param stateName (string) State to check
-- @return (boolean) True if in that state
function FSM:isInState(stateName)
    return self.currentStateName == stateName
end

--- Sets owner (entity ID or any identifier)
-- @param owner Owner reference
function FSM:setOwner(owner)
    self.owner = owner
end

--- Gets owner
-- @return Owner reference
function FSM:getOwner()
    return self.owner
end

--- Sets shared data (accessible by all states)
-- @param key (string) Data key
-- @param value Value to store
function FSM:setData(key, value)
    self.data[key] = value
end

--- Gets shared data
-- @param key (string) Data key
-- @return Stored value or nil
function FSM:getData(key)
    return self.data[key]
end

--- Enables/disables debug logging
-- @param enabled (boolean)
function FSM:setDebugEnabled(enabled)
    self.debugEnabled = enabled
end

-- ============================================================================
-- GLOBAL EXPORT
-- ============================================================================

_G.FSM = FSM

print("[LuaFSM] Module loaded")