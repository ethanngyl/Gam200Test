/**
===============================================================================
File:        SaveLoadSystem.h
Author:      GE YONGQI
Date:        2026-01-14
-------------------------------------------------------------------------------
Universal Save/Load System for entity serialization to JSON format.

Provides functionality to:
- Save ALL entities and their components to a JSON file
- Load entities from a JSON file (restores full scene state)
- Works with any level type: MainMenu, LevelSelect, Level2, Level3, etc.
- Auto-save/auto-load support for seamless session persistence

Supported Components:
- Transform, Sprite, Movement, BoxCollider, CircleCollider
- SpriteAnimation, AudioSource, ScriptComponent
- Health, AP, AttackAP, Chest, Goal, Inventory
- TagComponent, AttackRangeComponent, MeshRenderer
- GridTiles, EnemyAI, Renderable

JSON Format Example:
{
  "version": "1.0",
  "levelName": "Level3",
  "timestamp": "2026-01-14T12:00:00",
  "entities": [
    {
      "id": 1,
      "components": {
        "Transform": { "posX": 0.0, "posY": 0.0, "scaleX": 1.0, "scaleY": 1.0, "rotation": 0.0 },
        "Sprite": { "texturePath": "assets/player.png", "layer": 0, "tint": [1,1,1,1] },
        ...
      }
    }
  ]
}

Usage:
    // Save current scene
    SaveLoadSystem::SaveToJSON("assets/saves/level3_save.json", entityManager, "Level3");
    
    // Load scene
    SaveLoadSystem::LoadFromJSON("assets/saves/level3_save.json", entityManager);
    
    // Auto-save (uses default path)
    SaveLoadSystem::AutoSave(entityManager, "Level3");
===============================================================================
*/

#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "ECSEntityManager.h"
#include "Component.h"

namespace Framework {

    // Forward declarations
    class GraphicsSystemV2;

    /**
     * @class SaveLoadSystem
     * @brief Handles serialization and deserialization of entities to/from JSON
     */
    class SaveLoadSystem {
    public:
        /**
         * @brief Saves all entities to a JSON file
         * @param filepath Path to the output JSON file
         * @param entityManager Pointer to the EntityManager
         * @param levelName Name of the current level (for metadata)
         * @return true if save was successful, false otherwise
         */
        static bool SaveToJSON(const std::string& filepath, 
                               EntityManager* entityManager,
                               const std::string& levelName = "Unknown");

        /**
         * @brief Loads entities from a JSON file
         * @param filepath Path to the input JSON file
         * @param entityManager Pointer to the EntityManager
         * @param clearExisting If true, clears all existing entities before loading
         * @return true if load was successful, false otherwise
         */
        static bool LoadFromJSON(const std::string& filepath, 
                                 EntityManager* entityManager, 
                                 bool clearExisting = true);

        /**
         * @brief Gets the auto-save file path for a specific level
         * @param levelName Name of the level
         * @return Path to the auto-save file
         */
        static std::string GetAutoSavePath(const std::string& levelName);

        /**
         * @brief Checks if an auto-save file exists for a level
         * @param levelName Name of the level
         * @return true if auto-save file exists
         */
        static bool HasAutoSave(const std::string& levelName);

        /**
         * @brief Performs an auto-save for the current level
         * @param entityManager Pointer to the EntityManager
         * @param levelName Name of the current level
         * @return true if successful
         */
        static bool AutoSave(EntityManager* entityManager, const std::string& levelName);

        /**
         * @brief Loads from auto-save file for a specific level
         * @param entityManager Pointer to the EntityManager
         * @param levelName Name of the level
         * @return true if successful
         */
        static bool LoadAutoSave(EntityManager* entityManager, const std::string& levelName);

        /**
         * @brief Deletes the auto-save file for a level
         * @param levelName Name of the level
         * @return true if file was deleted or didn't exist
         */
        static bool ClearAutoSave(const std::string& levelName);

        /**
         * @brief Gets the level name from a save file
         * @param filepath Path to the save file
         * @return Level name or empty string if failed
         */
        static std::string GetLevelNameFromSave(const std::string& filepath);

        /**
         * @brief Sets the graphics system for texture loading during deserialization
         * @param graphics Pointer to GraphicsSystemV2
         */
        static void SetGraphicsSystem(GraphicsSystemV2* graphics);

    private:
        static GraphicsSystemV2* s_graphicsSystem;

        // Serialization helpers - Entity level
        static nlohmann::json SerializeEntity(Entity entity, EntityManager* entityManager);
        
        // Serialization helpers - Component level
        static nlohmann::json SerializeTransform(const Transform& transform);
        static nlohmann::json SerializeSprite(const Sprite& sprite);
        static nlohmann::json SerializeMovement(const Movement& movement);
        static nlohmann::json SerializeBoxCollider(const BoxCollider& collider);
        static nlohmann::json SerializeCircleCollider(const CircleCollider& collider);
        static nlohmann::json SerializeSpriteAnimation(const SpriteAnimation& anim);
        static nlohmann::json SerializeAudioSource(const AudioSource& audio);
        static nlohmann::json SerializeScriptComponent(const ScriptComponent& script);
        static nlohmann::json SerializeHealth(const Health& health);
        static nlohmann::json SerializeAP(const AP& ap);
        static nlohmann::json SerializeAttackAP(const AttackAP& attackAP);
        static nlohmann::json SerializeChest(const Chest& chest);
        static nlohmann::json SerializeGoal(const Goal& goal);
        static nlohmann::json SerializeInventory(const Inventory& inventory);
        static nlohmann::json SerializeAttackRangeComponent(const AttackRangeComponent& range);
        static nlohmann::json SerializeMeshRenderer(EntityManager* em, Entity entity);
        static nlohmann::json SerializeGridTiles(EntityManager* em, Entity entity);

        // Deserialization helpers
        static Entity DeserializeEntity(const nlohmann::json& json, EntityManager* entityManager);
        static void DeserializeTransform(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeSprite(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeMovement(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeBoxCollider(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeCircleCollider(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeSpriteAnimation(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeAudioSource(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeScriptComponent(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeHealth(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeAP(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeAttackAP(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeChest(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeGoal(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeInventory(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeAttackRangeComponent(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeMeshRenderer(const nlohmann::json& json, Entity entity, EntityManager* entityManager);
        static void DeserializeGridTiles(const nlohmann::json& json, Entity entity, EntityManager* entityManager);

        // Constants
        static const std::string SAVE_VERSION;
        static const std::string AUTO_SAVE_DIR;
    };

} // namespace Framework
