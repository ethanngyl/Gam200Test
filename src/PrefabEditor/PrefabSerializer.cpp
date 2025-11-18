//kahyan

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
static std::string f2(float v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << v;
    return oss.str();
}

namespace PrefabSerializer
{
    bool SavePrefab(Framework::EntityManager& em, Framework::Entity e, const std::string& outPath)
    {
        if (!e.IsValid()) return false;

        std::ofstream out(outPath);
        if (!out.is_open()) return false;

        out << "{\n";
        out << "  \"entity\": " << e.GetID() << ",\n";
        out << "  \"components\": {\n";

        bool first = true;
        auto writeComma = [&](void) {
            if (!first) out << ",\n";
            first = false;
            };

        // Transform
        if (em.HasComponent<Transform>(e)) {
            auto& t = em.GetComponent<Transform>(e);
            writeComma();
            out << "    \"Transform\": {"
                << "\"position\":[" << f2(t.position.x) << "," << f2(t.position.y) << "],"
                << "\"rotation\":" << f2(t.rotation) << ","
                << "\"scale\":[" << f2(t.scale.x) << "," << f2(t.scale.y) << "]"
                << "}";
        }

        // Sprite (simple path/label)
        if (em.HasComponent<Sprite>(e)) {
            auto& s = em.GetComponent<Sprite>(e);
            writeComma();
            out << "    \"Sprite\": {"
                << "\"texturePath\":\"" << s.texturePath << "\","
                << "\"layer\":" << s.layer
                << "}";
        }

        // MeshRenderer (record spriteName + layer + tint if you have it)
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

        // Movement
        if (em.HasComponent<Movement>(e)) {
            auto& m = em.GetComponent<Movement>(e);
            writeComma();
            out << "    \"Movement\": {"
                << "\"speed\":" << f2(m.moveSpeed) << ","
                << "\"direction\":[" << f2(m.direction.x) << "," << f2(m.direction.y) << "]"
                << "}";
        }

        // BoxCollider
        if (em.HasComponent<BoxCollider>(e)) {
            auto& c = em.GetComponent<BoxCollider>(e);
            writeComma();
            out << "    \"BoxCollider\": {"
                << "\"size\":[" << f2(c.size.x) << "," << f2(c.size.y) << "],"
                << "\"offset\":[" << f2(c.offset.x) << "," << f2(c.offset.y) << "],"
                << "\"isTrigger\":" << (c.isTrigger ? "true" : "false")
                << "}";
        }

        // CircleCollider
        if (em.HasComponent<CircleCollider>(e)) {
            auto& c = em.GetComponent<CircleCollider>(e);
            writeComma();
            out << "    \"CircleCollider\": {"
                << "\"radius\":" << f2(c.radius) << ","
                << "\"offset\":[" << f2(c.offset.x) << "," << f2(c.offset.y) << "]"
                << "}";
        }

        // (Optional) SpriteAnimation if you use it
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

        out << "\n  }\n";
        out << "}\n";
        out.close();
        return true;
    }

    // Reads a prefab and spawns a new entity with its components
    Framework::Entity LoadPrefab(Framework::EntityManager& em, const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "[PrefabLoader] Failed to open prefab: " << path << "\n";
            return {};
        }

        nlohmann::json data;
        file >> data;
        file.close();

        // Create a new entity
        Entity e = em.CreateEntity();

        // ---------------------------------------------------------------------
        // Recreate components that exist in the file
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

        Framework::PrefabInstanceTracker::Get().RegisterInstance(e, path);
        //Framework::PrefabInstanceRegistry::Get().RegisterInstance(e, path);

        return e;
    }
}

