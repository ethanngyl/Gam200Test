/**
===============================================================================
 File:          SaveLoadSystem.cpp
 Author:        Ge Yongqi
 Email:
 Date:          2026-01-14
 Contribution:  100%
 ------------------------------------------------------------------------------

 SAVE/LOAD SYSTEM - Implementation

 Brief:
    Implementation of the universal Save/Load System for entity serialization.
    Handles JSON file I/O, entity serialization/deserialization, and auto-save
    management. Serializes all supported ECS components to a structured JSON
    format and reconstructs entities with their full component state on load.


 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#include "Precompiled.h"
#include "SaveLoadSystem.h"
#include "RenderComponents.h"
#include "Grid/GridTile.h"
#include "Pathfinding.h"
#include "GraphicsSystemV2.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace Framework {

    // Static member initialization
    const std::string SaveLoadSystem::SAVE_VERSION = "1.0";
    const std::string SaveLoadSystem::AUTO_SAVE_DIR = "assets/saves/";
    GraphicsSystemV2* SaveLoadSystem::s_graphicsSystem = nullptr;

    void SaveLoadSystem::SetGraphicsSystem(GraphicsSystemV2* graphics) {
        s_graphicsSystem = graphics;
    }

    // ============================================================================
    // UTILITY: Get current timestamp as string
    // ============================================================================
    static std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &time);
#else
        localtime_r(&time, &tm_buf);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
        return oss.str();
    }

    // ============================================================================
    // SAVE TO JSON
    // ============================================================================
    bool SaveLoadSystem::SaveToJSON(const std::string& filepath, 
                                     EntityManager* entityManager,
                                     const std::string& levelName) {
        if (!entityManager) {
            LOG_ERROR("SaveLoadSystem", "Cannot save: EntityManager is null");
            return false;
        }

        // Ensure directory exists
        std::filesystem::path path(filepath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        // Build JSON structure
        nlohmann::json root;
        root["version"] = SAVE_VERSION;
        root["levelName"] = levelName;
        root["timestamp"] = GetTimestamp();
        root["entities"] = nlohmann::json::array();

        // Serialize all entities
        auto allEntities = entityManager->GetAllEntities();
        LOG_INFO("SaveLoadSystem", "Saving %zu entities to %s", allEntities.size(), filepath.c_str());

        for (const auto& entity : allEntities) {
            nlohmann::json entityJson = SerializeEntity(entity, entityManager);
            if (!entityJson.empty()) {
                root["entities"].push_back(entityJson);
            }
        }

        // Write to file with pretty printing
        std::ofstream file(filepath);
        if (!file.is_open()) {
            LOG_ERROR("SaveLoadSystem", "Failed to open file for writing: %s", filepath.c_str());
            return false;
        }

        file << root.dump(2);  // 2-space indentation
        file.close();

        LOG_INFO("SaveLoadSystem", "Successfully saved %zu entities to %s", 
                 root["entities"].size(), filepath.c_str());
        return true;
    }

    // ============================================================================
    // LOAD FROM JSON
    // ============================================================================
    bool SaveLoadSystem::LoadFromJSON(const std::string& filepath, 
                                       EntityManager* entityManager, 
                                       bool clearExisting) {
        if (!entityManager) {
            LOG_ERROR("SaveLoadSystem", "Cannot load: EntityManager is null");
            return false;
        }

        std::ifstream file(filepath);
        if (!file.is_open()) {
            LOG_ERROR("SaveLoadSystem", "Failed to open file for reading: %s", filepath.c_str());
            return false;
        }

        nlohmann::json root;
        try {
            file >> root;
        }
        catch (const nlohmann::json::parse_error& e) {
            LOG_ERROR("SaveLoadSystem", "JSON parse error: %s", e.what());
            return false;
        }

        // Validate version
        if (root.contains("version")) {
            std::string version = root["version"].get<std::string>();
            LOG_INFO("SaveLoadSystem", "Loading save file version: %s", version.c_str());
        }

        // Clear existing entities if requested
        if (clearExisting) {
            entityManager->ClearAllEntities();
            entityManager->ResetEntityIDCounter();
        }

        // Load entities
        if (!root.contains("entities") || !root["entities"].is_array()) {
            LOG_ERROR("SaveLoadSystem", "Invalid save file: missing 'entities' array");
            return false;
        }

        int loadedCount = 0;
        for (const auto& entityJson : root["entities"]) {
            Entity entity = DeserializeEntity(entityJson, entityManager);
            if (entity.IsValid()) {
                loadedCount++;
            }
        }

        LOG_INFO("SaveLoadSystem", "Successfully loaded %d entities from %s", loadedCount, filepath.c_str());
        return true;
    }

    // ============================================================================
    // AUTO-SAVE FUNCTIONS
    // ============================================================================
    std::string SaveLoadSystem::GetAutoSavePath(const std::string& levelName) {
        return AUTO_SAVE_DIR + levelName + "_autosave.json";
    }

    bool SaveLoadSystem::HasAutoSave(const std::string& levelName) {
        return std::filesystem::exists(GetAutoSavePath(levelName));
    }

    bool SaveLoadSystem::AutoSave(EntityManager* entityManager, const std::string& levelName) {
        std::filesystem::create_directories(AUTO_SAVE_DIR);
        return SaveToJSON(GetAutoSavePath(levelName), entityManager, levelName);
    }

    bool SaveLoadSystem::LoadAutoSave(EntityManager* entityManager, const std::string& levelName) {
        std::string path = GetAutoSavePath(levelName);
        if (!std::filesystem::exists(path)) {
            LOG_WARN("SaveLoadSystem", "No auto-save found for level: %s", levelName.c_str());
            return false;
        }
        return LoadFromJSON(path, entityManager, true);
    }

    bool SaveLoadSystem::ClearAutoSave(const std::string& levelName) {
        std::string path = GetAutoSavePath(levelName);
        if (std::filesystem::exists(path)) {
            return std::filesystem::remove(path);
        }
        return true;
    }

    std::string SaveLoadSystem::GetLevelNameFromSave(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return "";

        try {
            nlohmann::json root;
            file >> root;
            if (root.contains("levelName")) {
                return root["levelName"].get<std::string>();
            }
        }
        catch (...) {}
        return "";
    }

    // ============================================================================
    // ENTITY SERIALIZATION
    // ============================================================================
    nlohmann::json SaveLoadSystem::SerializeEntity(Entity entity, EntityManager* em) {
        nlohmann::json entityJson;
        entityJson["id"] = entity.GetID();
        entityJson["components"] = nlohmann::json::object();

        // Transform
        if (em->HasComponent<Transform>(entity)) {
            entityJson["components"]["Transform"] = SerializeTransform(em->GetComponent<Transform>(entity));
        }

        // Sprite
        if (em->HasComponent<Sprite>(entity)) {
            entityJson["components"]["Sprite"] = SerializeSprite(em->GetComponent<Sprite>(entity));
        }

        // Movement
        if (em->HasComponent<Movement>(entity)) {
            entityJson["components"]["Movement"] = SerializeMovement(em->GetComponent<Movement>(entity));
        }

        // BoxCollider
        if (em->HasComponent<BoxCollider>(entity)) {
            entityJson["components"]["BoxCollider"] = SerializeBoxCollider(em->GetComponent<BoxCollider>(entity));
        }

        // CircleCollider
        if (em->HasComponent<CircleCollider>(entity)) {
            entityJson["components"]["CircleCollider"] = SerializeCircleCollider(em->GetComponent<CircleCollider>(entity));
        }

        // SpriteAnimation
        if (em->HasComponent<SpriteAnimation>(entity)) {
            entityJson["components"]["SpriteAnimation"] = SerializeSpriteAnimation(em->GetComponent<SpriteAnimation>(entity));
        }

        // AudioSource
        if (em->HasComponent<AudioSource>(entity)) {
            entityJson["components"]["AudioSource"] = SerializeAudioSource(em->GetComponent<AudioSource>(entity));
        }

        // ScriptComponent
        if (em->HasComponent<ScriptComponent>(entity)) {
            entityJson["components"]["ScriptComponent"] = SerializeScriptComponent(em->GetComponent<ScriptComponent>(entity));
        }

        // Health
        if (em->HasComponent<Health>(entity)) {
            entityJson["components"]["Health"] = SerializeHealth(em->GetComponent<Health>(entity));
        }

        // AP
        if (em->HasComponent<AP>(entity)) {
            entityJson["components"]["AP"] = SerializeAP(em->GetComponent<AP>(entity));
        }

        // AttackAP
        if (em->HasComponent<AttackAP>(entity)) {
            entityJson["components"]["AttackAP"] = SerializeAttackAP(em->GetComponent<AttackAP>(entity));
        }

        // Chest
        if (em->HasComponent<Chest>(entity)) {
            entityJson["components"]["Chest"] = SerializeChest(em->GetComponent<Chest>(entity));
        }

        // Goal
        if (em->HasComponent<Goal>(entity)) {
            entityJson["components"]["Goal"] = SerializeGoal(em->GetComponent<Goal>(entity));
        }

        // Inventory
        if (em->HasComponent<Inventory>(entity)) {
            entityJson["components"]["Inventory"] = SerializeInventory(em->GetComponent<Inventory>(entity));
        }

        // AttackRangeComponent
        if (em->HasComponent<AttackRangeComponent>(entity)) {
            entityJson["components"]["AttackRangeComponent"] = SerializeAttackRangeComponent(em->GetComponent<AttackRangeComponent>(entity));
        }

        // MeshRenderer (Renderable)
        if (em->HasComponent<MeshRenderer>(entity)) {
            entityJson["components"]["MeshRenderer"] = SerializeMeshRenderer(em, entity);
        }

        // GridTiles
        if (em->HasComponent<GridTiles>(entity)) {
            entityJson["components"]["GridTiles"] = SerializeGridTiles(em, entity);
        }

        return entityJson;
    }

    // ============================================================================
    // COMPONENT SERIALIZATION
    // ============================================================================
    nlohmann::json SaveLoadSystem::SerializeTransform(const Transform& t) {
        return {
            {"posX", t.position.x},
            {"posY", t.position.y},
            {"scaleX", t.scale.x},
            {"scaleY", t.scale.y},
            {"rotation", t.rotation}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeSprite(const Sprite& s) {
        return {
            {"texturePath", s.texturePath},
            {"layer", s.layer},
            {"flipX", s.flipX},
            {"flipY", s.flipY},
            {"tint", {s.tint.r, s.tint.g, s.tint.b, s.tint.a}}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeMovement(const Movement& m) {
        return {
            {"moveSpeed", m.moveSpeed},
            {"directionX", m.direction.x},
            {"directionY", m.direction.y},
            {"blocked", m.blocked}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeBoxCollider(const BoxCollider& bc) {
        return {
            {"sizeX", bc.size.x},
            {"sizeY", bc.size.y},
            {"offsetX", bc.offset.x},
            {"offsetY", bc.offset.y},
            {"isTrigger", bc.isTrigger}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeCircleCollider(const CircleCollider& cc) {
        return {
            {"radius", cc.radius},
            {"offsetX", cc.offset.x},
            {"offsetY", cc.offset.y}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeSpriteAnimation(const SpriteAnimation& sa) {
        return {
            {"animName", sa.animName},
            {"startFrame", sa.startFrame},
            {"frameCount", sa.frameCount},
            {"rows", sa.rows},
            {"columns", sa.columns},
            {"frameTime", sa.frameTime},
            {"frameWidth", sa.frameWidth},
            {"frameHeight", sa.frameHeight},
            {"uvShrinkPx", sa.uvShrinkPx},
            {"loop", sa.loop},
            {"playing", sa.playing},
            {"flipX", sa.flipX},
            {"currentFrame", sa.currentFrame},
            {"useJsonConfig", sa.useJsonConfig}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeAudioSource(const AudioSource& as) {
        return {
            {"soundName", as.soundName},
            {"volume", as.volume},
            {"pitch", as.pitch},
            {"loop", as.loop},
            {"playOnStart", as.playOnStart}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeScriptComponent(const ScriptComponent& sc) {
        return {
            {"scriptPath", sc.scriptPath}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeHealth(const Health& h) {
        return {
            {"maxHealth", h.maxHealth},
            {"currentHealth", h.currentHealth},
            {"isDead", h.isDead}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeAP(const AP& ap) {
        return {
            {"actionPoints", ap.actionPoints},
            {"maxActionPoints", ap.maxActionPoints}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeAttackAP(const AttackAP& aap) {
        return {
            {"points", aap.points},
            {"maxPoints", aap.maxPoints}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeChest(const Chest& c) {
        return {
            {"collected", c.collected},
            {"chestID", c.chestID}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeGoal(const Goal& g) {
        return {
            {"chestsRequired", g.chestsRequired},
            {"canExit", g.canExit}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeInventory(const Inventory& inv) {
        return {
            {"collectedChests", inv.collectedChests}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeAttackRangeComponent(const AttackRangeComponent& arc) {
        return {
            {"minRange", arc.minRange},
            {"maxRange", arc.maxRange},
            {"showRange", arc.showRange}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeMeshRenderer(EntityManager* em, Entity entity) {
        auto& mr = em->GetComponent<MeshRenderer>(entity);
        return {
            {"spriteName", mr.spriteName},
            {"layer", mr.layer},
            {"orderInLayer", mr.orderInLayer},
            {"visible", mr.visible},
            {"tint", {mr.tint.r, mr.tint.g, mr.tint.b, mr.tint.a}}
        };
    }

    nlohmann::json SaveLoadSystem::SerializeGridTiles(EntityManager* em, Entity entity) {
        auto& gt = em->GetComponent<GridTiles>(entity);
        return {
            {"tileId", gt.tileId},
            {"x", gt.x},
            {"y", gt.y},
            {"blocked", gt.blocked},
            {"centerWorldX", gt.centerWorld.x},
            {"centerWorldY", gt.centerWorld.y},
            {"tileW", gt.tileW},
            {"tileH", gt.tileH}
        };
    }

    // ============================================================================
    // ENTITY DESERIALIZATION
    // ============================================================================
    Entity SaveLoadSystem::DeserializeEntity(const nlohmann::json& json, EntityManager* em) {
        Entity entity = em->CreateEntity();

        if (!json.contains("components")) {
            return entity;
        }

        const auto& components = json["components"];

        // Deserialize each component
        if (components.contains("Transform")) {
            DeserializeTransform(components["Transform"], entity, em);
        }
        if (components.contains("Sprite")) {
            DeserializeSprite(components["Sprite"], entity, em);
        }
        if (components.contains("Movement")) {
            DeserializeMovement(components["Movement"], entity, em);
        }
        if (components.contains("BoxCollider")) {
            DeserializeBoxCollider(components["BoxCollider"], entity, em);
        }
        if (components.contains("CircleCollider")) {
            DeserializeCircleCollider(components["CircleCollider"], entity, em);
        }
        if (components.contains("SpriteAnimation")) {
            DeserializeSpriteAnimation(components["SpriteAnimation"], entity, em);
        }
        if (components.contains("AudioSource")) {
            DeserializeAudioSource(components["AudioSource"], entity, em);
        }
        if (components.contains("ScriptComponent")) {
            DeserializeScriptComponent(components["ScriptComponent"], entity, em);
        }
        if (components.contains("Health")) {
            DeserializeHealth(components["Health"], entity, em);
        }
        if (components.contains("AP")) {
            DeserializeAP(components["AP"], entity, em);
        }
        if (components.contains("AttackAP")) {
            DeserializeAttackAP(components["AttackAP"], entity, em);
        }
        if (components.contains("Chest")) {
            DeserializeChest(components["Chest"], entity, em);
        }
        if (components.contains("Goal")) {
            DeserializeGoal(components["Goal"], entity, em);
        }
        if (components.contains("Inventory")) {
            DeserializeInventory(components["Inventory"], entity, em);
        }
        if (components.contains("AttackRangeComponent")) {
            DeserializeAttackRangeComponent(components["AttackRangeComponent"], entity, em);
        }
        if (components.contains("MeshRenderer")) {
            DeserializeMeshRenderer(components["MeshRenderer"], entity, em);
        }
        if (components.contains("GridTiles")) {
            DeserializeGridTiles(components["GridTiles"], entity, em);
        }

        return entity;
    }

    // ============================================================================
    // COMPONENT DESERIALIZATION
    // ============================================================================
    void SaveLoadSystem::DeserializeTransform(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<Transform>(entity);
        auto& t = em->GetComponent<Transform>(entity);
        t.position.x = j.value("posX", 0.0f);
        t.position.y = j.value("posY", 0.0f);
        t.scale.x = j.value("scaleX", 1.0f);
        t.scale.y = j.value("scaleY", 1.0f);
        t.rotation = j.value("rotation", 0.0f);
    }

    void SaveLoadSystem::DeserializeSprite(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<Sprite>(entity);
        auto& s = em->GetComponent<Sprite>(entity);
        s.texturePath = j.value("texturePath", "");
        s.layer = j.value("layer", 0);
        s.flipX = j.value("flipX", false);
        s.flipY = j.value("flipY", false);
        if (j.contains("tint") && j["tint"].is_array() && j["tint"].size() >= 4) {
            s.tint.r = j["tint"][0].get<float>();
            s.tint.g = j["tint"][1].get<float>();
            s.tint.b = j["tint"][2].get<float>();
            s.tint.a = j["tint"][3].get<float>();
        }
    }

    void SaveLoadSystem::DeserializeMovement(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<Movement>(entity);
        auto& m = em->GetComponent<Movement>(entity);
        m.moveSpeed = j.value("moveSpeed", 100.0f);
        m.direction.x = j.value("directionX", 0.0f);
        m.direction.y = j.value("directionY", 0.0f);
        m.blocked = j.value("blocked", false);
    }

    void SaveLoadSystem::DeserializeBoxCollider(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<BoxCollider>(entity);
        auto& bc = em->GetComponent<BoxCollider>(entity);
        bc.size.x = j.value("sizeX", 1.0f);
        bc.size.y = j.value("sizeY", 1.0f);
        bc.offset.x = j.value("offsetX", 0.0f);
        bc.offset.y = j.value("offsetY", 0.0f);
        bc.isTrigger = j.value("isTrigger", false);
    }

    void SaveLoadSystem::DeserializeCircleCollider(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<CircleCollider>(entity);
        auto& cc = em->GetComponent<CircleCollider>(entity);
        cc.radius = j.value("radius", 0.5f);
        cc.offset.x = j.value("offsetX", 0.0f);
        cc.offset.y = j.value("offsetY", 0.0f);
    }

    void SaveLoadSystem::DeserializeSpriteAnimation(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<SpriteAnimation>(entity);
        auto& sa = em->GetComponent<SpriteAnimation>(entity);
        sa.animName = j.value("animName", "");
        sa.startFrame = j.value("startFrame", 0);
        sa.frameCount = j.value("frameCount", 1);
        sa.rows = j.value("rows", 1);
        sa.columns = j.value("columns", 1);
        sa.frameTime = j.value("frameTime", 0.1f);
        sa.frameWidth = j.value("frameWidth", 0);
        sa.frameHeight = j.value("frameHeight", 0);
        sa.uvShrinkPx = j.value("uvShrinkPx", 0.0f);
        sa.loop = j.value("loop", true);
        sa.playing = j.value("playing", true);
        sa.flipX = j.value("flipX", false);
        sa.currentFrame = j.value("currentFrame", 0);
        sa.useJsonConfig = j.value("useJsonConfig", true);

        // Load sprite sheet texture if graphics system is available
        if (s_graphicsSystem && em->HasComponent<Sprite>(entity)) {
            auto& sprite = em->GetComponent<Sprite>(entity);
            if (!sprite.texturePath.empty()) {
                auto& rm = s_graphicsSystem->GetResourceManager();
                sa.spriteSheet = rm.LoadTexture(sprite.texturePath);
                if (auto* texture = rm.GetTexture(sa.spriteSheet)) {
                    int colsForSize = (sa.columns > 0) ? sa.columns : 1;
                    int rowsForSize = (sa.rows > 0) ? sa.rows : 1;
                    sa.frameWidth = texture->GetWidth() / colsForSize;
                    sa.frameHeight = texture->GetHeight() / rowsForSize;
                }
            }
        }
    }

    void SaveLoadSystem::DeserializeAudioSource(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<AudioSource>(entity);
        auto& as = em->GetComponent<AudioSource>(entity);
        as.soundName = j.value("soundName", "");
        as.volume = j.value("volume", 1.0f);
        as.pitch = j.value("pitch", 1.0f);
        as.loop = j.value("loop", false);
        as.playOnStart = j.value("playOnStart", false);
    }

    void SaveLoadSystem::DeserializeScriptComponent(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<ScriptComponent>(entity);
        auto& sc = em->GetComponent<ScriptComponent>(entity);
        sc.scriptPath = j.value("scriptPath", "");
    }

    void SaveLoadSystem::DeserializeHealth(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<Health>(entity);
        auto& h = em->GetComponent<Health>(entity);
        h.maxHealth = j.value("maxHealth", 50);
        h.currentHealth = j.value("currentHealth", 50);
        h.isDead = j.value("isDead", false);
    }

    void SaveLoadSystem::DeserializeAP(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<AP>(entity);
        auto& ap = em->GetComponent<AP>(entity);
        ap.actionPoints = j.value("actionPoints", 3);
        ap.maxActionPoints = j.value("maxActionPoints", 3);
    }

    void SaveLoadSystem::DeserializeAttackAP(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<AttackAP>(entity);
        auto& aap = em->GetComponent<AttackAP>(entity);
        aap.points = j.value("points", 1);
        aap.maxPoints = j.value("maxPoints", 3);
    }

    void SaveLoadSystem::DeserializeChest(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<Chest>(entity);
        auto& c = em->GetComponent<Chest>(entity);
        c.collected = j.value("collected", false);
        c.chestID = j.value("chestID", 0);
    }

    void SaveLoadSystem::DeserializeGoal(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<Goal>(entity);
        auto& g = em->GetComponent<Goal>(entity);
        g.chestsRequired = j.value("chestsRequired", 0);
        g.canExit = j.value("canExit", false);
    }

    void SaveLoadSystem::DeserializeInventory(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<Inventory>(entity);
        auto& inv = em->GetComponent<Inventory>(entity);
        if (j.contains("collectedChests") && j["collectedChests"].is_array()) {
            for (const auto& chestId : j["collectedChests"]) {
                inv.collectedChests.push_back(chestId.get<int>());
            }
        }
    }

    void SaveLoadSystem::DeserializeAttackRangeComponent(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<AttackRangeComponent>(entity);
        auto& arc = em->GetComponent<AttackRangeComponent>(entity);
        arc.minRange = j.value("minRange", 1);
        arc.maxRange = j.value("maxRange", 3);
        arc.showRange = j.value("showRange", false);
    }

    void SaveLoadSystem::DeserializeMeshRenderer(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<MeshRenderer>(entity);
        auto& mr = em->GetComponent<MeshRenderer>(entity);
        mr.spriteName = j.value("spriteName", "");
        mr.layer = j.value("layer", 0);
        mr.orderInLayer = j.value("orderInLayer", 0);
        mr.visible = j.value("visible", true);
        if (j.contains("tint") && j["tint"].is_array() && j["tint"].size() >= 4) {
            mr.tint.r = j["tint"][0].get<float>();
            mr.tint.g = j["tint"][1].get<float>();
            mr.tint.b = j["tint"][2].get<float>();
            mr.tint.a = j["tint"][3].get<float>();
        }
    }

    void SaveLoadSystem::DeserializeGridTiles(const nlohmann::json& j, Entity entity, EntityManager* em) {
        em->AddComponent<GridTiles>(entity);
        auto& gt = em->GetComponent<GridTiles>(entity);
        gt.tileId = j.value("tileId", -1);
        gt.x = j.value("x", 0);
        gt.y = j.value("y", 0);
        gt.blocked = j.value("blocked", false);
        gt.centerWorld.x = j.value("centerWorldX", 0.0f);
        gt.centerWorld.y = j.value("centerWorldY", 0.0f);
        gt.tileW = j.value("tileW", 1.0f);
        gt.tileH = j.value("tileH", 1.0f);
        gt.entity = entity;
    }

} // namespace Framework
