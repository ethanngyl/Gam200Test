/*
===============================================================================
File:        PrefabSerializer.cpp
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-11-07
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
Implements PrefabSerializer, the system responsible for saving and loading
entities (prefabs) from JSON files. Each prefab stores its components�
properties (Transform, Sprite, MeshRenderer, Collider, etc.) so they can be
recreated at runtime or reused across scenes.

Details:
- SavePrefab() writes all active components of an entity into a JSON file,
  including position, rotation, scale, and rendering data.
- LoadPrefab() reads the JSON file, creates a new entity, and re-adds
  components with their saved properties.
- Supports Transform, Sprite, MeshRenderer, Movement, BoxCollider,
  CircleCollider, and SpriteAnimation components.
- Integrates with PrefabInstanceTracker to register loaded prefab instances
  for real-time updates or editor synchronization.

Notes:
- Uses nlohmann::json for serialization and deserialization.
- Ensures floating-point values are written with fixed precision for clean output.
- Provides simple, human-readable JSON formatting compatible with the engine�s
  editor tools.

 Copyright (C) 2025 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#include "PrefabSerializer.h"
#include <fstream>
#include <iomanip>   // std::setprecision
#include <sstream>
#include <Component.h>
#include <RenderComponents.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include "PrefabEditor/PrefabTracker.h"
#include "PrefabEditor/PrefabInstanceRegistry.h"

using namespace Framework;

// tiny helper to print float nicely
// We serialize floats with a fixed number of decimal places so the prefab
// files are more consistent and readable (and avoid weird scientific notation).
static std::string f2(float v) {

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << v;
    return oss.str();
}

namespace PrefabSerializer
{
    bool SavePrefab(Framework::EntityManager& em, Framework::Entity e, const std::string& outPath)
    {
        // If the entity is invalid, there is nothing to save.
        if (!e.IsValid()) return false;

        // Open output file for writing the JSON content.
        std::ofstream out(outPath);
        if (!out.is_open()) return false;

        // ---------------------------------------------------------------------
        // Write JSON header and "entity" id
        // ---------------------------------------------------------------------
        out << "{\n";
        out << "  \"entity\": " << e.GetID() << ",\n";
        out << "  \"components\": {\n";

        // Helper to insert commas between component blocks.
        bool first = true;
        auto writeComma = [&](void) {
            if (!first) out << ",\n";
            first = false;
            };

        // ---------------------------------------------------------------------
        // Transform
        // ---------------------------------------------------------------------
        //
        // We record position, rotation and scale in a compact JSON object.
        if (em.HasComponent<Transform>(e)) {
            auto& t = em.GetComponent<Transform>(e);
            writeComma();
            out << "    \"Transform\": {"
                << "\"position\":[" << f2(t.position.x) << "," << f2(t.position.y) << "],"
                << "\"rotation\":" << f2(t.rotation) << ","
                << "\"scale\":[" << f2(t.scale.x) << "," << f2(t.scale.y) << "]"
                << "}";
        }

        // ---------------------------------------------------------------------
        // Sprite
        // ---------------------------------------------------------------------
        //
        // Simple visual component: just store the texture path and draw layer.
        if (em.HasComponent<Sprite>(e)) {
            auto& s = em.GetComponent<Sprite>(e);
            writeComma();
            out << "    \"Sprite\": {"
                << "\"texturePath\":\"" << s.texturePath << "\","
                << "\"layer\":" << s.layer
                << "}";
        }

        // ---------------------------------------------------------------------
        // MeshRenderer
        // ---------------------------------------------------------------------
        //
        // Mesh-based rendering: store spriteName, layer, order in layer,
        // and tint color (RGBA).
        if (em.HasComponent<MeshRenderer>(e)) {
            auto& mr = em.GetComponent<MeshRenderer>(e);
            writeComma();
            out << "    \"MeshRenderer\": {"
                << "\"spriteName\":\"" << mr.spriteName << "\","
                << "\"layer\":" << mr.layer << ","
                << "\"orderInLayer\":" << mr.orderInLayer << ","
                << "\"tint\":[" << f2(mr.tint.r) << "," << f2(mr.tint.g) << "," << f2(mr.tint.b) << "," << f2(mr.tint.a) << "]"
                << "}";
        }

        // ---------------------------------------------------------------------
        // Movement
        // ---------------------------------------------------------------------
        //
        // Store move speed and direction vector so behavior can be restored.

        if (em.HasComponent<Movement>(e)) {
            auto& m = em.GetComponent<Movement>(e);
            writeComma();
            out << "    \"Movement\": {"
                << "\"speed\":" << f2(m.moveSpeed) << ","
                << "\"direction\":[" << f2(m.direction.x) << "," << f2(m.direction.y) << "]"
                << "}";
        }

        // ---------------------------------------------------------------------
        // BoxCollider
        // ---------------------------------------------------------------------
        //
        // 2D box collision: size, offset from the transform and trigger flag.
        if (em.HasComponent<BoxCollider>(e)) {
            auto& c = em.GetComponent<BoxCollider>(e);
            writeComma();
            out << "    \"BoxCollider\": {"
                << "\"size\":[" << f2(c.size.x) << "," << f2(c.size.y) << "],"
                << "\"offset\":[" << f2(c.offset.x) << "," << f2(c.offset.y) << "],"
                << "\"isTrigger\":" << (c.isTrigger ? "true" : "false")
                << "}";
        }

        // ---------------------------------------------------------------------
        // CircleCollider
        // ---------------------------------------------------------------------
        //
        // Circle collision: radius and offset from the transform.
        if (em.HasComponent<CircleCollider>(e)) {
            auto& c = em.GetComponent<CircleCollider>(e);
            writeComma();
            out << "    \"CircleCollider\": {"
                << "\"radius\":" << f2(c.radius) << ","
                << "\"offset\":[" << f2(c.offset.x) << "," << f2(c.offset.y) << "]"
                << "}";
        }

        // ---------------------------------------------------------------------
        // SpriteAnimation
        // ---------------------------------------------------------------------
        //
        // If sprite animation is used, persist basic animation setup:
        // sheet ID, frame dimensions/count, current frame and flags.
        if (em.HasComponent<SpriteAnimation>(e)) {
            auto& a = em.GetComponent<SpriteAnimation>(e);
            writeComma();
            out << "    \"SpriteAnimation\": {"
                << "\"spriteSheet\":" << a.spriteSheet.GetID() << ","
                << "\"frameWidth\":" << a.frameWidth << ","
                << "\"frameHeight\":" << a.frameHeight << ","
                << "\"frameCount\":" << a.frameCount << ","
                << "\"currentFrame\":" << a.currentFrame << ","
                << "\"uvShrinkPx\":" << a.uvShrinkPx << ","
                << "\"flipX\":" << (a.flipX ? "true" : "false")
                << "}";
        }

        // ---------------------------------------------------------------------
        // AudioSource
        // ---------------------------------------------------------------------
        //
        // Per-entity audio configuration: sound name, volume, pitch, loop flag,
        // and playOnStart flag for automatic playback.
        if (em.HasComponent<AudioSource>(e)) {
            auto& audio = em.GetComponent<AudioSource>(e);
            writeComma();
            out << "    \"AudioSource\": {"
                << "\"soundName\":\"" << audio.soundName << "\","
                << "\"volume\":" << f2(audio.volume) << ","
                << "\"pitch\":" << f2(audio.pitch) << ","
                << "\"loop\":" << (audio.loop ? "true" : "false") << ","
                << "\"playOnStart\":" << (audio.playOnStart ? "true" : "false")
                << "}";
        }

        // Close the "components" object and the root JSON object.
        out << "\n  }\n";
        out << "}\n";
        out.close();
        return true;
    }

    // Reads a prefab and spawns a new entity with its components
    Framework::Entity LoadPrefab(Framework::EntityManager& em, const std::string& path)
    {
        // ---------------------------------------------------------------------
        // STEP 1: Open and parse the JSON file
        // ---------------------------------------------------------------------
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "[PrefabLoader] Failed to open prefab: " << path << "\n";
            return {};
        }

        nlohmann::json data;
        file >> data;
        file.close();

        // ---------------------------------------------------------------------
        // STEP 2: Create a fresh entity in the ECS
        // ---------------------------------------------------------------------
        Entity e = em.CreateEntity();

        // ---------------------------------------------------------------------
        // STEP 3: Recreate components from the "components" block
        // ---------------------------------------------------------------------
        const auto& comps = data["components"];

        // Transform
        if (comps.contains("Transform"))
        {
            const auto& t = comps["Transform"];
            em.AddComponent<Transform>(e);
            auto& c = em.GetComponent<Transform>(e);
            c.position = { t["position"][0], t["position"][1] };
            c.scale = { t["scale"][0], t["scale"][1] };
            if (t.contains("rotation"))
                c.rotation = t["rotation"];
        }

        // Sprite
        if (comps.contains("Sprite"))
        {
            const auto& s = comps["Sprite"];
            em.AddComponent<Sprite>(e);
            auto& c = em.GetComponent<Sprite>(e);
            c.texturePath = s.value("texturePath", "");
            c.layer = s.value("layer", 0);
        }

        // MeshRenderer
        if (comps.contains("MeshRenderer"))
        {
            const auto& m = comps["MeshRenderer"];
            em.AddComponent<MeshRenderer>(e);
            auto& c = em.GetComponent<MeshRenderer>(e);
            c.spriteName = m.value("spriteName", "");
            c.layer = m.value("layer", 0);
            c.orderInLayer = m.value("orderInLayer", 0);
            if (m.contains("tint"))
            {
                c.tint.r = m["tint"][0];
                c.tint.g = m["tint"][1];
                c.tint.b = m["tint"][2];
                c.tint.a = m["tint"][3];
            }
        }

        // Movement
        if (comps.contains("Movement"))
        {
            const auto& m = comps["Movement"];
            em.AddComponent<Movement>(e);
            auto& c = em.GetComponent<Movement>(e);
            c.moveSpeed = m.value("speed", 0.0f);
            c.direction.x = m["direction"][0];
            c.direction.y = m["direction"][1];
        }

        // BoxCollider
        if (comps.contains("BoxCollider"))
        {
            const auto& b = comps["BoxCollider"];
            em.AddComponent<BoxCollider>(e);
            auto& c = em.GetComponent<BoxCollider>(e);
            c.size.x = b["size"][0];
            c.size.y = b["size"][1];
            c.offset.x = b["offset"][0];
            c.offset.y = b["offset"][1];
            c.isTrigger = b.value("isTrigger", false);
        }

        // CircleCollider
        if (comps.contains("CircleCollider"))
        {
            const auto& cdata = comps["CircleCollider"];
            em.AddComponent<CircleCollider>(e);
            auto& c = em.GetComponent<CircleCollider>(e);
            c.radius = cdata["radius"];
            c.offset.x = cdata["offset"][0];
            c.offset.y = cdata["offset"][1];
        }

        // AudioSource
        if (comps.contains("AudioSource"))
        {
            const auto& audioData = comps["AudioSource"];
            em.AddComponent<AudioSource>(e);
            auto& audio = em.GetComponent<AudioSource>(e);
            audio.soundName = audioData.value("soundName", "");
            audio.volume = audioData.value("volume", 1.0f);
            audio.pitch = audioData.value("pitch", 1.0f);
            audio.loop = audioData.value("loop", false);
            audio.playOnStart = audioData.value("playOnStart", false);
            audio.isPlaying = false;  // Always start as not playing
            audio.fmodChannel = nullptr;  // Always initialize to nullptr
        }

        // ---------------------------------------------------------------------
        // STEP 4: Register this entity as a prefab instance so tools
        //         (like the prefab editor) can track and update it.
        // ---------------------------------------------------------------------
        Framework::PrefabInstanceTracker::Get().RegisterInstance(e, path);

        return e;
    }

    bool ApplyPrefabToEntity(Framework::EntityManager& em, 
                             Framework::Entity entity, 
                             const std::string& prefabPath,
                             bool preservePosition)
    {
        if (!entity.IsValid()) {
            std::cerr << "[PrefabSerializer] ApplyPrefabToEntity: Invalid entity\n";
            return false;
        }

        // Open and parse the prefab file
        std::ifstream file(prefabPath);
        if (!file.is_open()) {
            std::cerr << "[PrefabSerializer] ApplyPrefabToEntity: Failed to open prefab: " << prefabPath << "\n";
            return false;
        }

        nlohmann::json data;
        try {
            file >> data;
        }
        catch (const nlohmann::json::exception& e) {
            std::cerr << "[PrefabSerializer] ApplyPrefabToEntity: JSON parse error: " << e.what() << "\n";
            return false;
        }
        file.close();

        if (!data.contains("components")) {
            std::cerr << "[PrefabSerializer] ApplyPrefabToEntity: Prefab missing components\n";
            return false;
        }

        const auto& comps = data["components"];

        // Save current position if we want to preserve it
        Vector2D savedPosition(0.0f, 0.0f);
        if (preservePosition && em.HasComponent<Transform>(entity)) {
            savedPosition = em.GetComponent<Transform>(entity).position;
        }

        // Apply Transform (but preserve position if requested)
        if (comps.contains("Transform")) {
            const auto& t = comps["Transform"];
            if (!em.HasComponent<Transform>(entity)) {
                em.AddComponent<Transform>(entity);
            }
            auto& c = em.GetComponent<Transform>(entity);
            
            if (preservePosition) {
                c.position = savedPosition;  // Keep original position
            } else {
                c.position = { t["position"][0], t["position"][1] };
            }
            c.scale = { t["scale"][0], t["scale"][1] };
            if (t.contains("rotation"))
                c.rotation = t["rotation"];
        }

        // Apply Sprite
        if (comps.contains("Sprite")) {
            const auto& s = comps["Sprite"];
            if (!em.HasComponent<Sprite>(entity)) {
                em.AddComponent<Sprite>(entity);
            }
            auto& c = em.GetComponent<Sprite>(entity);
            c.texturePath = s.value("texturePath", "");
            c.layer = s.value("layer", 0);
        }

        // Apply MeshRenderer
        if (comps.contains("MeshRenderer")) {
            const auto& m = comps["MeshRenderer"];
            if (!em.HasComponent<MeshRenderer>(entity)) {
                em.AddComponent<MeshRenderer>(entity);
            }
            auto& c = em.GetComponent<MeshRenderer>(entity);
            c.spriteName = m.value("spriteName", "");
            c.layer = m.value("layer", 0);
            c.orderInLayer = m.value("orderInLayer", 0);
            if (m.contains("tint")) {
                c.tint.r = m["tint"][0];
                c.tint.g = m["tint"][1];
                c.tint.b = m["tint"][2];
                c.tint.a = m["tint"][3];
            }
        }

        // Apply Movement
        if (comps.contains("Movement")) {
            const auto& m = comps["Movement"];
            if (!em.HasComponent<Movement>(entity)) {
                em.AddComponent<Movement>(entity);
            }
            auto& c = em.GetComponent<Movement>(entity);
            c.moveSpeed = m.value("speed", 0.0f);
            c.direction.x = m["direction"][0];
            c.direction.y = m["direction"][1];
        }

        // Apply BoxCollider
        if (comps.contains("BoxCollider")) {
            const auto& b = comps["BoxCollider"];
            if (!em.HasComponent<BoxCollider>(entity)) {
                em.AddComponent<BoxCollider>(entity);
            }
            auto& c = em.GetComponent<BoxCollider>(entity);
            c.size.x = b["size"][0];
            c.size.y = b["size"][1];
            c.offset.x = b["offset"][0];
            c.offset.y = b["offset"][1];
            c.isTrigger = b.value("isTrigger", false);
        }

        // Apply CircleCollider
        if (comps.contains("CircleCollider")) {
            const auto& cdata = comps["CircleCollider"];
            if (!em.HasComponent<CircleCollider>(entity)) {
                em.AddComponent<CircleCollider>(entity);
            }
            auto& c = em.GetComponent<CircleCollider>(entity);
            c.radius = cdata["radius"];
            c.offset.x = cdata["offset"][0];
            c.offset.y = cdata["offset"][1];
        }

        // Apply AudioSource
        if (comps.contains("AudioSource")) {
            const auto& audioData = comps["AudioSource"];
            if (!em.HasComponent<AudioSource>(entity)) {
                em.AddComponent<AudioSource>(entity);
            }
            auto& audio = em.GetComponent<AudioSource>(entity);
            audio.soundName = audioData.value("soundName", "");
            audio.volume = audioData.value("volume", 1.0f);
            audio.pitch = audioData.value("pitch", 1.0f);
            audio.loop = audioData.value("loop", false);
            audio.playOnStart = audioData.value("playOnStart", false);
        }

        std::cout << "[PrefabSerializer] Applied prefab '" << prefabPath 
                  << "' to entity " << entity.GetID() << "\n";

        return true;
    }

    bool RevertToPrefab(Framework::EntityManager& em, 
                        Framework::Entity entity, 
                        const std::string& prefabPath)
    {
        return ApplyPrefabToEntity(em, entity, prefabPath, true);
    }
}

