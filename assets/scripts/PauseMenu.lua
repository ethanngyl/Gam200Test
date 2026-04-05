--[[
===============================================================================
| File:          PauseMenu.lua
| Author:        GE YONGQI (50%), Carl Padilla Jameson (50%)
| Email:         yongqi.ge@digipen.edu , c.padilla@digipen.edu
| Date:          2026-01-27
| ------------------------------------------------------------------------------
|  Reusable Pause Menu System with Visual UI Elements
|
|  Features:
|  - Wood background with semi-transparent overlay
|  - Button images with hover highlight (like MainMenu)
|  - Horizontal layout: Quit | Settings | Resume
|  - Settings sub-menu with volume control
|
|  Usage:
|    local PauseMenu = require("PauseMenu")
|    PauseMenu.Init()           -- In OnInit
|    PauseMenu.Update(dt)       -- In OnUpdate
|    PauseMenu.Draw()           -- In OnDraw

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
--]]

local PauseMenu = {}
local SettingsMenu = require("SettingsMenu")
local SavePath = require("SavePath")

local ENEMY_DEX_BUTTONS = {
    {
        label = "PREV",
        offsetX = -0.34,
        offsetY = -0.35,
        callback = "OnEnemyDexPrevClicked",
        text = { offsetX = -44, offsetY = -12, scale = 0.8, color = { r = 255, g = 255, b = 255 } }
    },
    {
        label = "RETURN",
        offsetX = 0.0,
        offsetY = -0.35,
        callback = "OnEnemyDexReturnClicked",
        text = { offsetX = -66, offsetY = -12, scale = 0.8, color = { r = 255, g = 255, b = 255 } }
    },
    {
        label = "NEXT",
        offsetX = 0.34,
        offsetY = -0.35,
        callback = "OnEnemyDexNextClicked",
        text = { offsetX = -42, offsetY = -12, scale = 0.8, color = { r = 255, g = 255, b = 255 } }
    }
}

local ENEMY_DEX_PREVIEW = {
    offsetX = 0.56,
    offsetY = 0.16,
    scaleX = 0.24,
    scaleY = 0.24,
    layer = 52
}

-- ============================================================================
-- CONFIGURATION
-- ============================================================================

local config = {
    -- Background
    background = {
        texture = "assets/Menu/WoodBackground.png",
        scale = { x = 2.0, y = 1.5 },
        layer = 50
    },
    
    -- Scroll overlay (on top of background, behind buttons)
    overlay = {
        texture = "assets/Menu/Scroll Overlay.png",
        scale = { x = 2.25, y = 1.25 },
        layer = 50
    },

    -- Title
    title = {
        text = "Paused",
        scale = 2.0,
        color = { r = 0.2, g = 0.15, b = 0.1 }
    },
    
    -- Buttons (using CreateButton for hover effect)
    buttons = {
        texture = "assets/Menu/Ui_btn.png",
        scale = { x = 0.35, y = 0.10 },
        layer = 51,
        
        -- Layout:
        --   [Resume]   [Settings]
        --   [Restart]  [MainMenu]
        --       [Extra]
        items = {
            { 
                id = "resume", 
                label = "Resume",
                offsetX = -0.24,    -- World X offset from camera
                offsetY = 0.0,     -- World Y offset from camera (top row)
                callback = "OnPauseResumeClicked",
                text = {
                    offsetX = -60,
                    offsetY = -10,
                    scale = 0.8,
                    color = { r = 255, g = 255, b = 255 }
                }
            },
            { 
                id = "settings", 
                label = "Settings",
                offsetX = 0.24,
                offsetY = 0.0,
                callback = "OnPauseSettingsClicked",
                text = {
                    offsetX = -60,
                    offsetY = -10,
                    scale = 0.8,
                    color = { r = 255, g = 255, b = 255 }
                }
            },
            { 
                id = "restart", 
                label = "Restart",
                offsetX = -0.24,
                offsetY = -0.15,    -- Bottom row
                callback = "OnPauseRestartClicked",
                text = {
                    offsetX = -60,
                    offsetY = -15,
                    scale = 0.8,
                    color = { r = 255, g = 255, b = 255 }
                }
            },
            { 
                id = "quit", 
                label = "MainMenu",
                offsetX = 0.24,
                offsetY = -0.15,
                callback = "OnPauseQuitClicked",
                text = {
                    offsetX = -80,
                    offsetY = -15,
                    scale = 0.8,
                    color = { r = 255, g = 255, b = 255 }
                }
            },
            {
                id = "extra",
                label = "Enemies",
                offsetX = 0.0,
                offsetY = -0.30,
                callback = "OnPauseExtraClicked",
                text = {
                    offsetX = -66,
                    offsetY = -15,
                    scale = 0.8,
                    color = { r = 255, g = 255, b = 255 }
                }
            }
        }
    }
}

-- ============================================================================
-- STATE
-- ============================================================================

local state = {
    initialized = false,
    
    -- Entity IDs
    backgroundID = 0,
    overlayID = 0,
    buttonIDs = {},  -- Array of {id = buttonID, config = buttonConfig}

    -- Enemy Encyclopedia state
    enemyDexActive = false,
    enemyDexIndex = 1,
    enemyDexEntries = {},
    enemyDexButtonIDs = {},
    enemyDexPreviewID = 0,
    enemyDexPreviewIndex = -1,
    
    -- Input tracking
    wasEscapePressed = false,
    wasLeftPressed = false,
    wasRightPressed = false
}

local RecreatePauseButtons
local UpdateEnemyDexPreview

local function DestroyEnemyDexPreview()
    if state.enemyDexPreviewID and state.enemyDexPreviewID > 0 then
        DestroyEntity(state.enemyDexPreviewID)
        state.enemyDexPreviewID = 0
    end
    state.enemyDexPreviewIndex = -1
end

local function LoadEnemyDexEntries()
    state.enemyDexEntries = {
        {
            name = "Enemy Knight",
            anim = {
                tex = "assets/Enemy/Enemy_Knight_Idle_Front-Sheet.png",
                rows = 1,
                cols = 12,
                frames = 12,
                time = 0.08,
                loop = true
            },
            lines = {
                "Strike: Adjacent Tiles, 1 Damage, Consumes 1 AP.",
                "Summon Reinforcements: Skip next turn. On following turn, if still alive,",
                "summon another Enemy Knight on a random open tile. Costs 2 AP.",
                "Cooldown: usable every 2 turns.",
                "Usage condition: only when 2 or more enemies have died this level.",
                "HP 5, AP 2, MP 3"
            }
        },
        {
            name = "Enemy Mage",
            anim = {
                tex = "assets/Enemy/EnemyMage_Idle_Front-Sheet.png",
                rows = 4,
                cols = 3,
                frames = 12,
                time = 0.08,
                loop = true
            },
            lines = {
                "Arcane Bolt: Fires in facing direction, 1 Damage, Consumes 2 AP.",
                "Barrier: Blocks next damage taken for target character, Consumes 2 AP.",
                "Can only be used every 2 turns and is prioritized.",
                "Targets a random ally if possible; if no other enemies are alive,",
                "targets self.",
                "HP 3, AP 4, MP 3"
            }
        },
        {
            name = "Dark Knight (Level 1/2)",
            anim = {
                tex = "assets/Enemy/Boss1_Idle_Front-Sheet.png",
                rows = 4,
                cols = 3,
                frames = 12,
                time = 0.08,
                loop = true
            },
            lines = {
                "Strike: Adjacent Tiles, 2 Damage, Consumes 1 AP.",
                "Cross Impact: Cross slash reaching 3 tiles in each direction.",
                "Fated Encounter: Targets highest-health player for 3 turns and",
                "pathfinds only to them. While active, both deal +1 damage to each other.",
                "Always prioritized if there is no targeted player.",
                "Fated Hour: At 50% HP, one-time charge state. Ends current turn and",
                "skips next turn, then teleports adjacent to lowest-health player",
                "and executes that player.",
                "HP 10, 2 Actions Per Turn"
            }
        },
        {
            name = "Enemy Knight Commander",
            anim = {
                tex = "assets/Enemy/Enemy_Knight_Idle_Front-Sheet.png",
                rows = 1,
                cols = 12,
                frames = 12,
                time = 0.08,
                loop = true,
                tint = { 1.0, 0.9, 0.3 }
            },
            lines = {
                "Strike: Adjacent Tiles, 1 Damage, Consumes 1 AP.",
                "Rallying Cry: Enemies gain +1 movement point for next turn only.",
                "Consumes 2 AP and can only be used every 3 turns.",
                "Bolstered Morale: Enemies gain +1 Damage while this unit is alive.",
                "HP 3, AP 3, MP 3"
            }
        },
        {
            name = "Enemy Tank",
            anim = {
                tex = "assets/Enemy/Enemy_Knight_Idle_Front-Sheet.png",
                rows = 1,
                cols = 12,
                frames = 12,
                time = 0.08,
                loop = true,
                tint = { 0.3, 0.9, 0.3 }
            },
            lines = {
                "Heavy Armor: Takes 1 reduced damage from all sources (always active).",
                "Shield Bash: Adjacent tile, stuns target player, 2 damage, 3 AP.",
                "Taunt: While alive, ally damage is redirected to this unit.",
                "Used if an enemy has less than 50% health. Costs 2 AP.",
                "HP 7, AP 3, MP 2"
            }
        },
        {
            name = "Orc Shaman",
            anim = {
                tex = "assets/Enemy/Warlock_Boss_Idle_Front-Sheet.png",
                rows = 1,
                cols = 5,
                frames = 5,
                time = 0.12,
                loop = true
            },
            lines = {
                "Preparatory Rites: Always first use on activation. Starts an 8-turn",
                "countdown (excluding current turn). During countdown, boss cannot move.",
                "After countdown: empowered state, +5 Orc Warriors spawn randomly.",
                "Empowered aura: all enemies gain +1 Attack, +1 MP, +1 AP.",
                "Each broken totem deals 2 damage to boss and spawns 2 Orc Warriors.",
                "Totem of Unkilling: 3 HP totem; 5x5 zone sets fatal damage to 1 HP.",
                "Totem of Massacre: 3 HP totem; 5x5 zone, invulnerable while enemies",
                "remain in zone. Each enemy death in zone deals 1 damage to totem.",
                "At 0 HP, all entities in battle take 3 damage.",
                "Totem of Blight: 5 HP totem; 5x5 zone; after enemy phase, all entities",
                "take 1 damage. If entities in zone exceed 4, rites countdown -1.",
                "Totemic Blessing (Passive): While a totem is alive, boss takes 0 damage.",
                "HP 10, Conditional Skill Usage"
            }
        },
        {
            name = "Orc Warrior",
            anim = {
                tex = "assets/enemy/Boss_Minion_Idle_Front-Sheet.png",
                rows = 1,
                cols = 4,
                frames = 4,
                time = 0.12,
                loop = true
            },
            lines = {
                "Strong Swing: 3-tile horizontal range relative to facing direction.",
                "Consumes 2 AP, deals 2 Damage. No cooldown.",
                "Regenerate: Consumes 2 AP, heals 1 HP, 2-turn cooldown.",
                "Used when HP is less than or equal to 50%.",
                "HP 6, AP 2, MP 2"
            }
        }
    }
end

local function CloseEnemyDex()
    state.enemyDexActive = false
    ClearAllButtons()
    state.enemyDexButtonIDs = {}
    DestroyEnemyDexPreview()
    RecreatePauseButtons()
end

local function CreateEnemyDexButtons()
    local camX, camY, camZ = GetCameraPosition()
    local btnConfig = config.buttons
    state.enemyDexButtonIDs = {}

    for i, btn in ipairs(ENEMY_DEX_BUTTONS) do
        local buttonID = CreateButton(
            btnConfig.texture,
            camX + btn.offsetX,
            camY + btn.offsetY,
            btnConfig.scale.x,
            btnConfig.scale.y,
            btn.callback,
            btnConfig.layer
        )

        state.enemyDexButtonIDs[i] = {
            id = buttonID,
            config = btn
        }
    end
end

UpdateEnemyDexPreview = function(force)
    if not state.enemyDexActive then
        return
    end

    local entry = state.enemyDexEntries[state.enemyDexIndex]
    local anim = entry and entry.anim
    if not anim then
        DestroyEnemyDexPreview()
        return
    end

    if not force and state.enemyDexPreviewIndex == state.enemyDexIndex then
        return
    end

    DestroyEnemyDexPreview()

    if not SpawnAnimatedSprite then
        return
    end

    local camX, camY, camZ = GetCameraPosition()
    local previewID = SpawnAnimatedSprite(
        anim.tex,
        camX + ENEMY_DEX_PREVIEW.offsetX,
        camY + ENEMY_DEX_PREVIEW.offsetY,
        ENEMY_DEX_PREVIEW.scaleX,
        ENEMY_DEX_PREVIEW.scaleY,
        ENEMY_DEX_PREVIEW.layer,
        anim.rows,
        anim.cols,
        anim.frames,
        anim.time,
        anim.loop
    )

    if previewID and previewID > 0 then
        if SetSpriteColor then
            local tint = anim.tint
            if tint then
                SetSpriteColor(previewID, tint[1] or 1.0, tint[2] or 1.0, tint[3] or 1.0)
            else
                SetSpriteColor(previewID, 1.0, 1.0, 1.0)
            end
        end

        state.enemyDexPreviewID = previewID
        state.enemyDexPreviewIndex = state.enemyDexIndex
    end
end

local function DrawEnemyDex()
    local fbWidth, fbHeight = GetFramebufferSize()
    local centerX = fbWidth * 0.5
    local centerY = fbHeight * 0.5

    local scaleFactorX = fbWidth / 1920
    local scaleFactorY = fbHeight / 1080
    local scaleFactor = math.min(scaleFactorX, scaleFactorY)

    local entry = state.enemyDexEntries[state.enemyDexIndex]
    if not entry then return end

    local headerScale = 1.10 * scaleFactor
    local bodyScale = 0.72 * scaleFactor
    local lineStep = 36 * scaleFactorY

    DrawText("Jersey20Regular", entry.name, centerX - 610 * scaleFactorX, centerY + 250 * scaleFactorY,
        headerScale, 0.0, 0.0, 0.0)

    local y = centerY + 182 * scaleFactorY
    for _, line in ipairs(entry.lines or {}) do
        DrawText("Jersey20Regular", line, centerX - 610 * scaleFactorX, y,
            bodyScale, 0.0, 0.0, 0.0)
        y = y - lineStep
    end

    local pageText = tostring(state.enemyDexIndex) .. " / " .. tostring(#state.enemyDexEntries)
    DrawText("Jersey20Regular", pageText, centerX + 530 * scaleFactorX, centerY + 248 * scaleFactorY,
        0.75 * scaleFactor, 0.0, 0.0, 0.0)

    for _, btnData in ipairs(state.enemyDexButtonIDs) do
        if btnData.id and btnData.id > 0 then
            local textCfg = btnData.config.text
            DrawButtonText(
                btnData.id,
                "Playfair48",
                btnData.config.label,
                textCfg.offsetX,
                textCfg.offsetY,
                textCfg.scale,
                textCfg.color.r,
                textCfg.color.g,
                textCfg.color.b
            )
        end
    end
end

-- ============================================================================
-- GLOBAL BUTTON CALLBACKS (called by CreateButton system)
-- ============================================================================

function OnPauseQuitClicked()
    PlaySound("button2", false, 0.7)
    PauseMenu.OnQuit()
end

function OnPauseSettingsClicked()
    PlaySound("button2", false, 0.7)
    PauseMenu.OnSettings()
end

function OnPauseResumeClicked()
    PlaySound("button2", false, 0.7)
    PauseMenu.OnResume()
end

function OnPauseRestartClicked()
    PlaySound("button2", false, 0.7)
    PauseMenu.OnRestart()
end

function OnPauseExtraClicked()
    PlaySound("button2", false, 0.7)
    PauseMenu.OnEnemyDex()
end

function OnEnemyDexPrevClicked()
    PlaySound("button2", false, 0.7)
    PauseMenu.OnEnemyDexPrev()
end

function OnEnemyDexReturnClicked()
    PlaySound("button2", false, 0.7)
    PauseMenu.OnEnemyDexReturn()
end

function OnEnemyDexNextClicked()
    PlaySound("button2", false, 0.7)
    PauseMenu.OnEnemyDexNext()
end

-- ============================================================================
-- INITIALIZATION
-- ============================================================================

function PauseMenu.Init()
    state.initialized = true
    
    -- Reset input states
    state.wasEscapePressed = false
    state.wasLeftPressed = false
    state.wasRightPressed = false
    state.enemyDexActive = false
    state.enemyDexIndex = 1

    LoadEnemyDexEntries()
    
    -- Initialize settings menu
    SettingsMenu.Init()
    
    Log("[PauseMenu] Initialized")
end

-- ============================================================================
-- CREATE UI ELEMENTS (called when paused)
-- ============================================================================

local function CreatePauseUI()
    local camX, camY, camZ = GetCameraPosition()
    
    -- Create background
    state.backgroundID = SpawnSprite(
        config.background.texture,
        camX,
        camY,
        config.background.scale.x,
        config.background.scale.y,
        config.background.layer
    )
    
    -- Create scroll overlay on top of background
    state.overlayID = SpawnSprite(
        config.overlay.texture,
        camX,
        camY,
        config.overlay.scale.x,
        config.overlay.scale.y,
        config.overlay.layer
    )
    
    -- Create buttons using CreateButton (with hover highlight)
    local btnConfig = config.buttons
    state.buttonIDs = {}

    for i, btn in ipairs(btnConfig.items) do
        local btnX = camX + btn.offsetX
        local btnY = camY + btn.offsetY

        local buttonID = CreateButton(
            btnConfig.texture,
            btnX, btnY,
            btnConfig.scale.x, btnConfig.scale.y,
            btn.callback,
            btnConfig.layer
        )

        state.buttonIDs[i] = {
            id = buttonID,
            config = btn
        }

        Log("[PauseMenu] Created button: " .. btn.id .. " (ID: " .. buttonID .. ")")
    end
    
    Log("[PauseMenu] UI created")
end

RecreatePauseButtons = function()
    local camX, camY, camZ = GetCameraPosition()
    local btnConfig = config.buttons
    state.buttonIDs = {}

    for i, btn in ipairs(btnConfig.items) do
        local btnX = camX + btn.offsetX
        local btnY = camY + btn.offsetY

        local buttonID = CreateButton(
            btnConfig.texture,
            btnX, btnY,
            btnConfig.scale.x, btnConfig.scale.y,
            btn.callback,
            btnConfig.layer
        )

        state.buttonIDs[i] = {
            id = buttonID,
            config = btn
        }
    end
end

local function PrevEnemyDexPage()
    if #state.enemyDexEntries == 0 then
        return
    end

    state.enemyDexIndex = state.enemyDexIndex - 1
    if state.enemyDexIndex < 1 then
        state.enemyDexIndex = #state.enemyDexEntries
    end

    UpdateEnemyDexPreview(true)
end

local function NextEnemyDexPage()
    if #state.enemyDexEntries == 0 then
        return
    end

    state.enemyDexIndex = state.enemyDexIndex + 1
    if state.enemyDexIndex > #state.enemyDexEntries then
        state.enemyDexIndex = 1
    end

    UpdateEnemyDexPreview(true)
end

-- ============================================================================
-- DESTROY UI ELEMENTS (called when resumed)
-- ============================================================================

local function DestroyPauseUI()
    -- Destroy background
    if state.backgroundID and state.backgroundID > 0 then
        DestroyEntity(state.backgroundID)
        state.backgroundID = 0
    end
    
    -- Destroy scroll overlay
    if state.overlayID and state.overlayID > 0 then
        DestroyEntity(state.overlayID)
        state.overlayID = 0
    end
    
    -- Clear all buttons (created with CreateButton)
    ClearAllButtons()
    state.buttonIDs = {}
    state.enemyDexButtonIDs = {}
    DestroyEnemyDexPreview()
    
    Log("[PauseMenu] UI destroyed")
end

-- ============================================================================
-- UPDATE
-- ============================================================================

function PauseMenu.Update(dt)
    -- Don't process pause input when SkillSwapUI is active
    if _G.SkillSwapUI and _G.SkillSwapUI.IsActive and _G.SkillSwapUI.IsActive() then
        return
    end

    -- If settings menu is active, let it handle input
    if SettingsMenu.IsActive() then
        SettingsMenu.Update(dt)
        return
    end

    if state.enemyDexActive then
        local leftPressed = IsKeyDown("Left") or IsKeyDown("A")
        local rightPressed = IsKeyDown("Right") or IsKeyDown("D")
        local escapePressed = IsKeyDown("Escape")

        if leftPressed and not state.wasLeftPressed then
            PrevEnemyDexPage()
        end

        if rightPressed and not state.wasRightPressed then
            NextEnemyDexPage()
        end

        if escapePressed and not state.wasEscapePressed then
            CloseEnemyDex()
        end

        state.wasLeftPressed = leftPressed
        state.wasRightPressed = rightPressed
        state.wasEscapePressed = escapePressed
        return
    end
    
    -- Toggle pause with Escape key
    local isEscapePressed = IsKeyDown("Escape")
    
    if isEscapePressed and not state.wasEscapePressed then
        TogglePause()
        
        if IsPaused() then
            Log("[PauseMenu] Game PAUSED")
            -- Lower volume to 30% of the saved volume setting
            local savedVolume = SettingsMenu.GetSavedVolume()
            SetMasterVolume(savedVolume * 0.3)
            CreatePauseUI()
        else
            Log("[PauseMenu] Game RESUMED")
            -- Restore to saved volume setting
            SetMasterVolume(SettingsMenu.GetSavedVolume())
            DestroyPauseUI()
        end
    end
    state.wasEscapePressed = isEscapePressed
end

-- ============================================================================
-- DRAW
-- ============================================================================

function PauseMenu.Draw()
    if not IsPaused() then
        return
    end

    -- Don't draw pause menu when SkillSwapUI is showing
    if _G.SkillSwapUI and _G.SkillSwapUI.IsActive and _G.SkillSwapUI.IsActive() then
        return
    end
    
    -- If settings menu is active, draw it instead
    if SettingsMenu.IsActive() then
        SettingsMenu.Draw()
        return
    end

    if state.enemyDexActive then
        DrawEnemyDex()
        return
    end
    
    local fbWidth, fbHeight = GetFramebufferSize()
    local centerX = fbWidth * 0.5
    local centerY = fbHeight * 0.5
    
    -- Screen scale factor (based on 1920x1080 reference resolution)
    local scaleFactorX = fbWidth / 1920
    local scaleFactorY = fbHeight / 1080
    local scaleFactor = math.min(scaleFactorX, scaleFactorY)
    
    -- ========================================================================
    -- DRAW TITLE
    -- ========================================================================
    local titleCfg = config.title
    local titleText = titleCfg.text
    local titleScale = titleCfg.scale * scaleFactor
    
    -- Center the title
    local approxTitleWidth = #titleText * 30 * titleScale
    local titleX = centerX - (approxTitleWidth * 0.5)
    local titleY = centerY + 150 * scaleFactorY  -- Above the buttons
    
    DrawText("Playfair48", titleText, titleX, titleY, titleScale,
             titleCfg.color.r, titleCfg.color.g, titleCfg.color.b)
    
    -- ========================================================================
    -- DRAW BUTTON TEXT (using DrawButtonText for proper positioning)
    -- ========================================================================
    for i, btnData in ipairs(state.buttonIDs) do
        if btnData.id and btnData.id > 0 then
            local textCfg = btnData.config.text
            DrawButtonText(
                btnData.id,
                "Playfair48",
                btnData.config.label,
                textCfg.offsetX,
                textCfg.offsetY,
                textCfg.scale,
                textCfg.color.r,
                textCfg.color.g,
                textCfg.color.b
            )
        end
    end
end

-- ============================================================================
-- CALLBACKS
-- ============================================================================

function PauseMenu.OnResume()
    state.enemyDexActive = false
    TogglePause()
    -- Restore to saved volume setting
    SetMasterVolume(SettingsMenu.GetSavedVolume())
    DestroyPauseUI()
    Log("[PauseMenu] Resumed")
end

function PauseMenu.OnSettings()
    state.enemyDexActive = false
    -- Hide pause menu UI elements
    DestroyPauseUI()
    
    -- Open settings menu with callback to restore pause menu when closed
    SettingsMenu.Open(function()
        -- When settings closes, recreate pause menu UI
        CreatePauseUI()
    end)
    
    Log("[PauseMenu] Opening Settings")
end

function PauseMenu.OnRestart()
    state.enemyDexActive = false
    -- Reset campaign progress back to level 1
    local f = io.open(SavePath.GetLevelProgressPath(), "w")
    if f then
        f:write("{\n  \"currentLevel\": 1,\n  \"totalLevels\": 3\n}\n")
        f:close()
        Log("[PauseMenu] Level progress reset to 1")
    end

    DestroyPauseUI()
    TogglePause()
    -- Restore to saved volume setting
    SetMasterVolume(SettingsMenu.GetSavedVolume())
    SetNextGameState("LEVEL_3")
    Log("[PauseMenu] Restarting campaign from Level 1")
end

function PauseMenu.OnQuit()
    state.enemyDexActive = false
    DestroyPauseUI()
    TogglePause()
    -- Restore to saved volume setting
    SetMasterVolume(SettingsMenu.GetSavedVolume())
    SetNextGameState("mainMenu")
    Log("[PauseMenu] Returning to main menu")
end

function PauseMenu.OnEnemyDex()
    state.enemyDexActive = true
    state.enemyDexIndex = 1
    state.wasLeftPressed = false
    state.wasRightPressed = false
    state.wasEscapePressed = false
    ClearAllButtons()
    state.buttonIDs = {}
    CreateEnemyDexButtons()
    UpdateEnemyDexPreview(true)
end

function PauseMenu.OnEnemyDexPrev()
    PrevEnemyDexPage()
end

function PauseMenu.OnEnemyDexNext()
    NextEnemyDexPage()
end

function PauseMenu.OnEnemyDexReturn()
    CloseEnemyDex()
end

-- ============================================================================
-- CLEANUP
-- ============================================================================

function PauseMenu.Destroy()
    SettingsMenu.Destroy()
    DestroyPauseUI()
    state.initialized = false
    Log("[PauseMenu] Destroyed")
end

-- ============================================================================
-- RETURN MODULE
-- ============================================================================

return PauseMenu
