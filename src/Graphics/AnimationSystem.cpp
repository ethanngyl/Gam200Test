/**
===============================================================================
 File:           AnimationSystem.cpp
 Author:         TAN WEI LEONG
 Email:          weileong.tan@digipen.edu
 Date:           2025-11-04
 Contribution:   100%
 ------------------------------------------------------------------------------
ANIMATION SYSTEM IMPLEMENTATION

Purpose:
    This file contains the complete implementation of the AnimationSystem class.
    It handles the core animation logic including frame timing, entity processing,
    and message handling within the ECS framework.

Implementation Details:
    - Uses delta time for frame-accurate animation timing
    - Processes all entities with SpriteAnimation component each frame
    - Handles both looping and one-shot animation types
    - Provides debug output for animation state changes
    - Responds to external messages for animation control

Component Requirements:
    Entities must have SpriteAnimation component with:
    - playing: bool (whether animation is active)
    - elapsedTime: float (accumulated time for current frame)
    - frameTime: float (duration to display each frame)
    - currentFrame: int (current frame index)
    - frameCount: int (total frames in animation)
    - loop: bool (whether animation should loop)

Usage:
    The system automatically processes animations during each update cycle.
    No manual per-entity animation updates are required.
===============================================================================
 */

#include "Precompiled.h"
#include "AnimationSystem.h"
#include "ConfigReader.h"
#include "Component.h"
#include "TimeConstants.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

// This holds the path to the combined JSON file (e.g. assets/animations.json)
namespace { std::string g_animationConfigPath; }

namespace Framework {
    // Helper: convert string from JSON to AnimGroup enum
    static AnimGroup ParseAnimGroup(const std::string& s)
    {
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (lower == "idle")      return AnimGroup::Idle;
        if (lower == "walk")      return AnimGroup::Walk;
        if (lower == "attack")    return AnimGroup::Attack;
        if (lower == "injured")   return AnimGroup::Injured;
        if (lower == "death")     return AnimGroup::Death;

        return AnimGroup::Count; // invalid / unknown
    }

    // Helper: convert string from JSON to AnimDirection enum
    static AnimDirection ParseAnimDirection(const std::string& s)
    {
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (lower == "front")     return AnimDirection::Front;
        if (lower == "back")      return AnimDirection::Back;
        if (lower == "side")      return AnimDirection::Side;
        if (lower.empty())        return AnimDirection::None;

        return AnimDirection::None;
    }

    /**
    ===============================================================================
     * @brief Constructor - Initializes animation system with default values
     *
     * Sets up:
     * - currentFrame to 0 (first frame)
     * - frameTimer to 0.0f (ready to start timing)
     * - isPlaying to false (paused by default)
     * - entityManager to nullptr (must be set later)
    ===============================================================================
     */
    AnimationSystem::AnimationSystem()
        : currentFrame(0)
        , frameTimer(0.0f)
        , isPlaying(false)
        , entityManager(nullptr)
        , max_static_threshold(0.0f)
        , default_zero(0.0f)
        , anim_current_frame(0)
        , anim_frame_mod(0)
        , frameDurations()
        , animationFrames()
    {
    }

    /**
    ===============================================================================
     * @brief Destructor - Cleans up animation system resources
     *
     * Currently handles basic cleanup. Can be extended for
     * resource deallocation if needed in the future.
     *
    ===============================================================================
     */
    AnimationSystem::~AnimationSystem() {
        LOG_INFO("ANIMATION", "Destroying Animation System...");
    }

    // ==================== CORE LIFECYCLE METHODS ====================

    /**
    ===============================================================================
     * @brief Initializes the animation system
     *
     * Called once when the system is first created. Performs:
     * - System validation checks
     * - Resource preparation
     * - Debug output confirmation
     *
     * @note Must be called before any Update() calls
    ===============================================================================
     */
    void AnimationSystem::Initialize() {
        ConfigReader::LoadConfig("valueloader.txt");
        max_static_threshold = ConfigReader::GetFloat("max_static_threshold", 0.001f);
        default_zero = ConfigReader::GetFloat("default_zero", 0.0);
        anim_current_frame = ConfigReader::GetInt("anim_current_frame", 0);
        anim_frame_mod = ConfigReader::GetInt("anim_frame_mod", 1);
        std::cout << "AnimationSystem: Initialized!" << std::endl;
    }

    /**
    ===============================================================================
     * @brief Updates all active animations
     *
     * Main update loop called every frame. Processes:
     * - All entities with SpriteAnimation component
     * - Frame timing and progression
     * - Looping behavior management
     * - Animation state transitions
     *
     * @param dt Delta time in seconds since last frame
     *        Used for frame-accurate animation timing
     *
     * @note Skips processing if entityManager is not set
     * @note Only processes animations marked as playing
    ===============================================================================
     */
    void AnimationSystem::Update(float dt) {
        if (!entityManager) return;

        // Collect entities whose one-shot animation finished with autoDestroyOnFinish
        std::vector<Entity> autoDestroyList;

        // === Sprite sheet frame stepping ===
        for (Entity e : entityManager->GetAllEntities()) {
            if (!entityManager->HasComponent<SpriteAnimation>(e)) continue;

            auto& anim = entityManager->GetComponent<SpriteAnimation>(e);

            if (!anim.playing) continue;

            // -----------------------------------------
            // RESET ANIMATION TO IDLE WHEN 'O' PRESSED
            // -----------------------------------------
            auto* input = CORE->GetInputSystem();
            if (input && input->IsKeyPressed(KEY_O))
            {
                anim.group = AnimGroup::Idle;
                anim.direction = AnimDirection::Front; // or keep previous direction
                anim.currentFrame = 0;
                anim.elapsedTime = 0.0f;
                anim.playing = true;
            }

            // -------------------------------
            // SKIP JSON CONFIG FOR STANDALONE ANIMATIONS (UI elements, etc.)
            // -------------------------------
            if (!anim.useJsonConfig)
            {
                // Just advance frames without JSON-based animation selection
                anim.elapsedTime += dt;

                if (anim.elapsedTime >= anim.frameTime) {
                    anim.elapsedTime = 0.0f;
                    anim.currentFrame++;

                    if (anim.currentFrame >= anim.frameCount)
                    {
                        if (anim.loop)
                        {
                            anim.currentFrame = 0;
                        }
                        else
                        {
                            // One-shot animation finished -> clamp to final frame first
                            anim.currentFrame = anim.frameCount - 1;

                            // Death should remain on last frame (already special-cased earlier),
                            // but keep this guard anyway.
                            if (anim.group != AnimGroup::Death)
                            {
                                // If this was an Attack one-shot, return to Idle immediately
                                if (anim.group == AnimGroup::Attack)
                                {
                                    anim.group = AnimGroup::Idle;
                                    anim.loop = true;
                                    anim.playing = true;
                                    anim.currentFrame = 0;
                                    anim.elapsedTime = 0.0f;

                                    // Force immediate swap to the Idle sprite sheet this frame
                                    const auto itGroup = groupMap.find(AnimGroup::Idle);
                                    if (itGroup != groupMap.end())
                                    {
                                        const auto itDir = itGroup->second.find(anim.direction);
                                        if (itDir != itGroup->second.end())
                                        {
                                            const std::string& idleName = itDir->second;

                                            anim.animName = idleName;
                                            auto* gfx = CORE->GetGraphicsSystem();
                                            LoadAnimation(e, anim, gfx, idleName);
                                        }
                                        else
                                        {
                                            // Fallback: let normal selection handle it next frame
                                            anim.animName.clear();
                                        }
                                    }
                                    else
                                    {
                                        anim.animName.clear();
                                    }
                                }
                                else
                                {
                                    // For other non-loop one-shots (if any), just freeze on last frame
                                    anim.playing = false;

                                    // Auto-destroy effect entities (explosions, etc.)
                                    if (anim.autoDestroyOnFinish) {
                                        autoDestroyList.push_back(e);
                                    }
                                }
                            }
                        }
                    }

                }
                continue;
            }

            // -------------------------------
            // SELECT JSON ANIMATION BY ENUMS
            // -------------------------------
            // ----------- DEATH ANIMATION OVERRIDE ----------------
            if (anim.group == AnimGroup::Death)
            {
                // ALWAYS use the Death animation (no direction variants)
                if (anim.animName != "Death")
                {
                    anim.animName = "Death";

                    auto* gfx = CORE->GetGraphicsSystem();
                    LoadAnimation(e, anim, gfx, "Death");

                    anim.currentFrame = 0;
                    anim.elapsedTime = 0.0f;
                }

                // Advance frames even during death
                anim.elapsedTime += Framework::Time::FIXED_DT;

                if (anim.elapsedTime >= anim.frameTime)
                {
                    anim.elapsedTime = 0.0f;
                    anim.currentFrame++;

                    // Stop on final frame since loop=false
                    if (anim.currentFrame >= anim.frameCount)
                        anim.currentFrame = anim.frameCount - 1;
                }

                continue; // Skip walking/idle animation logic ENTIRELY
            }

            // ----------- NORMAL ANIMATION SELECTION -------------------
            // Skip automatic animation switching for UI scroll animations
            // These are manually controlled by Lua scripts
            const bool isScrollAnim = anim.animName.rfind("Scroll", 0) == 0;
            if (isScrollAnim) {
                continue; // Skip automatic animation selection for scroll UI
            }



            // 
            std::string selected;
            auto itGroup = groupMap.find(anim.group);
            if (itGroup != groupMap.end()) {
                auto itDir = itGroup->second.find(anim.direction);
                if (itDir != itGroup->second.end()) {
                    selected = itDir->second;
                }
            }

            // If no matching animation found, fall back to Idle_front
            if (selected.empty()) {
                selected = "Idle_front";  // Safe fallback
            }

            // Optional per-entity override: "Mage_" + "Attack_front" => "Mage_Attack_front"
            // Only apply if that key exists in the loaded animation JSON.
            if (!anim.animPrefix.empty())
            {
                const std::string prefixed = anim.animPrefix + selected;
                if (animNameSet.find(prefixed) != animNameSet.end())
                {
                    selected = prefixed;
                }
            }

            // CRITICAL FIX: Force LoadAnimation if:
            // 1. Animation name changed
            // 2. OR frameWidth/frameHeight are invalid (0 or negative)
            // This ensures animation data is always properly initialized
            bool needsReload = (anim.animName != selected);
            bool frameDataInvalid = (anim.frameWidth <= 0 || anim.frameHeight <= 0);

            if (needsReload || frameDataInvalid)
            {
                anim.animName = selected;
                auto* gfx = CORE->GetGraphicsSystem();
                LoadAnimation(e, anim, gfx, anim.animName);
            }

            // -------------------------------
            // FREEZE ONLY WALK WHEN NOT MOVING
            // -------------------------------
            bool moving = false;
            if (entityManager->HasComponent<Movement>(e))
            {
                auto& mv = entityManager->GetComponent<Movement>(e);
                moving = fabs(mv.direction.x) > max_static_threshold ||
                    fabs(mv.direction.y) > max_static_threshold;
            }

            if (anim.group == AnimGroup::Walk && !moving)
                continue;

            // -------------------------------
            // FRAME ADVANCE
            // -------------------------------

            anim.elapsedTime += dt;

            if (anim.elapsedTime >= anim.frameTime) {
                anim.elapsedTime = default_zero;
                anim.currentFrame++;

                if (anim.currentFrame >= anim.frameCount)
                {
                    if (anim.loop)
                    {
                        anim.currentFrame = 0;
                    }
                    else
                    {
                        // Non-loop (one-shot) animations normally hit the end and would “freeze”
                        // on the last frame forever (because the engine keeps rendering that frame).
                        // That freeze is exactly what you see after Attack: it stays on the last
                        // attack frame until something else (like movement) changes the anim group.
                        // one-shot finished: clamp
                        anim.currentFrame = anim.frameCount - 1;

                        // Fix: when Attack one-shot finishes, automatically transition back to Idle.
                        // This guarantees the player returns to a standing/facing sprite immediately
                        // after the attack completes, without requiring another input.
                        // return to Idle after Attack finishes (Death stays clamped by its own override)
                        if (anim.group == AnimGroup::Attack)
                        {
                            anim.group = AnimGroup::Idle;
                            anim.loop = true;
                            anim.playing = true;
                            anim.currentFrame = 0;
                            anim.elapsedTime = 0.0f;

                            // IMPORTANT: also swap sprite sheet immediately this frame
                            std::string idleSelected;
                            auto itIdleGroup = groupMap.find(AnimGroup::Idle);
                            if (itIdleGroup != groupMap.end()) {
                                auto itIdleDir = itIdleGroup->second.find(anim.direction);
                                if (itIdleDir != itIdleGroup->second.end()) {
                                    idleSelected = itIdleDir->second;
                                }
                            }
                            if (idleSelected.empty()) {
                                idleSelected = "Idle_front";  // Safe fallback
                            }

                            // Apply per-entity prefix (e.g. "Mage_" + "Idle_front" => "Mage_Idle_front")
                            if (!anim.animPrefix.empty()) {
                                const std::string prefixed = anim.animPrefix + idleSelected;
                                if (animNameSet.count(prefixed) > 0) {
                                    idleSelected = prefixed;
                                }
                            }

                            if (!idleSelected.empty())
                            {
                                anim.animName = idleSelected;
                                auto* gfx = CORE->GetGraphicsSystem();
                                LoadAnimation(e, anim, gfx, anim.animName);
                            }
                        }
                        else
                        {
                            // other one-shots: freeze or stop (your call)
                            anim.playing = false;
                        }
                    }
                }


                // PERFORMANCE FIX: Removed per-frame logging (was causing 1-3ms lag per frame)
                // LOG_INFO("ANIM", "Entity %u Animation '%s' advanced to frame %d", (unsigned)e.id, anim.animName.c_str(), anim.currentFrame);
            }
        }

        // === Auto-destroy effect entities whose one-shot animation finished ===
        for (Entity eff : autoDestroyList) {
            if (entityManager->HasComponent<Transform>(eff)) {
                entityManager->DestroyEntity(eff);
            }
        }
    }

    /**
    ===============================================================================
     * @brief Handles engine messages for animation control
     *
     * @param message Pointer to the incoming message object
     *        Contains message ID and optional data payload
     *
     * Currently unused.
     * Animation events are handled via config + Update() movement checks.
     * Message-based animation control can be added in future iterations.
    ===============================================================================
     */
    void AnimationSystem::SendEngineMessage(Message* message) {
        // Animation system currently does not react to messages.
        (void)message; // silence unused variable warning
    }

    // ============================================================================
    // JSON-BASED ANIMATION CONFIG LOADING
    // ============================================================================
    // configPath is now the PATH to the combined JSON, e.g. "assets/animations.json"
    void AnimationSystem::LoadAnimationConfig(const std::string& configPath) {
        g_animationConfigPath = configPath;
        animEntries.clear();
        groupMap.clear();
        animNameSet.clear();
        std::ifstream file(configPath);
        if (!file.is_open()) {
            LOG_ERROR("ANIM", "Failed to open animation JSON: %s", configPath.c_str());
            return;
        }

        json j;
        try {
            file >> j;
        }
        catch (const std::exception& e) {
            LOG_ERROR("ANIM", "Failed to parse JSON '%s': %s", configPath.c_str(), e.what());
            return;
        }

        if (!j.contains("animations") || !j["animations"].is_object()) {
            LOG_ERROR("ANIM", "JSON '%s' missing 'animations' object", configPath.c_str());
            return;
        }

        // =====================================================================
        // PASS 1: Collect ALL animation names into animNameSet & animEntries
        // =====================================================================
        for (auto& [key, animObj] : j["animations"].items()) {

            AnimEntry entry;
            entry.name = key;
            entry.file = key;
            animNameSet.insert(key);

            if (animObj.contains("key") && animObj["key"].is_string()) {
                const std::string keyStr = animObj["key"].get<std::string>();
                if (!keyStr.empty())
                    entry.key = keyStr[0];
            }

            animEntries.push_back(entry);
        }

        // =====================================================================
        // PASS 2: Build groupMap from BASE animations only (skip variants)
        //
        // A "variant" is an animation whose name = SomePrefix_ + BaseName,
        // where BaseName is ALSO a valid animation in animNameSet.
        // e.g. "Mage_Attack_front" is a variant because "Attack_front" exists.
        // Variants are looked up at runtime via the per-entity animPrefix field,
        // so they must NOT overwrite the base entry in groupMap.
        // =====================================================================
        for (auto& [key, animObj] : j["animations"].items()) {

            std::string groupStr = animObj.value("group", std::string{});
            std::string dirStr = animObj.value("direction", std::string{});

            AnimGroup group = ParseAnimGroup(groupStr);
            AnimDirection direction = ParseAnimDirection(dirStr);

            if (group == AnimGroup::Count) {
                LOG_WARN("ANIM",
                    "Animation '%s' has invalid or missing group='%s'; skipping groupMap entry",
                    key.c_str(), groupStr.c_str());
                continue;
            }

            // For Death animation, we don't care about direction
            if (group == AnimGroup::Death) {
                direction = AnimDirection::None;
            }

            // Detect variant: check every underscore position to see if the
            // suffix after the prefix is itself a known base animation name.
            // e.g. "Mage_Attack_front" -> suffix "Attack_front" exists -> variant
            bool isVariant = false;
            for (size_t i = 1; i < key.size(); ++i) {
                if (key[i] == '_') {
                    std::string suffix = key.substr(i + 1);
                    if (animNameSet.count(suffix) > 0) {
                        isVariant = true;
                        LOG_INFO("ANIM", "Animation '%s' is a variant of '%s' - skipping groupMap",
                            key.c_str(), suffix.c_str());
                        break;
                    }
                }
            }

            if (!isVariant) {
                groupMap[group][direction] = key;
            }

            LOG_INFO("ANIM", "Added animation: name=%s group=%s dir=%s variant=%s",
                key.c_str(),
                groupStr.c_str(),
                dirStr.c_str(),
                isVariant ? "YES" : "no");
        }

        LOG_INFO("ANIM", "Total animations loaded from JSON: %zu", animEntries.size());
    }

    // configPath PARAMETER is now the ANIMATION NAME (e.g. "Idle"),
    // the JSON PATH comes from g_animationConfigPath.
    void AnimationSystem::LoadAnimation(Entity e, SpriteAnimation& anim, GraphicsSystemV2* gfx, const std::string& configPath) {
        if (!gfx) {
            LOG_ERROR("ANIM", "GraphicsSystemV2 pointer is null");
            return;
        }

        if (!entityManager) {
            LOG_ERROR("ANIM", "EntityManager is null in AnimationSystem::LoadAnimation");
            return;
        }

        if (g_animationConfigPath.empty()) {
            LOG_ERROR("ANIM", "Animation JSON path not set. Call LoadAnimationConfig() first.");
            return;
        }

        const std::string animationName = configPath; // reuse parameter as name

        std::ifstream file(g_animationConfigPath);
        if (!file.is_open()) {
            LOG_ERROR("ANIM", "Failed to open animation JSON: %s", g_animationConfigPath.c_str());
            return;
        }

        json j;
        try {
            file >> j;
        }
        catch (const std::exception& e) {
            LOG_ERROR("ANIM", "Failed to parse JSON '%s': %s", g_animationConfigPath.c_str(), e.what());
            return;
        }

        if (!j.contains("animations") || !j["animations"].is_object()) {
            LOG_ERROR("ANIM", "JSON '%s' missing 'animations' object", g_animationConfigPath.c_str());
            return;
        }

        auto itAnim = j["animations"].find(animationName);
        if (itAnim == j["animations"].end()) {
            LOG_ERROR("ANIM", "Animation '%s' not found in %s",
                animationName.c_str(), g_animationConfigPath.c_str());


            itAnim = j["animations"].find("Idle_front");
            if (itAnim == j["animations"].end()) {
                return;  // Only fail if fallback also missing
            }
            anim.animName = "Idle_front";
        }

        const json& a = *itAnim;

        LOG_INFO("ANIM", "Loading animation '%s' from JSON '%s'",
            animationName.c_str(), g_animationConfigPath.c_str());

        // ----------------------------
        // 1. Load the REAL sprite sheet
        // ----------------------------
        std::string spritePath = a.value("sprite", std::string());
        if (spritePath.empty()) {
            LOG_ERROR("ANIM", "Animation '%s' in JSON has no 'sprite' field",
                animationName.c_str());
            return;
        }

        auto& resourceManager = gfx->GetResourceManager();
        anim.spriteSheet = resourceManager.LoadTexture(spritePath);
        if (!anim.spriteSheet.IsValid()) {
            LOG_ERROR("ANIM", "Failed to load texture for animation '%s' (sprite=%s)",
                animationName.c_str(), spritePath.c_str());
            return;
        }

        Texture* tex = resourceManager.GetTexture(anim.spriteSheet);
        if (!tex) {
            LOG_ERROR("ANIM", "Texture pointer null for '%s'", spritePath.c_str());
            return;
        }

        // ----------------------------
        // 2. Configure layout values
        // ----------------------------
        anim.rows = a.value("rows", 1);
        anim.columns = a.value("columns", 1);
        anim.frameCount = a.value("frameCount", 1);
        anim.frameTime = static_cast<float>(a.value("frameTime", 0.0));
        anim.loop = a.value("loop", true);
        anim.uvShrinkPx = static_cast<float>(a.value("uvShrinkPx", 0.0));

        if (anim.rows <= 0)    anim.rows = 1;
        if (anim.columns <= 0) anim.columns = 1;

        anim.frameWidth = tex->GetWidth() / anim.columns;
        anim.frameHeight = tex->GetHeight() / anim.rows;

        anim.playing = true;
        anim.currentFrame = 0;
        anim.elapsedTime = 0.0f;

        // ----------------------------
        // 3. Assign to renderer (MeshRenderer)
        // ----------------------------
        if (!entityManager->HasComponent<MeshRenderer>(e)) {
            LOG_WARN("ANIM", "Entity %u has no MeshRenderer; sprite loaded but not assigned", (unsigned)e.id);
        }
        else {
            auto& mr = entityManager->GetComponent<MeshRenderer>(e);
            mr.spriteName = spritePath;

            // DON'T call AssignMeshAndMaterial - it overwrites the unique material instance!
            // The entity already has its material set up from spawning.
            // Just update the sprite name so the animation texture is referenced.
            // gfx->AssignMeshAndMaterial(mr, spritePath);  // COMMENTED OUT - preserves unique materials

            // FIX: Also update the material's texture handle immediately so it's
            // consistent with the new sprite sheet before the next render pass.
            // Without this, there's a 1-frame window where the material still references
            // the old texture while anim.spriteSheet already points to the new one.
            if (mr.material.IsValid()) {
                auto& resourceManager = gfx->GetResourceManager();
                Material* mat = resourceManager.GetMaterial(mr.material);
                if (mat) {
                    mat->albedoTexture = anim.spriteSheet;
                }
            }

            LOG_INFO("ANIM", "Updated MeshRenderer spriteName to '%s' (preserved existing material)", spritePath.c_str());
        }

        LOG_INFO("ANIM",
            "Animation '%s' loaded: sprite=%s rows=%d cols=%d frames=%d frameTime=%.3f loop=%s",
            animationName.c_str(),
            spritePath.c_str(),
            anim.rows,
            anim.columns,
            anim.frameCount,
            anim.frameTime,
            anim.loop ? "true" : "false");
    }
}
