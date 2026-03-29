#include "Precompiled.h"
#include "EngineScriptAPIProvider.h"

#include "LevelLoader.h"

namespace Framework::EngineScriptAPIProvider {

void Register(lua_State* L)
{
    // Logging
    lua_register(L, "Log", LevelLoader::Lua_Log);

    // Camera
    lua_register(L, "SetCameraPosition", LevelLoader::Lua_SetCameraPosition);
    lua_register(L, "SetCameraZoom", LevelLoader::Lua_SetCameraZoom);
    lua_register(L, "SetCameraFollowTarget", LevelLoader::Lua_SetCameraFollowTarget);
    lua_register(L, "GetFramebufferSize", LevelLoader::Lua_GetFramebufferSize);

    // Engine control
    lua_register(L, "SetEnginePlayState", LevelLoader::Lua_SetEnginePlayState);
    lua_register(L, "SetNextGameState", LevelLoader::Lua_SetNextGameState);

    // ImGui
    lua_register(L, "ToggleEditor", LevelLoader::Lua_ToggleEditor);
    lua_register(L, "IsEditorEnabled", LevelLoader::Lua_IsEditorEnabled);
    lua_register(L, "SetEditorMode", LevelLoader::Lua_SetEditorMode);
    lua_register(L, "DisableImGui", LevelLoader::Lua_DisableImGui);
    lua_register(L, "EnableImGui", LevelLoader::Lua_EnableImGui);

    // Pause control
    lua_register(L, "TogglePause", LevelLoader::Lua_TogglePause);
    lua_register(L, "IsPaused", LevelLoader::Lua_IsPaused);

    // Audio
    lua_register(L, "PlaySound", LevelLoader::Lua_PlaySound);
    lua_register(L, "PlayMusic", LevelLoader::Lua_PlayMusic);
    lua_register(L, "StopMusic", LevelLoader::Lua_StopMusic);
    lua_register(L, "StopSound", LevelLoader::Lua_StopSound);
    lua_register(L, "StopAllSounds", LevelLoader::Lua_StopAllSounds);
    lua_register(L, "UpdateAudio", LevelLoader::Lua_UpdateAudio);
    lua_register(L, "SetMasterVolume", LevelLoader::Lua_SetMasterVolume);
    lua_register(L, "GetMasterVolume", LevelLoader::Lua_GetMasterVolume);
    lua_register(L, "SaveMasterVolume", LevelLoader::Lua_SaveMasterVolume);
    lua_register(L, "SetMusicVolume", LevelLoader::Lua_SetMusicVolume);
    lua_register(L, "GetMusicVolume", LevelLoader::Lua_GetMusicVolume);
    lua_register(L, "SaveMusicVolume", LevelLoader::Lua_SaveMusicVolume);
    lua_register(L, "SetSfxVolume", LevelLoader::Lua_SetSfxVolume);
    lua_register(L, "GetSfxVolume", LevelLoader::Lua_GetSfxVolume);
    lua_register(L, "SaveSfxVolume", LevelLoader::Lua_SaveSfxVolume);

    // UI Buttons
    lua_register(L, "CreateButton", LevelLoader::Lua_CreateButton);
    lua_register(L, "ClearAllButtons", LevelLoader::Lua_ClearAllButtons);
    lua_register(L, "DrawButtonText", LevelLoader::Lua_DrawButtonText);
    lua_register(L, "DrawText", LevelLoader::Lua_DrawText);
    lua_register(L, "WorldToScreen", LevelLoader::Lua_WorldToScreen);

    // Input
    lua_register(L, "IsKeyDown", LevelLoader::Lua_IsKeyDown);
    lua_register(L, "IsMouseButtonDown", LevelLoader::Lua_IsMouseButtonDown);
    lua_register(L, "IsMouseButtonPressed", LevelLoader::Lua_IsMouseButtonPressed);
    lua_register(L, "GetMousePosition", LevelLoader::Lua_GetMousePosition);
    lua_register(L, "GetMouseWorldPosition", LevelLoader::Lua_GetMouseWorldPosition);

    // JSON and level loading
    lua_register(L, "LoadJSON", LevelLoader::Lua_LoadJSON);
    lua_register(L, "LoadTileMap", LevelLoader::Lua_LoadTileMap);
}

} // namespace Framework::EngineScriptAPIProvider
