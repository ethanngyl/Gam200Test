--[[
===============================================================================
 File:           PlayerScript.lua
 Date:           2025-01-27
 ------------------------------------------------------------------------------

 PLAYER SCRIPT - FSM Version
 
 Grid-based player movement using Finite State Machine.
 Currently has 2 states (ready for more when animations are added):
   - WaitingForInput: Checks for WASD/P input
   - Moving: Executes movement to target tile

 FUTURE STATES (add when needed):
   - Attacking: When combat is implemented
   - Hurt: When taking damage with animation
   - Dead: When player dies
   - CastingSkill: When skills are implemented

===============================================================================
]]--

-- ============================================================================
-- CONFIGURATION (Easy to modify)
-- ============================================================================

local Config = {
    moveCooldownTime = 0.2,     -- Seconds between moves
    apCostPerMove = 1,          -- AP cost per movement
    moveDuration = 0.15,        -- How long movement state lasts
    debugFSM = true,            -- Show FSM state changes in console
}

-- ============================================================================
-- ANIMATION ENUMS (Ready for future use)
-- ============================================================================

local AnimGroup = {
    Idle = 0,
    Walk = 1,
    Attack = 2,
    Injured = 3,
    Death = 4
}

local AnimDirection = {
    Front = 0,
    Back = 1,
    Side = 2,
    None = 3
}

-- ============================================================================
-- SCRIPT STATE
-- ============================================================================

local entityID = 0
local playerFSM = nil

-- Animation state (for future use)
local currentAnimDirection = AnimDirection.Front
local isFlippedX = false

-- Turn tracking
local lastActiveCheck = false
local hasLoggedActive = false
local blockedKeys = {}

-- ============================================================================
-- HELPER FUNCTIONS
-- ============================================================================

--- Blocks keys that are currently held (prevents input carry-over)
local function blockHeldKeys()
    blockedKeys = {}
    if IsKeyDown("W") then blockedKeys["W"] = true end
    if IsKeyDown("S") then blockedKeys["S"] = true end
    if IsKeyDown("A") then blockedKeys["A"] = true end
    if IsKeyDown("D") then blockedKeys["D"] = true end
    if IsKeyDown("P") then blockedKeys["P"] = true end
end

--- Updates blocked keys - unblocks when released
local function updateBlockedKeys()
    if blockedKeys["W"] and not IsKeyDown("W") then blockedKeys["W"] = nil end
    if blockedKeys["S"] and not IsKeyDown("S") then blockedKeys["S"] = nil end
    if blockedKeys["A"] and not IsKeyDown("A") then blockedKeys["A"] = nil end
    if blockedKeys["D"] and not IsKeyDown("D") then blockedKeys["D"] = nil end
    if blockedKeys["P"] and not IsKeyDown("P") then blockedKeys["P"] = nil end
end

--- Gets movement input (respects blocked keys)
-- @return wDown, sDown, aDown, dDown
local function getMovementInput()
    return
        IsKeyDown("W") and not blockedKeys["W"],
        IsKeyDown("S") and not blockedKeys["S"],
        IsKeyDown("A") and not blockedKeys["A"],
        IsKeyDown("D") and not blockedKeys["D"]
end

--- Checks if P key is pressed (respects blocked keys)
local function isEndTurnPressed()
    return IsKeyDown("P") and not blockedKeys["P"]
end

--- Updates animation direction based on movement
-- @param moveDirX (-1, 0, or 1)
-- @param moveDirY (-1, 0, or 1)
local function updateAnimationDirection(moveDirX, moveDirY)
    local newDirection = currentAnimDirection
    local newFlipX = isFlippedX

    if moveDirY > 0 then
        newDirection = AnimDirection.Back
    elseif moveDirY < 0 then
        newDirection = AnimDirection.Front
    elseif moveDirX > 0 then
        newDirection = AnimDirection.Side
        newFlipX = false
    elseif moveDirX < 0 then
        newDirection = AnimDirection.Side
        newFlipX = true
    end

    if newDirection ~= currentAnimDirection then
        currentAnimDirection = newDirection
        SetAnimationDirection(entityID, currentAnimDirection)
    end
    if newFlipX ~= isFlippedX then
        isFlippedX = newFlipX
        SetAnimationFlipX(entityID, isFlippedX)
    end
end

--- Ends the current character's turn and resets state
local function endTurn()
    EndCharacterTurn()
    hasLoggedActive = false
    lastActiveCheck = false
    blockedKeys = {}
end

-- ============================================================================
-- FSM STATES
-- ============================================================================

--- Creates and adds all player states to the FSM
-- @param fsm The FSM instance to add states to
local function createPlayerStates(fsm)

    ---------------------------------------------------------------------------
    -- WAITING FOR INPUT STATE
    -- Player is active and waiting for input (WASD to move, P to end turn)
    ---------------------------------------------------------------------------
    fsm:addState("WaitingForInput", {
        enter = function(self)
            -- FUTURE: Set idle animation here
            -- SetAnimationGroup(entityID, AnimGroup.Idle)
        end,

        update = function(self, dt)
            -- Only process if active character
            if not IsActiveCharacter(entityID) then return end

            -- Update blocked keys
            updateBlockedKeys()

            -- Check for end turn (P key)
            if isEndTurnPressed() then
                print("[PlayerScript] P pressed - ending turn")
                endTurn()
                return
            end

            -- Check for movement input
            local wDown, sDown, aDown, dDown = getMovementInput()
            
            if wDown or sDown or aDown or dDown then
                -- Store movement direction in FSM data
                fsm:setData("moveW", wDown)
                fsm:setData("moveS", sDown)
                fsm:setData("moveA", aDown)
                fsm:setData("moveD", dDown)
                fsm:changeState("Moving")
                return
            end

            -- FUTURE: Check for attack input
            -- if IsKeyPressed("SPACE") then
            --     fsm:changeState("Attacking")
            --     return
            -- end
        end,

        exit = function(self)
            -- Nothing needed
        end
    })

    ---------------------------------------------------------------------------
    -- MOVING STATE
    -- Executes movement to target tile, then returns to WaitingForInput
    ---------------------------------------------------------------------------
    fsm:addState("Moving", {
        -- State-local variables
        targetX = 0,
        targetY = 0,
        moveDirX = 0,
        moveDirY = 0,
        moveTimer = 0,

        enter = function(self)
            -- Get current position
            local currentX, currentY = GetEntityGridPosition(entityID)
            self.targetX = currentX
            self.targetY = currentY
            self.moveDirX = 0
            self.moveDirY = 0
            self.moveTimer = 0

            -- Determine direction from FSM data
            if self.fsm:getData("moveW") then
                self.targetY = currentY + 1
                self.moveDirY = 1
            elseif self.fsm:getData("moveS") then
                self.targetY = currentY - 1
                self.moveDirY = -1
            elseif self.fsm:getData("moveA") then
                self.targetX = currentX - 1
                self.moveDirX = -1
            elseif self.fsm:getData("moveD") then
                self.targetX = currentX + 1
                self.moveDirX = 1
            end

            -- Clear FSM data
            self.fsm:setData("moveW", false)
            self.fsm:setData("moveS", false)
            self.fsm:setData("moveA", false)
            self.fsm:setData("moveD", false)

            -- Update animation direction
            updateAnimationDirection(self.moveDirX, self.moveDirY)

            -- FUTURE: Set walk animation here
            -- SetAnimationGroup(entityID, AnimGroup.Walk)
            -- SetAnimationPlaying(entityID, true)

            -- Execute movement immediately
            self:executeMove()
        end,

        update = function(self, dt)
            self.moveTimer = self.moveTimer + dt

            -- Return to WaitingForInput after movement duration
            if self.moveTimer >= Config.moveDuration then
                self.fsm:changeState("WaitingForInput")
            end
        end,

        exit = function(self)
            -- FUTURE: Could trigger idle animation here
        end,

        --- Executes the actual movement
        executeMove = function(self)
            -- Validate position
            if not IsValidGridPosition(self.targetX, self.targetY) then
                print("[PlayerScript] FAILED: Invalid grid position")
                return
            end

            if not IsWalkableTile(self.targetX, self.targetY) then
                print("[PlayerScript] FAILED: Tile not walkable")
                PulseTile(self.targetX, self.targetY, 0.3, 1.0, 0.3, 0.3)  -- Red pulse
                return
            end

            -- Check AP
            local currentAP, maxAP = GetEntityAP(entityID)
            if currentAP < Config.apCostPerMove then
                print("[PlayerScript] FAILED: Not enough AP")
                PulseTile(self.targetX, self.targetY, 0.3, 1.0, 1.0, 0.3)  -- Yellow pulse
                if currentAP == 0 then
                    endTurn()
                end
                return
            end

            -- Execute move
            local success = MoveEntityToTile(entityID, self.targetX, self.targetY)
            
            if success then
                -- Consume AP
                ConsumeEntityAP(entityID, Config.apCostPerMove)

                -- Visual feedback
                ShowTileBorder(self.targetX, self.targetY, 0.5)
                PulseTile(self.targetX, self.targetY, 0.3, 0.3, 1.0, 0.3)  -- Green pulse

                -- Check for interactions
                if HasChestAtTile(self.targetX, self.targetY) then
                    CollectChest(self.targetX, self.targetY)
                    print("[PlayerScript] Collected chest!")
                end

                if HasGoalAtTile(self.targetX, self.targetY) then
                    print("[PlayerScript] Reached goal!")
                end

                -- Check if AP depleted
                local newAP, _ = GetEntityAP(entityID)
                if newAP == 0 then
                    print("[PlayerScript] AP depleted - ending turn")
                    endTurn()
                end

                print("[PlayerScript] Moved to (" .. self.targetX .. ", " .. self.targetY .. ") - AP: " .. newAP)
            else
                print("[PlayerScript] FAILED: Move rejected")
            end
        end
    })

    ---------------------------------------------------------------------------
    -- FUTURE STATES (uncomment and implement when needed)
    ---------------------------------------------------------------------------

    --[[
    fsm:addState("Attacking", {
        attackTimer = 0,
        attackDuration = 0.4,

        enter = function(self)
            self.attackTimer = 0
            SetAnimationGroup(entityID, AnimGroup.Attack)
        end,

        update = function(self, dt)
            self.attackTimer = self.attackTimer + dt
            if self.attackTimer >= self.attackDuration then
                self.fsm:changeState("WaitingForInput")
            end
        end,

        exit = function(self) end
    })

    fsm:addState("Hurt", {
        hurtTimer = 0,
        hurtDuration = 0.3,

        enter = function(self)
            self.hurtTimer = 0
            SetAnimationGroup(entityID, AnimGroup.Injured)
        end,

        update = function(self, dt)
            self.hurtTimer = self.hurtTimer + dt
            if self.hurtTimer >= self.hurtDuration then
                local hp, _ = GetEntityHP(entityID)
                if hp <= 0 then
                    self.fsm:changeState("Dead")
                else
                    self.fsm:changeState("WaitingForInput")
                end
            end
        end,

        exit = function(self) end
    })

    fsm:addState("Dead", {
        enter = function(self)
            SetAnimationGroup(entityID, AnimGroup.Death)
            print("[PlayerScript] Player died!")
        end,

        update = function(self, dt) end,

        exit = function(self) end
    })
    ]]--
end

-- ============================================================================
-- LIFECYCLE: OnInit
-- ============================================================================

function OnInit(id)
    print("============================================================")
    print("[PlayerScript] OnInit for Entity " .. id)
    print("============================================================")

    entityID = id

    -- Log initial stats
    local ap, maxAP = GetEntityAP(entityID)
    local hp, maxHP = GetEntityHP(entityID)
    print("[PlayerScript] AP: " .. tostring(ap) .. "/" .. tostring(maxAP) .. ", HP: " .. tostring(hp) .. "/" .. tostring(maxHP))

    -- Initialize animation state
    currentAnimDirection = AnimDirection.Front
    isFlippedX = false
    SetAnimationDirection(entityID, currentAnimDirection)
    SetAnimationFlipX(entityID, isFlippedX)

    -- Create FSM
    playerFSM = FSM:new("Player_" .. entityID)
    playerFSM:setOwner(entityID)
    playerFSM:setDebugEnabled(Config.debugFSM)

    -- Add states
    createPlayerStates(playerFSM)

    -- Start FSM
    playerFSM:start("WaitingForInput")

    print("[PlayerScript] Initialized with FSM")
    print("============================================================")
end

-- ============================================================================
-- LIFECYCLE: OnUpdate
-- ============================================================================

function OnUpdate(dt)
    -- Check if this is the active character
    local isActive = IsActiveCharacter(entityID)

    -- Handle becoming inactive
    if not isActive then
        if lastActiveCheck then
            -- Just became inactive
            lastActiveCheck = false
            hasLoggedActive = false
            blockedKeys = {}
        end
        return
    end

    -- Handle becoming active
    if isActive and not lastActiveCheck then
        print("============================================================")
        print("[PlayerScript] Entity " .. entityID .. " is now ACTIVE")
        print("============================================================")
        blockHeldKeys()
        hasLoggedActive = true
    end
    lastActiveCheck = isActive

    -- Update FSM
    if playerFSM then
        playerFSM:update(dt)
    end
end

-- ============================================================================
-- LIFECYCLE: OnDestroy
-- ============================================================================

function OnDestroy()
    print("[PlayerScript] Destroyed for entity " .. entityID)
    playerFSM = nil
end

-- ============================================================================
-- EXTERNAL API (Call from other scripts or C++)
-- ============================================================================

--- Called when player takes damage (FUTURE USE)
function OnPlayerHit(damage)
    if playerFSM and not playerFSM:isInState("Dead") then
        -- FUTURE: playerFSM:changeState("Hurt")
        print("[PlayerScript] Player hit for " .. tostring(damage) .. " damage")
    end
end

--- Called when player dies (FUTURE USE)
function OnPlayerDeath()
    if playerFSM then
        -- FUTURE: playerFSM:changeState("Dead")
        print("[PlayerScript] Player died")
    end
end

--- Gets current player state
function GetPlayerState()
    if playerFSM then
        return playerFSM:getCurrentState()
    end
    return "Unknown"
end

--- Gets the FSM instance (for external control)
function GetPlayerFSM()
    return playerFSM
end

-- ============================================================================
-- EXPORTS
-- ============================================================================

_G.OnPlayerHit = OnPlayerHit
_G.OnPlayerDeath = OnPlayerDeath
_G.GetPlayerState = GetPlayerState
_G.GetPlayerFSM = GetPlayerFSM