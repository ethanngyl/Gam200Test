-- ============================================================================
-- AttackAPIndicatorUI.lua
-- Attack Action Point (AP) indicator component
-- ============================================================================
-- Displays attack AP as a row of animated diamonds
-- New AP_Crystal.png is a 4x4 sprite sheet with animation frames:
--   Row 1-2 (frames 0-7): Idle/filled animation (loops)
--   Row 3-4 (frames 8-15): Consume animation (plays once when AP is spent)
--
-- TEST: Press SPACE to toggle between filled (rows 1-2) and consume (rows 3-4) animations
-- ============================================================================

local UIComponent = require("UI/UIComponent")
local AttackAPIndicatorUI = {}
setmetatable(AttackAPIndicatorUI, {__index = UIComponent})
AttackAPIndicatorUI.__index = AttackAPIndicatorUI

-- ============================================================================
-- CONSTRUCTOR
-- ============================================================================

function AttackAPIndicatorUI:New()
    local instance = UIComponent:New()
    setmetatable(instance, self)
    return instance
end

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function AttackAPIndicatorUI:Init(config)
    self.config = config or {}

    -- Configuration
    self.maxAP = self.config.maxAP or 3
    self.indicatorSize = self.config.size or 0.06
    self.indicatorSpacing = self.config.spacing or 0.1
    self.offsetX = self.config.offsetX or -0.64
    self.offsetY = self.config.offsetY or -0.32
    self.layer = self.config.layer or 4
    self.emptyLayerOffset = self.config.emptyLayerOffset or 0
    self.filledLayerOffset = self.config.filledLayerOffset or 1

    -- Textures
    self.emptyTexture = self.config.emptyTexture or "assets/UI/AP_Empty.png"
    self.filledTexture = self.config.filledTexture or "assets/UI/AP_Crystal.png"
    
    -- Sprite sheet animation config (AP_Crystal.png is 4x4)
    self.useAnimatedSprite = self.config.useAnimatedSprite
    if self.useAnimatedSprite == nil then
        self.useAnimatedSprite = true  -- Default to using animated sprite sheet
    end
    self.spriteRows = self.config.spriteRows or 4
    self.spriteCols = self.config.spriteCols or 4
    
    -- Animation frame ranges (4x4 = 16 frames total)
    -- Filled/Idle animation: frames 0-7 (rows 1-2), loops
    self.filledStartFrame = self.config.filledStartFrame or 0
    self.filledFrameCount = self.config.filledFrameCount or 8
    -- Consume animation: frames 8-15 (rows 3-4), plays once
    self.consumeStartFrame = self.config.consumeStartFrame or 8
    self.consumeFrameCount = self.config.consumeFrameCount or 8
    
    self.frameTime = self.config.frameTime or 0.1
    self.animationLoop = self.config.animationLoop
    if self.animationLoop == nil then
        self.animationLoop = true
    end

    -- Tint configuration
    self.useTint = self.config.useTint
    if self.useTint == nil then
        self.useTint = true
    end
    self.filledTint = self.config.filledTint or { r = 1.0, g = 1.0, b = 1.0 }
    self.emptyTint = self.config.emptyTint or self.filledTint
    self.useGray = self.config.useGray or false
    self.filledGrayAmount = self.config.filledGrayAmount or 0.0
    self.emptyGrayAmount = self.config.emptyGrayAmount or 1.0

    -- State
    self.indicators = {}
    self.indicatorStates = {}  -- Track state: "filled", "consuming", "empty"
    self.emptyIndicators = {}
    self.filledIndicators = {}
    self.lastKnownAP = 0
    
    -- TEST: Toggle state for space key
    self.testAnimationState = "filled"  -- "filled" = rows 1-2, "consume" = rows 3-4
    self.spaceWasPressed = false

    -- Get current AP
    local currentAP = self:GetCurrentAP()
    self.lastKnownAP = math.min(currentAP or 0, self.maxAP)

    -- Get camera position for initial placement
    local camX, camY, camZ = GetCameraPosition()

    if self.useTint then
        -- Single-layer: all indicators use the same sprite, just different animation states
        for i = 1, self.maxAP do
            local xPos = camX + self.offsetX + ((i - 1) * self.indicatorSpacing)
            local yPos = camY + self.offsetY

            local entityID
            if self.useAnimatedSprite then
                -- Spawn with filled animation initially (frames 0-7)
                entityID = self:SpawnAnimatedSprite(
                    self.filledTexture,
                    xPos, yPos,
                    self.indicatorSize, self.indicatorSize,
                    self.layer,
                    self.spriteRows, self.spriteCols,
                    self.filledFrameCount,  -- Only use frames 0-7
                    self.frameTime,
                    true  -- Loop
                )
            else
                -- Use static sprite
                entityID = self:SpawnSprite(
                    self.filledTexture,
                    xPos, yPos,
                    self.indicatorSize, self.indicatorSize,
                    self.layer
                )
            end

            self.indicators[i] = entityID
            self.indicatorStates[i] = "filled"

            if entityID and entityID > 0 then
                -- Set initial animation to filled state (frames 0-7)
                SetAnimationFrameRange(entityID, self.filledStartFrame, self.filledFrameCount, true)
                SetAnimationLoop(entityID, true)
                self:ApplyVisual(entityID, self.filledTint, self.filledGrayAmount)
            end
        end
        
        -- Sync all crystal animations to start at the same frame
        -- This ensures they all animate together instead of being out of phase
        for i = 1, #self.indicators do
            local entityID = self.indicators[i]
            if entityID and entityID > 0 then
                SetAnimationFrameRange(entityID, self.filledStartFrame, self.filledFrameCount, true)
            end
        end
        Log("[AttackAPIndicatorUI] Created " .. #self.indicators .. " crystals, animations synced")
    else
        -- Two-layer mode (not updated for new animation system)
        for i = 1, self.maxAP do
            local xPos = camX + self.offsetX + ((i - 1) * self.indicatorSpacing)
            local yPos = camY + self.offsetY

            local entityID = self:SpawnSprite(
                self.emptyTexture,
                xPos, yPos,
                self.indicatorSize, self.indicatorSize,
                self.layer + self.emptyLayerOffset
            )

            self.emptyIndicators[i] = entityID
        end

        for i = 1, self.lastKnownAP do
            local xPos = camX + self.offsetX + ((i - 1) * self.indicatorSpacing)
            local yPos = camY + self.offsetY

            local entityID
            if self.useAnimatedSprite then
                entityID = self:SpawnAnimatedSprite(
                    self.filledTexture,
                    xPos, yPos,
                    self.indicatorSize, self.indicatorSize,
                    self.layer + self.filledLayerOffset,
                    self.spriteRows, self.spriteCols,
                    self.filledFrameCount, self.frameTime,
                    self.animationLoop
                )
            else
                entityID = self:SpawnSprite(
                    self.filledTexture,
                    xPos, yPos,
                    self.indicatorSize, self.indicatorSize,
                    self.layer + self.filledLayerOffset
                )
            end

            self.filledIndicators[i] = entityID

            if entityID and entityID > 0 then
                self:ApplyVisual(entityID, self.filledTint, self.filledGrayAmount)
            end
        end
    end

    Log("[AttackAPIndicatorUI] Initialized - " .. self.lastKnownAP .. "/" .. self.maxAP .. " AP (animated=" .. tostring(self.useAnimatedSprite) .. ")")
    Log("[AttackAPIndicatorUI] TEST: Press SPACE to toggle between rows 1-2 and rows 3-4 animations")
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function AttackAPIndicatorUI:Update(dt, cameraPos)
    if not self.enabled then return end

    -- TEST: Check for SPACE key to consume one AP
    local spacePressed = IsKeyDown("Space")
    if spacePressed and not self.spaceWasPressed then
        self:ConsumeOneAP()
    end
    self.spaceWasPressed = spacePressed

    -- Update consuming crystals (check if animation finished)
    self:UpdateConsumingCrystals(dt)

    -- Update sprite positions if camera moved
    if self:ShouldUpdatePosition(cameraPos) then
        self:UpdatePositions(cameraPos)
    end
end

-- ============================================================================
-- CONSUME AP - Trigger consume animation on one crystal
-- ============================================================================

function AttackAPIndicatorUI:ConsumeOneAP()
    -- Find the rightmost filled crystal to consume
    local crystalToConsume = nil
    for i = #self.indicators, 1, -1 do
        if self.indicatorStates[i] == "filled" then
            crystalToConsume = i
            break
        end
    end
    
    if not crystalToConsume then
        Log("[AttackAPIndicatorUI] No AP left to consume!")
        return
    end
    
    local entityID = self.indicators[crystalToConsume]
    if not entityID or entityID <= 0 then return end
    
    Log("[AttackAPIndicatorUI] Consuming AP crystal " .. crystalToConsume .. " (Entity " .. entityID .. ")")
    
    -- Change state to "consuming"
    self.indicatorStates[crystalToConsume] = "consuming"
    
    -- Start consume animation (frames 8-19, rows 3-5)
    SetAnimationFrameRange(entityID, self.consumeStartFrame, self.consumeFrameCount, true)
    SetAnimationLoop(entityID, false)  -- Don't loop - play once
    SetAnimationPlaying(entityID, true)
    
    -- Track this crystal's animation progress
    if not self.consumingCrystals then
        self.consumingCrystals = {}
    end
    self.consumingCrystals[crystalToConsume] = {
        entityID = entityID,
        elapsedTime = 0,
        totalTime = self.consumeFrameCount * self.frameTime  -- 12 frames * 0.1s = 1.2s
    }
end

-- ============================================================================
-- UPDATE CONSUMING CRYSTALS - Check if consume animation finished
-- ============================================================================

function AttackAPIndicatorUI:UpdateConsumingCrystals(dt)
    if not self.consumingCrystals then return end
    
    local toRemove = {}
    
    for index, data in pairs(self.consumingCrystals) do
        data.elapsedTime = data.elapsedTime + dt
        
        -- Check if animation finished
        if data.elapsedTime >= data.totalTime then
            Log("[AttackAPIndicatorUI] Crystal " .. index .. " consume animation finished - hiding")
            
            -- Hide the crystal
            local entityID = data.entityID
            if entityID and entityID > 0 then
                -- Hide by setting invisible (keeps entity for potential restore)
                SetSpriteVisibility(entityID, false)
            end
            
            -- Mark state as "empty"
            self.indicatorStates[index] = "empty"
            
            -- Mark for removal from tracking
            table.insert(toRemove, index)
        end
    end
    
    -- Remove finished crystals from tracking
    for _, index in ipairs(toRemove) do
        self.consumingCrystals[index] = nil
    end
end

-- ============================================================================
-- RESTORE AP - Show crystal again with filled animation
-- ============================================================================

function AttackAPIndicatorUI:RestoreAP(index)
    if index < 1 or index > #self.indicators then return end
    
    local entityID = self.indicators[index]
    if not entityID or entityID <= 0 then return end
    
    Log("[AttackAPIndicatorUI] Restoring AP crystal " .. index)
    
    -- Show the crystal again
    SetSpriteVisibility(entityID, true)
    
    -- Reset to filled animation (frames 0-7, rows 1-2)
    SetAnimationFrameRange(entityID, self.filledStartFrame, self.filledFrameCount, true)
    SetAnimationLoop(entityID, true)
    SetAnimationPlaying(entityID, true)
    
    -- Update state
    self.indicatorStates[index] = "filled"
end

-- ============================================================================
-- RESTORE ALL AP - Reset all crystals to filled state
-- ============================================================================

function AttackAPIndicatorUI:RestoreAllAP()
    Log("[AttackAPIndicatorUI] Restoring all AP crystals")
    for i = 1, #self.indicators do
        self:RestoreAP(i)
    end
    self.consumingCrystals = {}
end

-- ============================================================================
-- HELPER METHODS
-- ============================================================================

function AttackAPIndicatorUI:GetCurrentAP()
    local currentAP, maxPlayerAP = GetPlayerAttackAP()
    return currentAP or 0
end

function AttackAPIndicatorUI:UpdatePositions(cameraPos)
    if self.useTint then
        for i = 1, #self.indicators do
            local entityID = self.indicators[i]
            if entityID and entityID > 0 then
                local xPos = cameraPos.x + self.offsetX + ((i - 1) * self.indicatorSpacing)
                local yPos = cameraPos.y + self.offsetY
                SetSpritePosition(entityID, xPos, yPos)
            end
        end
    else
        for i = 1, #self.emptyIndicators do
            local entityID = self.emptyIndicators[i]
            if entityID and entityID > 0 then
                local xPos = cameraPos.x + self.offsetX + ((i - 1) * self.indicatorSpacing)
                local yPos = cameraPos.y + self.offsetY
                SetSpritePosition(entityID, xPos, yPos)
            end
        end

        for i = 1, #self.filledIndicators do
            local entityID = self.filledIndicators[i]
            if entityID and entityID > 0 then
                local xPos = cameraPos.x + self.offsetX + ((i - 1) * self.indicatorSpacing)
                local yPos = cameraPos.y + self.offsetY
                SetSpritePosition(entityID, xPos, yPos)
            end
        end
    end
end

function AttackAPIndicatorUI:HandleAPChange(newAP, cameraPos)
    newAP = math.max(0, math.min(newAP, self.maxAP))

    if newAP < self.lastKnownAP then
        -- AP decreased - destroy extra diamonds
        for i = self.lastKnownAP, newAP + 1, -1 do
            if self.filledIndicators[i] then
                DestroyEntity(self.filledIndicators[i])
                self.filledIndicators[i] = nil
            end
        end
    elseif newAP > self.lastKnownAP then
        -- AP increased - create new diamonds
        for i = self.lastKnownAP + 1, newAP do
            local xPos = cameraPos.x + self.offsetX + ((i - 1) * self.indicatorSpacing)
            local yPos = cameraPos.y + self.offsetY

            local entityID
            if self.useAnimatedSprite then
                entityID = self:SpawnAnimatedSprite(
                    self.filledTexture,
                    xPos, yPos,
                    self.indicatorSize, self.indicatorSize,
                    self.layer + self.filledLayerOffset,
                    self.spriteRows, self.spriteCols,
                    self.frameCount, self.frameTime,
                    self.animationLoop
                )
            else
                entityID = self:SpawnSprite(
                    self.filledTexture,
                    xPos, yPos,
                    self.indicatorSize, self.indicatorSize,
                    self.layer + self.filledLayerOffset
                )
            end

            self.filledIndicators[i] = entityID

            if entityID and entityID > 0 then
                self:ApplyTint(entityID, self.filledTint)
            end
        end
    end
end

function AttackAPIndicatorUI:UpdateTints(currentAP)
    for i = 1, #self.indicators do
        local entityID = self.indicators[i]
        if entityID and entityID > 0 then
            local currentState = self.indicatorStates[i] or "empty"
            local shouldBeFilled = (i <= currentAP)
            
            if shouldBeFilled and currentState ~= "filled" then
                -- Transition to filled state (refill animation could be added here)
                self:SetAnimationFrameRange(entityID, self.filledStartFrame, self.filledFrameCount, true)
                self:SetAnimationLoop(entityID, true)
                self.indicatorStates[i] = "filled"
                self:ApplyVisual(entityID, self.filledTint, self.filledGrayAmount)
                Log("[AttackAPIndicatorUI] Crystal " .. i .. " -> FILLED")
                
            elseif not shouldBeFilled and currentState == "filled" then
                -- Transition to consuming state (play consume animation)
                self:SetAnimationFrameRange(entityID, self.consumeStartFrame, self.consumeFrameCount, true)
                self:SetAnimationLoop(entityID, false)  -- Play once
                self.indicatorStates[i] = "consuming"
                -- Keep filled tint during consume animation
                self:ApplyVisual(entityID, self.filledTint, self.filledGrayAmount)
                Log("[AttackAPIndicatorUI] Crystal " .. i .. " -> CONSUMING")
            end
            -- Note: "consuming" -> "empty" transition happens after animation finishes
            -- For now, we can gray it out immediately after consume animation starts
            -- The animation system will stop on the last frame automatically
        end
    end
end

function AttackAPIndicatorUI:ApplyVisual(entityID, tint, grayAmount)
    self:ApplyTint(entityID, tint)
    if self.useGray then
        SetSpriteGray(entityID, grayAmount or 0.0)
    end
end

function AttackAPIndicatorUI:ApplyTint(entityID, tint)
    if not entityID or entityID <= 0 or not tint then
        return
    end

    local r = tint.r or tint[1] or 1.0
    local g = tint.g or tint[2] or 1.0
    local b = tint.b or tint[3] or 1.0
    SetSpriteColor(entityID, r, g, b)
end

function AttackAPIndicatorUI:Destroy()
    if self.useTint then
        for i = 1, #self.indicators do
            if self.indicators[i] and self.indicators[i] > 0 then
                DestroyEntity(self.indicators[i])
            end
        end
        self.indicators = {}
    else
        for i = 1, #self.emptyIndicators do
            if self.emptyIndicators[i] and self.emptyIndicators[i] > 0 then
                DestroyEntity(self.emptyIndicators[i])
            end
        end

        for i = 1, #self.filledIndicators do
            if self.filledIndicators[i] and self.filledIndicators[i] > 0 then
                DestroyEntity(self.filledIndicators[i])
            end
        end
    end

    self.emptyIndicators = {}
    self.filledIndicators = {}

    Log("[AttackAPIndicatorUI] Destroyed")
end

return AttackAPIndicatorUI
