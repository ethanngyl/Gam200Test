#include "Precompiled.h"
#include "GameScriptAPIProvider.h"

#include "LevelLoader.h"

namespace Framework::GameScriptAPIProvider {

void Register(lua_State* L)
{
    // AP Indicator / Entity Management
    lua_register(L, "SpawnSprite", LevelLoader::Lua_SpawnSprite);
    lua_register(L, "SpawnAnimatedSprite", LevelLoader::Lua_SpawnAnimatedSprite);
    lua_register(L, "SetSpriteAnimationSheet", LevelLoader::Lua_SetSpriteAnimationSheet);
    lua_register(L, "SetSpriteColor", LevelLoader::Lua_SetSpriteColor);
    lua_register(L, "TintTile", LevelLoader::Lua_TintTile);
    lua_register(L, "SetSpriteGray", LevelLoader::Lua_SetSpriteGray);
    lua_register(L, "SetSpriteTexture", LevelLoader::Lua_SetSpriteTexture);
    lua_register(L, "SetSpritePosition", LevelLoader::Lua_SetSpritePosition);
    lua_register(L, "SetScale", LevelLoader::Lua_SetScale);
    lua_register(L, "SetSpriteVisibility", LevelLoader::Lua_SetSpriteVisibility);
    lua_register(L, "SetSpriteBlendMode", LevelLoader::Lua_SetSpriteBlendMode);
    lua_register(L, "SetSpriteFilterMode", LevelLoader::Lua_SetSpriteFilterMode);
    lua_register(L, "SetSpriteUVRect", LevelLoader::Lua_SetSpriteUVRect);
    lua_register(L, "DestroyEntity", LevelLoader::Lua_DestroyEntity);
    lua_register(L, "IsEntityValid", LevelLoader::Lua_IsEntityValid);
    lua_register(L, "SetEntityRotation", LevelLoader::Lua_SetEntityRotation);
    lua_register(L, "SetEntityScale", LevelLoader::Lua_SetEntityScale);
    lua_register(L, "SetSharedInt", LevelLoader::Lua_SetSharedInt);
    lua_register(L, "GetSharedInt", LevelLoader::Lua_GetSharedInt);
    lua_register(L, "ClearAllEntities", LevelLoader::Lua_ClearAllEntities);

    lua_register(L, "GetPlayerAP", LevelLoader::Lua_GetPlayerAP);
    lua_register(L, "GetCameraPosition", LevelLoader::Lua_GetCameraPosition);
    lua_register(L, "GetPlayerAttackAP", LevelLoader::Lua_GetPlayerAttackAP);
    lua_register(L, "GetPlayerHP", LevelLoader::Lua_GetPlayerHP);

    // Enemy AI Configuration
    lua_register(L, "FindPlayer", LevelLoader::Lua_FindPlayer);
    lua_register(L, "GetAllPlayers", LevelLoader::Lua_GetAllPlayers);
    lua_register(L, "GetAllEnemies", LevelLoader::Lua_GetAllEnemies);
    lua_register(L, "SetEnemyTarget", LevelLoader::Lua_SetEnemyTarget);
    lua_register(L, "GetCurrentTurn", LevelLoader::Lua_GetCurrentTurn);
    lua_register(L, "GetChestProgress", LevelLoader::Lua_GetChestProgress);

    // Enemy Turn Management System
    lua_register(L, "InitializeEnemyTurn", LevelLoader::Lua_InitializeEnemyTurn);
    lua_register(L, "IsActiveEnemy", LevelLoader::Lua_IsActiveEnemy);
    lua_register(L, "IsEnemyActionReady", LevelLoader::Lua_IsEnemyActionReady);
    lua_register(L, "MarkEnemyActionComplete", LevelLoader::Lua_MarkEnemyActionComplete);
    lua_register(L, "UpdateEnemyTurnManager", LevelLoader::Lua_UpdateEnemyTurnManager);

    // Animation
    lua_register(L, "LoadAnimationConfig", LevelLoader::Lua_LoadAnimationConfig);
    lua_register(L, "LoadPlayerAnimation", LevelLoader::Lua_LoadPlayerAnimation);
    lua_register(L, "LoadAnimationForEntity", LevelLoader::Lua_LoadAnimationForEntity);
    lua_register(L, "LoadAnimationForAllPlayers", LevelLoader::Lua_LoadAnimationForAllPlayers);
    lua_register(L, "LoadAnimationForAllEnemies", LevelLoader::Lua_LoadAnimationForAllEnemies);

    // Scroll Animation API
    lua_register(L, "PlayAnimationByName", LevelLoader::Lua_PlayAnimationByName);
    lua_register(L, "SetAnimationFrame", LevelLoader::Lua_SetAnimationFrame);
    lua_register(L, "GetAnimationFrame", LevelLoader::Lua_GetAnimationFrame);
    lua_register(L, "GetAnimationFrameCount", LevelLoader::Lua_GetAnimationFrameCount);

    // Animation Control API
    lua_register(L, "SetAnimationGroup", LevelLoader::Lua_SetAnimationGroup);
    lua_register(L, "SetAnimationDirection", LevelLoader::Lua_SetAnimationDirection);
    lua_register(L, "SetAnimationFlipX", LevelLoader::Lua_SetAnimationFlipX);
    lua_register(L, "GetAnimationDirection", LevelLoader::Lua_GetAnimationDirection);
    lua_register(L, "GetAnimationFlipX", LevelLoader::Lua_GetAnimationFlipX);
    lua_register(L, "SetAnimationPlaying", LevelLoader::Lua_SetAnimationPlaying);
    lua_register(L, "SetAnimationPrefix", LevelLoader::Lua_SetAnimationPrefix);
    lua_register(L, "SetAnimationLoop", LevelLoader::Lua_SetAnimationLoop);
    lua_register(L, "SetAnimationFrameRange", LevelLoader::Lua_SetAnimationFrameRange);
    lua_register(L, "GetAnimationGroup", LevelLoader::Lua_GetAnimationGroup);
    lua_register(L, "GetEntityMovementDirection", LevelLoader::Lua_GetEntityMovementDirection);

    // Party System
    lua_register(L, "GetEntityAP", LevelLoader::Lua_GetEntityAP);
    lua_register(L, "GetEntityAttackAP", LevelLoader::Lua_GetEntityAttackAP);
    lua_register(L, "ConsumeEntityAP", LevelLoader::Lua_ConsumeEntityAP);
    lua_register(L, "ConsumeEntityAttackAP", LevelLoader::Lua_ConsumeEntityAttackAP);
    lua_register(L, "RefillEntityAP", LevelLoader::Lua_RefillEntityAP);
    lua_register(L, "RefillEntityAttackAP", LevelLoader::Lua_RefillEntityAttackAP);
    lua_register(L, "GetEntityHP", LevelLoader::Lua_GetEntityHP);
    lua_register(L, "SetEntityHP", LevelLoader::Lua_SetEntityHP);
    lua_register(L, "IsActiveCharacter", LevelLoader::Lua_IsActiveCharacter);

    // Script Component Management
    lua_register(L, "AddScriptComponentToEntity", LevelLoader::Lua_AddScriptComponentToEntity);
    lua_register(L, "RemoveScriptComponentFromEntity", LevelLoader::Lua_RemoveScriptComponentFromEntity);

    // Player Grid Movement API
    lua_register(L, "GetPlayerGridPosition", LevelLoader::Lua_GetPlayerGridPosition);
    lua_register(L, "IsValidGridPosition", LevelLoader::Lua_IsValidGridPosition);
    lua_register(L, "IsWalkableTile", LevelLoader::Lua_IsWalkableTile);
    lua_register(L, "IsTileWall", LevelLoader::Lua_IsTileWall);
    lua_register(L, "MovePlayerToTile", LevelLoader::Lua_MovePlayerToTile);
    lua_register(L, "SetGridMovementEnabled", LevelLoader::Lua_SetGridMovementEnabled);
    lua_register(L, "ShowTileBorder", LevelLoader::Lua_ShowTileBorder);
    lua_register(L, "PulseTile", LevelLoader::Lua_PulseTile);
    lua_register(L, "ConsumePlayerAP", LevelLoader::Lua_ConsumePlayerAP);
    lua_register(L, "RefillPlayerAP", LevelLoader::Lua_RefillPlayerAP);
    lua_register(L, "GetTurnIndex", LevelLoader::Lua_GetTurnIndex);
    lua_register(L, "EndPlayerTurn", LevelLoader::Lua_EndPlayerTurn);
    lua_register(L, "EndEnemyTurn", LevelLoader::Lua_EndEnemyTurn);
    lua_register(L, "CallLevelFunction", LevelLoader::Lua_CallLevelFunction);
    lua_register(L, "SetPlayerFlipX", LevelLoader::Lua_SetPlayerFlipX);
    lua_register(L, "HasChestAtTile", LevelLoader::Lua_HasChestAtTile);
    lua_register(L, "CollectChest", LevelLoader::Lua_CollectChest);
    lua_register(L, "HasGoalAtTile", LevelLoader::Lua_HasGoalAtTile);

    // Enemy/Entity API
    lua_register(L, "GetEnemyAP", LevelLoader::Lua_GetEnemyAP);
    lua_register(L, "RefillEnemyAP", LevelLoader::Lua_RefillEnemyAP);
    lua_register(L, "GetEntityGridPosition", LevelLoader::Lua_GetEntityGridPosition);
    lua_register(L, "GetEntityWorldPosition", LevelLoader::Lua_GetEntityWorldPosition);
    lua_register(L, "MoveEntityToTile", LevelLoader::Lua_MoveEntityToTile);
    lua_register(L, "ConsumeEnemyAP", LevelLoader::Lua_ConsumeEnemyAP);
    lua_register(L, "DamageEntity", LevelLoader::Lua_DamageEntity);
    lua_register(L, "GetDamageModifier", LevelLoader::Lua_GetDamageModifier);
    lua_register(L, "SetDamageModifier", LevelLoader::Lua_SetDamageModifier);
    lua_register(L, "FindPathToTarget", LevelLoader::Lua_FindPathToTarget);

    // Tile Occupancy
    lua_register(L, "SetTileOccupant", LevelLoader::Lua_SetTileOccupant);
    lua_register(L, "GetTileOccupant", LevelLoader::Lua_GetTileOccupant);
    lua_register(L, "IsTileOccupied", LevelLoader::Lua_IsTileOccupied);

    // Grid conversion
    lua_register(L, "TileToWorld", LevelLoader::Lua_TileToWorld);
    lua_register(L, "ScreenToTile", LevelLoader::Lua_ScreenToTile);

    // Entity-specific aliases
    lua_register(L, "GetEntityAP", LevelLoader::Lua_GetEntityAP);
    lua_register(L, "GetEntityHP", LevelLoader::Lua_GetEntityHP);
    lua_register(L, "SetEntityHP", LevelLoader::Lua_SetEntityHP);
    lua_register(L, "SetEntityMaxAP", LevelLoader::Lua_SetEntityMaxAP);
    lua_register(L, "RefillEntityAP", LevelLoader::Lua_RefillEntityAP);
    lua_register(L, "RefillEntityAttackAP", LevelLoader::Lua_RefillEntityAttackAP);
    lua_register(L, "ConsumeEntityAP", LevelLoader::Lua_ConsumeEntityAP);
    lua_register(L, "SetActiveCharacter", LevelLoader::Lua_SetActiveCharacter);
    lua_register(L, "IsActiveCharacter", LevelLoader::Lua_IsActiveCharacter);

    lua_register(L, "ToggleEditorMode", LevelLoader::lua_ToggleEditorMode);
    lua_register(L, "IsEditorMode", LevelLoader::lua_IsEditorMode);
    lua_register(L, "ShouldDisableGameplay", LevelLoader::Lua_ShouldDisableGameplay);

    // Save/Load
    lua_register(L, "SaveSceneToJSON", LevelLoader::Lua_SaveSceneToJSON);
    lua_register(L, "LoadSceneFromJSON", LevelLoader::Lua_LoadSceneFromJSON);
    lua_register(L, "AutoSaveScene", LevelLoader::Lua_AutoSaveScene);
    lua_register(L, "LoadAutoSave", LevelLoader::Lua_LoadAutoSave);
    lua_register(L, "HasAutoSave", LevelLoader::Lua_HasAutoSave);
    lua_register(L, "ClearAutoSave", LevelLoader::Lua_ClearAutoSave);

    // Procedural/Map serializer
    lua_register(L, "LoadProceduralMap", LevelLoader::Lua_LoadProceduralMap);
    lua_register(L, "SaveCurrentMap", LevelLoader::Lua_SaveCurrentMap);
    lua_register(L, "LoadSavedMap", LevelLoader::Lua_LoadSavedMap);
    lua_register(L, "ListSavedMaps", LevelLoader::Lua_ListSavedMaps);

    // Entity spawning
    lua_register(L, "SpawnPlayerAt", LevelLoader::Lua_SpawnPlayerAt);
    lua_register(L, "RemoveMovementComponent", LevelLoader::Lua_RemoveMovementComponent);
    lua_register(L, "InitializeTurnSystem", LevelLoader::Lua_InitializeTurnSystem);
    lua_register(L, "SpawnEnemyAt", LevelLoader::Lua_SpawnEnemyAt);
    lua_register(L, "SpawnChestAt", LevelLoader::Lua_SpawnChestAt);
    lua_register(L, "SpawnGoalAt", LevelLoader::Lua_SpawnGoalAt);

    // Status effects
    lua_register(L, "ApplyStatusEffect", LevelLoader::Lua_ApplyStatusEffect);
    lua_register(L, "HasStatusEffect", LevelLoader::Lua_HasStatusEffect);
    lua_register(L, "RemoveStatusEffect", LevelLoader::Lua_RemoveStatusEffect);
    lua_register(L, "DecrementStatusEffects", LevelLoader::Lua_DecrementStatusEffects);
    lua_register(L, "GetStatusEffectSource", LevelLoader::Lua_GetStatusEffectSource);
    lua_register(L, "GetEffectDuration", LevelLoader::Lua_GetEffectDuration);

    // Skill database
    lua_register(L, "GetSkillByID", LevelLoader::Lua_GetSkillByID);
    lua_register(L, "GetClassSkills", LevelLoader::Lua_GetClassSkills);
    lua_register(L, "GetSkillCount", LevelLoader::Lua_GetSkillCount);

    // Particle emitters
    lua_register(L, "SpawnParticleEmitter", LevelLoader::Lua_SpawnParticleEmitter);
    lua_register(L, "SpawnParticleEmitterEthan", LevelLoader::Lua_SpawnParticleEmitterEthan);
    lua_register(L, "SpawnParticleEmitterActive", LevelLoader::Lua_SpawnParticleEmitterActive);
    lua_register(L, "SetUseEthanParticles", LevelLoader::Lua_SetUseEthanParticles);
    lua_register(L, "GetUseEthanParticles", LevelLoader::Lua_GetUseEthanParticles);
    lua_register(L, "ToggleUseEthanParticles", LevelLoader::Lua_ToggleUseEthanParticles);
    lua_register(L, "ClearAllParticleEmitters", LevelLoader::Lua_ClearAllParticleEmitters);
}

} // namespace Framework::GameScriptAPIProvider
