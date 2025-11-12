/**
 ===============================================================================
 File:           MovementSystem.h
 Author:         Josh Ong
 Email:          josh.o@digipen.edu
 Date:           2025-09-24
 Contribution:   100%
 ------------------------------------------------------------------------------
 Header file for the MovementSystem class.

  Design notes:
  This file declares the MovementSystem, which follows an Entity-Component-System
  (ECS) design. Its sole purpose is to move entities that have both a Transform
  and a Movement component.

  It depends on two other systems: the EntityManager (to get entity data) and
  the InputSystem (to read keyboard input). These dependencies are provided
  externally via setter methods to keep the system decoupled. The Update()
  method takes a delta time (`dt`) parameter to ensure movement is smooth and
  independent of the frame rate.
===============================================================================
 */
#pragma once
#include "Interface.h"
#include "ECSEntityManager.h"
#include "Event/Event.h"


namespace Framework
{

    class EventSystem;

    class ProjectileMovementSystem : public EngineSystem
    {
    public:
        /**
         * @brief Constructs the MovementSystem
         */
        ProjectileMovementSystem();

        /**
         * @brief Destructor
         */
        virtual ~ProjectileMovementSystem();

        /**
         * @brief Initializes the movement system
         */
        virtual void Initialize() override;

        /**
         * @brief Updates entity positions based on input
         * @param dt Delta time since last frame in seconds
         *
         * Processes movement for all entities with Transform and Movement components.
         * Reads WASD input, normalizes diagonal movement, and clamps to screen bounds.
         */
        virtual void Update(float dt) override;

        /**
         * @brief Handles engine messages
         * @param message Message to process
         */
        virtual void SendEngineMessage(Message* message) override;


        /**
         * @brief Sets the entity manager for accessing entity components
         * @param em Pointer to the EntityManager instance
         */
        void SetEntityManager(EntityManager* em) { entityManager = em; }

        void SetEventSystem(EventSystem* es) { eventSystem = es; }


    private:
        EntityManager* entityManager; //Entity component database
        EventSystem* eventSystem;

        void CheckProjectileEnemyCollisions();
    };
}