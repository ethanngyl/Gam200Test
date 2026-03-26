#include "Precompiled.h"
#include "Level2StateHooks.h"

#include "GameStateList.h"
#include "GameStateManager.h"
#include "LevelLoader.h"
#include "StateScriptRegistry.h"
#include "TimeConstants.h"

namespace Framework::Level2StateHooks {

void OnLoad()
{
    Framework::LevelLoader::GetInstance().LoadLevel(
        Framework::StateScriptRegistry::GetLevelScript(LEVEL_2),
        g_loadAsEditorMode
    );
    g_loadAsEditorMode = false;
}

void OnInitialize()
{
    // LevelLoader handles init via OnInit().
}

void OnUpdate()
{
    Framework::LevelLoader::GetInstance().UpdateCurrentLevel(
        Framework::Time::FIXED_DT
    );
}

void OnDraw()
{
    Framework::LevelLoader::GetInstance().DrawCurrentLevel();
}

void OnFree()
{
    Framework::LevelLoader::GetInstance().UnloadCurrentLevel();
}

void OnUnload()
{
    // Cleanup handled by LevelLoader.
}

} // namespace Framework::Level2StateHooks
