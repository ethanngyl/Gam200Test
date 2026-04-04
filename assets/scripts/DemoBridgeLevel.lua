local fullText = "The imperial squadron has staged a failed coup against the current king and are on the run.\nYour party has been hired to hunt down these rebel knights covertly to avoid public hysteira.\nThey seem to have taken refuge in a nearby forest, hunt down the traitors\nand find the mastermind behind this devious plot."
local visibleChars = 0
local revealRate = 34.0          -- characters per second
local holdAfterComplete = 1.0    -- seconds before loading Level 1
local doneTimer = 0.0
local startedTransition = false
local backgroundSpriteID = 0
local scrollOverlaySpriteID = 0

local function getVisibleText()
    local n = math.max(0, math.floor(visibleChars))
    return string.sub(fullText, 1, n)
end

function OnInit()
    Log("DemoBridgeLevel initialized")

    SetCameraPosition(0.0, 0.0, 0.0)
    SetCameraZoom(1.0)
    DisableImGui()
    SetEnginePlayState(true)

    backgroundSpriteID = SpawnSprite(
           "assets/Menu/WoodBackground.png",
        0.0,
        0.0,
        4.2,
        2.4,
        2
    )

    scrollOverlaySpriteID = SpawnSprite(
        "assets/Menu/Scroll Overlay.png",
        0.0,
        0.0,
        4.2,
        2.4,
        6
    )

    visibleChars = 0
    doneTimer = 0.0
    startedTransition = false
end

function OnUpdate(dt)
    if startedTransition then
        return
    end

    local totalChars = string.len(fullText)
    if visibleChars < totalChars then
        visibleChars = math.min(totalChars, visibleChars + revealRate * dt)
    else
        doneTimer = doneTimer + dt
        if doneTimer >= holdAfterComplete then
            startedTransition = true
            SetNextGameState("LEVEL_3")
        end
    end

    -- Optional skip once text is complete.
    if visibleChars >= totalChars and IsKeyDown("Y") then
        startedTransition = true
        SetNextGameState("LEVEL_3")
    end
end

function OnDraw()
    local fbW, fbH = GetFramebufferSize()
    if not fbW or fbW <= 0 then return end

    local scaleRef = fbW / 1920
    local textX = fbW * 0.17
    local startY = fbH * 0.62
    local lineSpacing = 48 * scaleRef

    local shown = getVisibleText()
    local lineIndex = 0
    for line in shown:gmatch("[^\n]+") do
        DrawText("Jersey20Regular", line, textX, startY - (lineIndex * lineSpacing), 0.62 * scaleRef, 0.06, 0.06, 0.06)
        lineIndex = lineIndex + 1
    end
end

function OnDestroy()
    if backgroundSpriteID and backgroundSpriteID > 0 then
        DestroyEntity(backgroundSpriteID)
        backgroundSpriteID = 0
    end
    if scrollOverlaySpriteID and scrollOverlaySpriteID > 0 then
        DestroyEntity(scrollOverlaySpriteID)
        scrollOverlaySpriteID = 0
    end
end
