/**
===============================================================================
 File:           MovementSystem.cpp
 Author:         Josh Ong
 Email:          josh.o@digipen.edu
 Date:           2025-09-24
 Contribution:   100%
 ------------------------------------------------------------------------------
 Implementation of the MovementSystem class.

  Design notes:
  This file implements the entity movement logic. In its Update() loop, the
  system gets all relevant entities from the EntityManager. For each one, it
  reads WASD input from the InputSystem to create a direction vector.

  A key part of the implementation is normalizing the direction vector. This
  prevents entities from moving faster on the diagonal than they do along
  the cardinal axes. The final position is calculated using the entity's
  speed and delta time, and then clamped to the screen boundaries.

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
 */
#include "Precompiled.h"
#include "PlayerManager.h"
#include "ConfigReader.h"
namespace Framework
{
    /**
     * @brief Constructs the MovementSystem
     *
     * Initializes entity manager and input system pointers to nullptr.
     * These must be set via SetEntityManager() and SetInputSystem() before use.
     */
    MovementSystem::MovementSystem() : entityManager(nullptr), inputSystem(nullptr)
    {
    }

    /**
     * @brief Destructor for MovementSystem
     */
    MovementSystem::~MovementSystem()
    {
    }

    /**
     * @brief Initializes the movement system
     *
     * Prints initialization confirmation to console.
     */
    void MovementSystem::Initialize()
    {
        ConfigReader::LoadConfig("assets/valueloader.txt");
        y_mov_displacement = ConfigReader::GetFloat("y_mov_displacement", 0.0f);
        x_mov_displacement = ConfigReader::GetFloat("x_mov_displacement", 0.0f);
        default_zero = ConfigReader::GetFloat("default_zero", 0.0f);
        input_dir_x = ConfigReader::GetFloat("input_dir_x", 0.0f);
        input_dir_y = ConfigReader::GetFloat("input_dir_y", 0.0f);
        scale_multiplier = ConfigReader::GetFloat("scale_multiplier", 0.0f);
        rotation_angle = ConfigReader::GetFloat("rotation_angle", 0.0f);
        x_bound = ConfigReader::GetFloat("x_bound", 0.0f);
        y_bound = ConfigReader::GetFloat("y_bound", 0.0f);
        std::cout << "MovementSystem: Initialized\n";
    }

    /**
     * @brief Updates movement for all entities with Movement components
     * @param dt Delta time since last frame in seconds
     *
     * For each entity with both Transform and Movement components:
     * 1. Reads WASD input to determine movement direction
     * 2. Normalizes diagonal movement to prevent faster diagonal speed
     * 3. Applies velocity-based movement: position += direction * speed * dt
     * 4. Clamps position to screen bounds [-1, 1] in both axes
     *
     * @note Does nothing if entityManager or inputSystem are not set
     */
    void MovementSystem::Update(float dt)
    {
        DBG_SCOPE_SYS("Movement System", eng::debug::Subsystem::Gameplay);

        if (!entityManager || !inputSystem) return;

        if (!Framework::CORE || !Framework::CORE->IsPlaying()) return;

        if(Framework::CORE->IsPlaying())
        {
            for (Entity entity : entityManager->GetAllEntities())
            {
                if (entityManager->HasComponent<Transform>(entity) &&
                    entityManager->HasComponent<Movement>(entity))
                {
                    // Skip entities controlled by Lua scripts
                    if (entityManager->HasComponent<ScriptComponent>(entity)) {
                        continue;
                    }

                    auto& transform = entityManager->GetComponent<Transform>(entity);
                    auto& movement = entityManager->GetComponent<Movement>(entity);

                    // Get input direction from WASD (start from zero, not config values)
                    Vector2D inputDir(0.0f, 0.0f);

                    if (inputSystem->IsKeyDown(KEY_W)) inputDir.y += y_mov_displacement;
                    if (inputSystem->IsKeyDown(KEY_S)) inputDir.y -= y_mov_displacement;
                    if (inputSystem->IsKeyDown(KEY_A)) inputDir.x -= x_mov_displacement;
                    if (inputSystem->IsKeyDown(KEY_D)) inputDir.x += x_mov_displacement;

                   // ============================================================================
                   // ANIMATION LOGIC MOVED TO: assets/scripts/AnimationController.lua
                   // ============================================================================
                   // Animation control has been refactored into a reusable Lua script.
                   // To add animation control to an entity, attach the AnimationController script:
                   //
                   //   AddScriptComponentToEntity(entityID, "assets/scripts/AnimationController.lua")
                   //
                   // Features now in AnimationController.lua:
                   //   - Automatic Idle/Walk transitions based on movement
                   //   - Direction-based sprite flipping and animation selection
                   //   - Attack/Injured/Death animation triggers (KEY_K/J/L)
                   //   - Movement blocking during Death animation
                   //
                   // Benefits of Lua-based animation control:
                   //   - Reusable across any entity type
                   //   - Hot-reloadable without recompilation
                   //   - Easy to customize per-entity
                   //   - Cleaner separation of concerns (Movement vs Animation)
                   // ============================================================================

                    // ---------------------- Scaling controls (KEY 3 / 4) ----------------------
                    // DEBUG/TEST FEATURE: Manual scaling controls
                    if (inputSystem->IsKeyDown(KEY_3))
                    {
                        transform.scale.x += scale_multiplier * dt;
                        transform.scale.y += scale_multiplier * dt;
                    }
                    if (inputSystem->IsKeyDown(KEY_4))
                    {
                        transform.scale.x -= scale_multiplier * dt;
                        transform.scale.y -= scale_multiplier * dt;
                    }

                    // ---------------------- Rotation controls (KEY 7 / 8) ----------------------
                    // DEBUG/TEST FEATURE: Manual rotation controls
                    if (inputSystem->IsKeyDown(KEY_7))
                    {
                        transform.rotation += rotation_angle * dt;
                    }
                    if (inputSystem->IsKeyDown(KEY_8))
                    {
                        transform.rotation -= rotation_angle * dt;
                    }

                    // Normalize diagonal movement
                    if (inputDir.length() > default_zero) {
                        inputDir.normalize();
                    }

                    // Update movement direction
                    movement.direction = inputDir;

                    //Apply movement
                    if (!movement.blocked) {
                        transform.position += movement.direction * movement.moveSpeed * dt;
                    }
                    // Keep on screen
                    if (transform.position.x > x_bound) transform.position.x = x_bound;
                    if (transform.position.x < -x_bound) transform.position.x = -x_bound;
                    if (transform.position.y > y_bound) transform.position.y = y_bound;
                    if (transform.position.y < -y_bound) transform.position.y = -y_bound;
                }
            }
        }
    }

    /**
     * @brief Handles engine messages
     * @param message Pointer to the message to process
     *
     * Currently does not process any messages.
     */
    void MovementSystem::SendEngineMessage(Message* message)
    {
        (void)message;
    }
}