#include "Precompiled.h"
#include <nlohmann/json.hpp>

namespace Framework {

    static float RandF(float a, float b)
    {
        return a + (b - a) * (float(rand()) / float(RAND_MAX));
    }
    static float DegToRad(float d) { return d * 3.1415926f / 180.0f; }

    void ParticleSystem::Initialize()
    {
        LoadPresets("assets/JSON/particles.json");
        std::cout << "[ParticleSystem] Initialize OK\n";
    }

    bool ParticleSystem::LoadPresets(const std::string& path)
    {
        std::ifstream f(path);
        if (!f.is_open()) return false;

        nlohmann::json j;
        f >> j;
        if (!j.contains("presets")) return false;

        for (auto it = j["presets"].begin(); it != j["presets"].end(); ++it)
        {
            Preset p;
            const auto& v = it.value();

            p.emitMode = v.value("emit_mode", "rate");
            p.emitRate = v.value("emit_rate", 0.0f);
            p.burstCount = v.value("burst_count", 0);
            p.maxParticles = v.value("max_particles", 100);

            p.spreadDeg = v.value("spread_deg", 360.0f);
            p.startSize = v.value("start_size", 0.2f);
            p.endSize = v.value("end_size", 0.0f);

            if (v.contains("lifetime")) { p.lifeMin = v["lifetime"][0]; p.lifeMax = v["lifetime"][1]; }
            if (v.contains("speed")) { p.speedMin = v["speed"][0];   p.speedMax = v["speed"][1]; }

            if (v.contains("start_color"))
                p.startColor = glm::vec4(v["start_color"][0], v["start_color"][1], v["start_color"][2], v["start_color"][3]);
            if (v.contains("end_color"))
                p.endColor = glm::vec4(v["end_color"][0], v["end_color"][1], v["end_color"][2], v["end_color"][3]);

            presets[it.key()] = p;
        }
        return true;
    }

    const ParticleSystem::Preset* ParticleSystem::GetPreset(const std::string& name) const
    {
        auto it = presets.find(name);
        if (it == presets.end()) return nullptr;
        return &it->second;
    }

    int ParticleSystem::FindDead(const EmitterRuntime& rt) const
    {
        for (int i = 0; i < (int)rt.state.size(); ++i)
            if (!rt.state[i].alive) return i;
        return -1;
    }

    void ParticleSystem::EnsurePool(Entity emitter, const Preset& p)
    {
        // NOTE: If Entity is not uint32, replace (uint32_t)emitter with your entity ID getter.
        auto& rt = runtime[emitter.GetID()];
        if (rt.poolBuilt) return;

        rt.pool.resize(p.maxParticles);
        rt.state.resize(p.maxParticles);

        for (int i = 0; i < p.maxParticles; ++i)
        {
            Entity pe = entityManager->CreateEntity();
            rt.pool[i] = pe;

            entityManager->AddComponent<Transform>(pe);
            entityManager->AddComponent<MeshRenderer>(pe);

            auto& mr = entityManager->GetComponent<MeshRenderer>(pe);

            // Assign a quad mesh/material. Your engine already has something like this.
            graphics->AssignMeshAndMaterial(mr, "quad");

            mr.visible = false;
            mr.tint = p.startColor;
            mr.layer = 50; // optional: render above most sprites
        }

        rt.poolBuilt = true;
    }

    void ParticleSystem::SpawnRate(Entity emitter, ParticleEmitter& em, const Preset& p, float dt, const Transform& et)
    {
        auto& rt = runtime[emitter.GetID()];

        em.emitAccumulator += dt * p.emitRate;
        int spawnCount = (int)em.emitAccumulator;
        em.emitAccumulator -= (float)spawnCount;

        for (int s = 0; s < spawnCount; ++s)
        {
            int idx = FindDead(rt);
            if (idx < 0) return;

            auto& st = rt.state[idx];
            st.alive = true;
            st.age = 0.0f;
            st.lifetime = RandF(p.lifeMin, p.lifeMax);

            float angle = DegToRad(RandF(-p.spreadDeg * 0.5f, p.spreadDeg * 0.5f));
            float speed = RandF(p.speedMin, p.speedMax);
            st.vel = Vector2D(std::cos(angle) * speed, std::sin(angle) * speed);

            Entity pe = rt.pool[idx];
            auto& tr = entityManager->GetComponent<Transform>(pe);
            auto& mr = entityManager->GetComponent<MeshRenderer>(pe);

            tr.position = et.position;
            tr.scale = Vector2D(p.startSize, p.startSize);

            mr.visible = true;
            mr.tint = p.startColor;
        }
    }

    void ParticleSystem::SpawnBurst(Entity emitter, ParticleEmitter& em, const Preset& p, const Transform& et)
    {
        auto& rt = runtime[emitter.GetID()];
        if (em.burstFired) return;

        for (int s = 0; s < p.burstCount; ++s)
        {
            int idx = FindDead(rt);
            if (idx < 0) break;

            auto& st = rt.state[idx];
            st.alive = true;
            st.age = 0.0f;
            st.lifetime = RandF(p.lifeMin, p.lifeMax);

            float angle = DegToRad(RandF(0.0f, p.spreadDeg));
            float speed = RandF(p.speedMin, p.speedMax);
            st.vel = Vector2D(std::cos(angle) * speed, std::sin(angle) * speed);

            Entity pe = rt.pool[idx];
            auto& tr = entityManager->GetComponent<Transform>(pe);
            auto& mr = entityManager->GetComponent<MeshRenderer>(pe);

            tr.position = et.position;
            tr.scale = Vector2D(p.startSize, p.startSize);

            mr.visible = true;
            mr.tint = p.startColor;
        }

        em.burstFired = true;
    }

    void ParticleSystem::UpdateParticles(Entity emitter, const Preset& p, float dt)
    {
        auto& rt = runtime[emitter.GetID()];

        for (int i = 0; i < (int)rt.state.size(); ++i)
        {
            auto& st = rt.state[i];
            if (!st.alive) continue;

            st.age += dt;
            float t = st.age / st.lifetime;

            Entity pe = rt.pool[i];
            auto& tr = entityManager->GetComponent<Transform>(pe);
            auto& mr = entityManager->GetComponent<MeshRenderer>(pe);

            if (t >= 1.0f)
            {
                st.alive = false;
                mr.visible = false;
                continue;
            }

            tr.position += st.vel * dt;

            float size = p.startSize + (p.endSize - p.startSize) * t;
            tr.scale = Vector2D(size, size);

            mr.tint = p.startColor + (p.endColor - p.startColor) * t;
        }
    }

    void ParticleSystem::Update(float dt)
    {
        if (!entityManager || !graphics) return;

        static int once = 0;
        if (once < 5) { std::cout << "[ParticleSystem] Update running\n"; once++; }

		auto entities = entityManager->GetAllEntities();

        for (const auto& e : entities)
        {
            if (!entityManager->HasComponent<Transform>(e)) continue;
            if (!entityManager->HasComponent<ParticleEmitter>(e)) continue;

            auto& t = entityManager->GetComponent<Transform>(e);
            auto& pe = entityManager->GetComponent<ParticleEmitter>(e);

            if (!pe.enabled) continue;

            const Preset* p = GetPreset(pe.preset);
            if (!p) continue;

            EnsurePool(e, *p);

            if (p->emitMode == "burst")
                SpawnBurst(e, pe, *p, t);
            else
                SpawnRate(e, pe, *p, dt, t);

            UpdateParticles(e, *p, dt);
        }
    }

} // namespace Framework
