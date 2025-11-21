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
#include "ConfigReader.h"
#include "Component.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

// This holds the path to the combined JSON file (e.g. assets/animations.json)
namespace { std::string g_animationConfigPath; }

namespace Framework {
	
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
	{}

	/**
	===============================================================================
	 * @brief Destructor - Cleans up animation system resources
	 *
	 * Currently handles basic cleanup. Can be extended for
	 * resource deallocation if needed in the future.
	===============================================================================
	 */
	AnimationSystem::~AnimationSystem() {}

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

		// === Sprite sheet frame stepping ===
		for (Entity e : entityManager->GetAllEntities()) {
			if (!entityManager->HasComponent<SpriteAnimation>(e)) continue;

			auto& anim = entityManager->GetComponent<SpriteAnimation>(e);

			if (!anim.playing) continue;

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
				anim.elapsedTime += dt;

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
			std::string selected =
				groupMap[anim.group][anim.direction];

			if (anim.animName != selected)
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

				if (anim.currentFrame >= anim.frameCount) {
					anim.currentFrame = anim.loop ? anim_current_frame : anim.frameCount - anim_frame_mod;
				}

				LOG_INFO("ANIM", "Entity %u Animation '%s' advanced to frame %d", (unsigned)e.id, anim.animName.c_str(), anim.currentFrame);
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

		for (auto& [key, animObj] : j["animations"].items()) {
			
			AnimEntry entry;
			entry.name = key;
			entry.file = key;

			if (animObj.contains("key"))
				entry.key = animObj["key"].get<std::string>()[0];

			animEntries.push_back(entry);

			AnimGroup group;
			AnimDirection dir = AnimDirection::None;

			if		(key.rfind("Idle_", 0) == 0)		group = AnimGroup::Idle;
			else if (key.rfind("Walk_", 0) == 0)		group = AnimGroup::Walk;
			else if (key.rfind("Attack_", 0) == 0)		group = AnimGroup::Attack;
			else if (key.rfind("Injured_", 0) == 0)	group = AnimGroup::Injured;
			else if (key == "Death")					        group = AnimGroup::Death;

			if		(key.find("front") != std::string::npos)	 dir = AnimDirection::Front;
			else if (key.find("back") != std::string::npos)		 dir = AnimDirection::Back;
			else if (key.find("sideview") != std::string::npos) dir = AnimDirection::Side;
			else													 dir = AnimDirection::None;

			groupMap[group][dir] = key;

			LOG_INFO("ANIM", "Added animation: name=%s key=%c",
				entry.name.c_str(),
				entry.key ? entry.key : '-');
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
			return;
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

			// Let GraphicsSystemV2 assign mesh + material for this sprite
			gfx->AssignMeshAndMaterial(mr, spritePath);
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