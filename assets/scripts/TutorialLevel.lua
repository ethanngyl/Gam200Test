-- ============================================================================
-- TutorialLevel.lua (All Buttons Version)
-- Tutorial Level with ALL text as transparent buttons
-- ============================================================================

local buttonIDs = {}
local initialized = false
local config = nil
local editorToggleCooldown = 0
local backgroundSpriteID = 0
local scrollOverlayID = 0
local overlaySprites = {}
local cornerSpriteIDs = {}

function OnInit()
    Log("Tutorial Level Script Initialized (All Buttons Version)")
    Log("Loading configuration from JSON file...")
    
    config = LoadJSON("assets/JSON/tutorial_config.json")
    
    if not config then
        Log("ERROR: Failed to load JSON configuration!")
        return
    end
    
    Log("Successfully loaded configuration for: " .. config.menu.name)
    
    local cam = config.menu.camera
    SetCameraPosition(cam.position.x, cam.position.y, cam.position.z)
    SetCameraZoom(cam.zoom)

    DisableImGui()
    SetEnginePlayState(true)

    -- Create background
    local background = config.menu.background
    Log("Creating background sprite: " .. background.texture)
    backgroundSpriteID = SpawnSprite(
        background.texture,
        background.position.x,
        background.position.y,
        background.scale.x,
        background.scale.y,
        background.layer
    )

    if backgroundSpriteID > 0 then
        Log("✓ Background sprite created (ID: " .. backgroundSpriteID .. ")")
    else
        Log("✗ WARNING: Failed to create background sprite")
    end

    -- Create overlay sprites
    if config.menu.overlaySprites then
        Log("Creating overlay sprites...")

        for i, overlay in ipairs(config.menu.overlaySprites) do
            local spriteID = SpawnSprite(
                overlay.texture,
                overlay.position.x,
                overlay.position.y,
                overlay.scale.x,
                overlay.scale.y,
                overlay.layer,
                overlay.rotation or 0
            )

            if spriteID > 0 then
                overlaySprites[overlay.id] = spriteID
                Log("  ✓ Overlay sprite '" .. overlay.id .. "' created (ID: " .. spriteID .. ")")
            else
                Log("  ✗ FAILED to create overlay sprite: " .. overlay.id)
            end
        end

        Log("Overlay sprites complete!")
    end

    -- Create corner sprites
    if config.menu.cornerSprites then
        Log("Creating corner decorations...")

        for i, corner in ipairs(config.menu.cornerSprites) do
            local spriteID = SpawnSprite(
                corner.texture,
                corner.offset.x,
                corner.offset.y,
                corner.scale.x,
                corner.scale.y,
                corner.layer,
                corner.rotation
            )

            if spriteID > 0 then
                cornerSpriteIDs[corner.id] = spriteID
                Log("  ✓ Corner sprite '" .. corner.id .. "' created (ID: " .. spriteID .. ", rotation: " .. corner.rotation .. "°)")
            else
                Log("  ✗ FAILED to create corner sprite: " .. corner.id)
            end
        end

        Log("Corner decorations complete!")
    end

    local music = config.menu.music
    PlaySound(music.name, music.loop, music.volume)
    Log("Playing tutorial music: " .. music.name)

    ClearAllButtons()
    CreateButtonsFromConfig()
    
    initialized = true
    Log("Tutorial initialization complete - ALL TEXT AS BUTTONS")
    Log("Press F1 to toggle editor mode")
end

function CreateButtonsFromConfig()
    if not config or not config.menu or not config.menu.buttons then
        Log("ERROR: Invalid configuration - no buttons found!")
        return
    end
    
    Log("Creating " .. #config.menu.buttons .. " buttons from config...")
    
    for i, button in ipairs(config.menu.buttons) do
        Log("Creating button: " .. button.id)

        local layer = button.layer or 10
        local buttonID = CreateButton(
            button.texture,
            button.position.x,
            button.position.y,
            button.scale.x,
            button.scale.y,
            button.callback,
            layer
        )
        
        if buttonID > 0 then
            buttonIDs[button.id] = {
                id = buttonID,
                config = button
            }
            Log("  ✓ Button '" .. button.id .. "' created (ID: " .. buttonID .. ")")
        else
            Log("  ✗ Failed to create button: " .. button.id)
        end
    end
    
    Log("Button creation complete!")
end

function OnNextButtonClicked()
    if IsEditorMode() then
        Log("Button disabled in editor mode")
        return
    end
    
    Log("NEXT button clicked!")
    Log("Transitioning to Level Select...")
    SetNextGameState("Level_select")
end

function OnUpdate(dt)
    UpdateAudio(dt)
    
    if IsKeyDown("F1") then
        if editorToggleCooldown <= 0 then
            ToggleEditorMode()
            
            if IsEditorMode() then
                SetEnginePlayState(false)

                Log("EDITOR MODE ON - Buttons disabled")
            else
                SetEnginePlayState(true)

                Log("EDITOR MODE OFF - Buttons enabled")
            end
            
            editorToggleCooldown = 0.3
        end
    end
    
    if editorToggleCooldown > 0 then
        editorToggleCooldown = editorToggleCooldown - dt
    end
    
    if IsKeyDown("Escape") then
        Log("ESC pressed - skipping tutorial")
        OnNextButtonClicked()
    end
end

function OnDraw()
    if not config then
        return
    end
    
    local editorMode = IsEditorMode()
    
    -- All text is now rendered as button text
    
    -- Draw ALL button text (including text-only buttons)
    if buttonIDs then
        for buttonKey, buttonData in pairs(buttonIDs) do
            local button = buttonData.config
            local text = button.text
            
            local colorR = editorMode and 0.5 or text.color.r
            local colorG = editorMode and 0.5 or text.color.g
            local colorB = editorMode and 0.5 or text.color.b
            
            DrawButtonText(
                buttonData.id,
                text.font,
                text.content,
                text.offset.x,
                text.offset.y,
                text.scale,
                colorR,
                colorG,
                colorB
            )
        end
    end
    
    if editorMode then
        DrawText("Sans48", "EDITOR MODE", 50, 50, 0.8, 1.0, 0.3, 0.3)
    end
end

function OnDestroy()
    Log("Tutorial cleanup...")

    StopAllSounds()
    ClearAllButtons()

    if backgroundSpriteID > 0 then
        DestroyEntity(backgroundSpriteID)
        Log("Background sprite destroyed")
    end

    for overlayID, spriteID in pairs(overlaySprites) do
        if spriteID > 0 then
            DestroyEntity(spriteID)
            Log("Overlay sprite '" .. overlayID .. "' destroyed")
        end
    end

    for cornerID, spriteID in pairs(cornerSpriteIDs) do
        if spriteID > 0 then
            DestroyEntity(spriteID)
            Log("Corner sprite '" .. cornerID .. "' destroyed")
        end
    end

    buttonIDs = {}
    config = nil
    initialized = false
    backgroundSpriteID = 0
    overlaySprites = {}
    cornerSpriteIDs = {}

    Log("Tutorial cleanup complete")
end

function PrintConfig()
    if not config then
        Log("No configuration loaded!")
        return
    end
    
    Log("═══════════════════════════════════════")
    Log("Tutorial Configuration (All Buttons):")
    Log("  Menu Name: " .. config.menu.name)
    Log("  Music: " .. config.menu.music.name)
    Log("  Camera Zoom: " .. config.menu.camera.zoom)
    Log("  Total Buttons: " .. #config.menu.buttons)
    Log("═══════════════════════════════════════")
end